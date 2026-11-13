#!/usr/bin/env bash
# push_release.sh — tag a release, read its CI once (never wait), bump the Homebrew cask through the tap gate
# Usage: ./scripts/push_release.sh [vX.Y]  (default: bump minor from latest tag)

set -euo pipefail

REPO="ztomer/CadGoose"
TAP_REPO="ztomer/homebrew-tap"
TAP_FILE="Casks/cadgoose.rb"

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
    TAGS=$(git tag --sort=-v:refname)
    LATEST=$(head -n 1 <<< "$TAGS")
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

# Create GitHub release if it does not exist
if gh release view "$VERSION" --repo "$REPO" >/dev/null 2>&1; then
    info "GitHub release $VERSION already exists."
else
    info "Creating GitHub release $VERSION"
    gh release create "$VERSION" --title "$VERSION" --notes "Release $VERSION" --repo "$REPO"
fi

# 4. Read the release CI ONCE, never wait on it (EST.CIWAIT, the rule of goh
#    GOH.CIWAIT). Hosted CI is advisory (owner 2026-11-11): the gate it re-runs
#    already passed at the pre-push hook. What the cask needs is the DMG this
#    run attaches, so: a red run refuses; the DMG attached bumps the cask; a run
#    still building (or not started) stops here with the release published and
#    the cask untouched -- every step above is idempotent, so re-running this
#    script with the same version once the DMG is attached finishes the job.
DMG_NAME="CadGoose-$VERSION.dmg"
RUN_LINE=$(gh run list --repo "$REPO" --event release --limit 20 \
    --json databaseId,headBranch,status,conclusion,url \
    --jq "[.[] | select(.headBranch==\"$VERSION\")] | max_by(.databaseId) // empty | [.status, (.conclusion // \"\"), .url] | @tsv" \
    2>/dev/null) || err "gh run list could not read the release CI for $VERSION"
IFS=$'\t' read -r RUN_STATUS RUN_CONCLUSION RUN_URL <<< "${RUN_LINE:-}" || true
if [[ "${RUN_STATUS:-}" == "completed" && "${RUN_CONCLUSION:-}" != "success" ]]; then
    err "release CI for $VERSION is ${RUN_CONCLUSION:-unknown} ($RUN_URL) -- cask not bumped"
fi
ASSETS=$(gh release view "$VERSION" --repo "$REPO" --json assets --jq '.assets[].name' 2>/dev/null) \
    || err "gh release view could not read the assets of $VERSION"
if ! grep -qxF "$DMG_NAME" <<< "$ASSETS"; then
    info "Release $VERSION is published; its CI (${RUN_STATUS:-not started}${RUN_URL:+ -- $RUN_URL}) has not attached $DMG_NAME yet."
    info "Not waiting (hosted CI is advisory). Cask NOT bumped: re-run ./scripts/push_release.sh $VERSION once the DMG is attached."
    exit 0
fi
ok "$DMG_NAME is attached to $VERSION"

# 6. Bump the Homebrew cask through the tap's OWN gate (homebrew-tap T2.1,
#    goh 6.8). tap_bump.sh hashes the DMG CI just attached, rewrites version
#    and sha256 together, runs the tap's tools/gate.sh --full and pushes the
#    commit pinned; a red tap gate pushes nothing. This used to be a CI step
#    that pushed to the tap with a token, which skipped that gate by
#    construction.
info "Bumping $TAP_FILE in $TAP_REPO via tap_bump.sh..."
"${GOH_DIR:-$HOME/Projects/gates_of_heck}/tools/release-kit/tap_bump.sh" \
    --tap "$TAP_REPO" --cask cadgoose --tag "$VERSION" \
    --artifact "https://github.com/$REPO/releases/download/$VERSION/CadGoose-$VERSION.dmg"
ok "Homebrew cask is at $SHORT_VERSION, through the tap's gate"

info "Release $VERSION complete!"
echo
echo "Artifacts attached to GitHub release: $VERSION"
echo "Homebrew users can upgrade with: brew upgrade cadgoose"