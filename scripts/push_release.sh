#!/usr/bin/env bash
# push_release.sh — tag a release, wait for CI, verify Homebrew tap update
# Usage: ./scripts/push_release.sh [vX.Y]  (default: bump minor from latest tag)

set -euo pipefail

REPO="ztomer/CadGoose"
TAP_REPO="ztomer/homebrew-tap"
TAP_FILE="Casks/cadgoose.rb"

# Tunables — no magic numbers in the logic below.
CI_POLL_ATTEMPTS=60   # max polls while waiting for the release CI run
CI_POLL_INTERVAL=5     # seconds between CI-run polls
TAP_SETTLE_SECONDS=10  # wait for the tap GHA to push after CI

info() { echo "→ $*"; }
ok()   { echo "✓ $*"; }
err()  { echo "✗ $*" >&2; exit 1; }

# Check if gh is authenticated. If GITHUB_TOKEN is set but invalid, try unsetting it to fall back to keyring/keychain.
if ! gh auth status >/dev/null 2>&1; then
    if [[ -n "${GITHUB_TOKEN:-}" ]]; then
        info "GITHUB_TOKEN is set but may be invalid. Attempting to run without GITHUB_TOKEN to use keyring..."
        if env -u GITHUB_TOKEN gh auth status >/dev/null 2>&1; then
            unset GITHUB_TOKEN
            ok "Successfully bypassed invalid GITHUB_TOKEN and authenticated using keyring."
        else
            err "gh CLI is not authenticated. Run 'gh auth login' first."
        fi
    else
        err "gh CLI is not authenticated. Run 'gh auth login' first."
    fi
fi

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

# Version without the leading "v" — what the Homebrew cask stores.
SHORT_VERSION="${VERSION#v}"

info "Releasing $VERSION"

# 2. Ensure clean working tree
git diff --quiet || err "Working tree not clean. Commit or stash changes first."
git diff --cached --quiet || err "Staged changes present. Commit first."

# 3. Tag and push
if git rev-parse "$VERSION" >/dev/null 2>&1; then
    info "Tag $VERSION already exists locally."
else
    info "Creating tag $VERSION"
    git tag "$VERSION"
fi

info "Pushing tag to origin"
git push origin "$VERSION"

# Capture existing release runs before creating/triggering a new one
EXISTING_RUNS=$(gh run list --repo "$REPO" --event release --limit 10 --json databaseId --jq '[.[].databaseId] | join(",")' 2>/dev/null || echo "")

# Create GitHub release if it does not exist
if gh release view "$VERSION" --repo "$REPO" >/dev/null 2>&1; then
    info "GitHub release $VERSION already exists."
else
    info "Creating GitHub release $VERSION"
    gh release create "$VERSION" --title "$VERSION" --notes "Release $VERSION" --repo "$REPO"
fi

# 4. Wait for CI
info "Waiting for CI to complete..."
RUN_ID=""
for i in $(seq 1 "$CI_POLL_ATTEMPTS"); do
    LATEST_RUN_ID=$(gh run list --repo "$REPO" --event release --limit 10 --json databaseId,headBranch --jq "[.[] | select(.headBranch==\"$VERSION\") | .databaseId] | max" 2>/dev/null || true)
    if [[ "$LATEST_RUN_ID" == "null" ]]; then
        LATEST_RUN_ID=""
    fi
    if [[ -n "$LATEST_RUN_ID" ]]; then
        if [[ ",$EXISTING_RUNS," != *",$LATEST_RUN_ID,"* ]]; then
            RUN_ID="$LATEST_RUN_ID"
            break
        fi
    fi
    sleep "$CI_POLL_INTERVAL"
done

[[ -n "$RUN_ID" ]] || err "Could not find CI run for release $VERSION"

info "Monitoring run $RUN_ID"
gh run watch "$RUN_ID" --repo "$REPO"

# 5. Check conclusion
CONCLUSION=$(gh run view "$RUN_ID" --repo "$REPO" --json conclusion --jq '.conclusion')
[[ "$CONCLUSION" == "success" ]] || err "CI failed with: $CONCLUSION"

ok "CI passed"

# 6. Verify Homebrew tap cask version.
#    Read the cask file directly from the tap repo's default branch — authoritative
#    (no CDN/cache lag) and format-independent (no dependence on the tap
#    commit-message wording).
info "Checking Homebrew tap cask version..."
sleep "$TAP_SETTLE_SECONDS"  # Give the tap GHA time to push

CASK_RAW=$(gh api "repos/$TAP_REPO/contents/$TAP_FILE" --jq '.content' 2>/dev/null | base64 -d 2>/dev/null || true)
if [[ "$CASK_RAW" == *"version \"$SHORT_VERSION\""* ]]; then
    ok "Homebrew cask version is $SHORT_VERSION"
else
    err "Homebrew cask not updated to $SHORT_VERSION. Cask head: ${CASK_RAW:0:160}"
fi

info "Release $VERSION complete!"
echo
echo "Artifacts attached to GitHub release: $VERSION"
echo "Homebrew users can upgrade with: brew upgrade cadgoose"