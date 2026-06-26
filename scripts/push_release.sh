#!/usr/bin/env bash
# push_release.sh — tag a release, wait for CI, verify Homebrew tap update
# Usage: ./scripts/push_release.sh [vX.Y]  (default: bump minor from latest tag)

set -euo pipefail

REPO="ztomer/CadGoose"
TAP_REPO="ztomer/homebrew-tap"
TAP_FILE="Casks/cadgoose.rb"

info() { echo "→ $*"; }
ok()   { echo "✓ $*"; }
err()  { echo "✗ $*" >&2; exit 1; }

# 1. Determine version
if [[ $# -eq 1 ]]; then
    VERSION="$1"
else
    LATEST=$(git tag --sort=-v:refname | head -1)
    # Extract major.minor, bump minor
    if [[ $LATEST =~ ^v([0-9]+)\.([0-9]+)$ ]]; then
        MAJOR="${BASH_REMATCH[1]}"
        MINOR="${BASH_REMATCH[2]}"
        VERSION="v$MAJOR.$((MINOR + 1))"
    else
        err "Cannot auto-bump from '$LATEST'. Specify version explicitly."
    fi
fi

# Validate format
[[ $VERSION =~ ^v[0-9]+\.[0-9]+$ ]] || err "Version must be vX.Y (e.g., v1.14)"

info "Releasing $VERSION"

# 2. Ensure clean working tree
git diff --quiet || err "Working tree not clean. Commit or stash changes first."
git diff --cached --quiet || err "Staged changes present. Commit first."

# 3. Tag and push
info "Creating tag $VERSION"
git tag "$VERSION"
info "Pushing tag to origin"
git push origin "$VERSION"

# 4. Wait for CI
info "Waiting for CI to complete..."
RUN_ID=""
for i in {1..60}; do
    RUN_ID=$(gh run list --repo "$REPO" --event push --limit 1 --json databaseId,conclusion,headBranch,event --jq '.[0] | select(.headBranch=="main" or .headBranch=="master") | .databaseId' 2>/dev/null || true)
    if [[ -n "$RUN_ID" ]]; then
        break
    fi
    sleep 5
done

[[ -n "$RUN_ID" ]] || err "Could not find CI run for tag push"

info "Monitoring run $RUN_ID"
gh run watch "$RUN_ID" --repo "$REPO"

# 5. Check conclusion
CONCLUSION=$(gh run view "$RUN_ID" --repo "$REPO" --json conclusion --jq '.conclusion')
[[ "$CONCLUSION" == "success" ]] || err "CI failed with: $CONCLUSION"

ok "CI passed"

# 6. Verify Homebrew tap update
info "Checking Homebrew tap..."
sleep 10  # Give tap action time to push

# Check latest commit on tap
TAP_SHA=$(gh api "repos/$TAP_REPO/commits/main" --jq '.sha')
TAP_MSG=$(gh api "repos/$TAP_REPO/commits/$TAP_SHA" --jq '.commit.message')

if [[ "$TAP_MSG" == *"$VERSION"* ]]; then
    ok "Homebrew tap updated: $TAP_MSG"
else
    err "Homebrew tap not updated yet. Latest commit: $TAP_MSG"
fi

# 7. Verify cask content
CASK_URL="https://raw.githubusercontent.com/$TAP_REPO/main/$TAP_FILE"
info "Verifying cask at $CASK_URL"
if curl -sf "$CASK_URL" | grep -q "version \"${VERSION#v}\""; then
    ok "Cask version matches $VERSION"
else
    err "Cask version mismatch. Check $CASK_URL"
fi

info "Release $VERSION complete!"
echo
echo "Artifacts attached to GitHub release: $VERSION"
echo "Homebrew users can upgrade with: brew upgrade cadgoose"