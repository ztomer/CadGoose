#!/usr/bin/env bash
# CadGoose release kit wrapper for goh ship (CG1.SHIPKIT).
# Usage: scripts/ship_release.sh [--check] <version>
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
GOH_DIR="${GOH_DIR:-$HOME/Projects/gates_of_heck}"
# shellcheck source=/dev/null
source "$GOH_DIR/tui/lib.sh"

CHECK=0
RAW_VER=""
while [ "$#" -gt 0 ]; do
  case "$1" in
    --check)
      CHECK=1
      shift
      ;;
    -*)
      die "ship_release.sh: unknown option $1"
      ;;
    *)
      if [ -n "$RAW_VER" ]; then
        die "ship_release.sh: unexpected extra argument $1"
      fi
      RAW_VER="$1"
      shift
      ;;
  esac
done

[ -n "$RAW_VER" ] || die "Usage: scripts/ship_release.sh [--check] <X.Y>"
V="${RAW_VER#v}"

cd "$REPO"

# CadGoose tags use two-part versions (v1.83, v1.82, ...) alongside legacy v0.5.3.
# Compare version tags numerically by integer tuple so v1.83 is read ahead of v0.5.3,
# and reject any target version <= latest tag (including stale v0.5.3-derived bumps).
VERSION_CHECK_OUT=$(python3 - "$V" <<'PY'
import re
import subprocess
import sys

target = sys.argv[1]
raw_tags = subprocess.run(
    ["git", "tag", "--list", "v*"],
    capture_output=True,
    text=True,
    stdin=subprocess.DEVNULL,
    check=True,
).stdout.splitlines()

parsed: list[tuple[tuple[int, ...], str]] = []
for tag in raw_tags:
    m = re.fullmatch(r"v(\d+(?:\.\d+)+)", tag.strip())
    if m:
        parsed.append((tuple(int(p) for p in m.group(1).split(".")), tag.strip()))

latest_tuple, latest_tag = max(parsed, key=lambda item: item[0]) if parsed else ((0,), "none")

if not re.fullmatch(r"\d+\.\d+", target):
    print(
        f"ship_release.sh: version '{target}' must use CadGoose X.Y format "
        f"(latest tag is {latest_tag})",
        file=sys.stderr,
    )
    sys.exit(1)

target_tuple = tuple(int(p) for p in target.split("."))
if target_tuple <= latest_tuple:
    print(
        f"ship_release.sh: version {target} does not advance past latest tag {latest_tag}",
        file=sys.stderr,
    )
    sys.exit(1)

print(latest_tag)
PY
) || die "version check failed for '$V'"

LATEST_TAG="$VERSION_CHECK_OUT"
info "Latest version tag resolved: ${LATEST_TAG}; target: v${V}"

BRANCH=$(git rev-parse --abbrev-ref HEAD)
[ "$BRANCH" = "main" ] || die "ship_release.sh: must run on branch main (currently on $BRANCH)"

if ! git diff --quiet || ! git diff --cached --quiet; then
  die "ship_release.sh: working tree has uncommitted changes"
fi

if git rev-parse -q --verify "refs/tags/v${V}" >/dev/null 2>&1; then
  die "ship_release.sh: tag v${V} already exists locally"
fi

GH_BIN="${GOH_GH:-gh}"
if command -v "$GH_BIN" >/dev/null 2>&1 || [ -x "$GH_BIN" ]; then
  if "$GH_BIN" release view "v${V}" >/dev/null 2>&1; then
    die "ship_release.sh: GitHub release v${V} already exists"
  fi
fi

CHANGELOG_FILE="docs/CHANGELOG.md"
[ -f "$CHANGELOG_FILE" ] || die "ship_release.sh: missing $CHANGELOG_FILE"
CUT_PY="$GOH_DIR/tools/release-kit/changelog_cut.py"
SUMMARY="${GOH_SHIP_SUMMARY:-}"

CHANGELOG_STATE=$(python3 - "$CHANGELOG_FILE" "$V" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
ver = sys.argv[2]
lines = text.splitlines()

ver_re = re.compile(r"^##\s+(?:v|\[)" + re.escape(ver) + r"(?:\]|\s|$)")
prev_re = re.compile(r"^##\s+(?:v|\[)(\d+\.\d+(?:\.\d+)?)(?:\]|\s|$)")
unrel_re = re.compile(r"^##\s+\[?unreleased\]?(?:\s|$)", re.IGNORECASE)

if any(ver_re.match(ln) for ln in lines):
    print("already_cut")
    sys.exit(0)

if any(unrel_re.match(ln) for ln in lines):
    print("unreleased_heading")
    sys.exit(0)

first_prev = next((i for i, ln in enumerate(lines) if prev_re.match(ln)), len(lines))
head_block = [
    ln.strip()
    for ln in lines[:first_prev]
    if ln.strip() and not ln.lstrip().startswith("# ")
]
if any(ln.startswith(("- ", "* ")) for ln in head_block):
    print("dated_entries")
    sys.exit(0)

print("empty")
PY
)

if [ "$CHECK" -eq 1 ]; then
  case "$CHANGELOG_STATE" in
    already_cut)
      python3 "$CUT_PY" check --version "$V" --file "$CHANGELOG_FILE"
      ;;
    unreleased_heading)
      if [ -n "$SUMMARY" ]; then
        python3 "$CUT_PY" ready --version "$V" --title "$SUMMARY" --file "$CHANGELOG_FILE"
      else
        python3 "$CUT_PY" ready --version "$V" --file "$CHANGELOG_FILE"
      fi
      ;;
    dated_entries)
      ok "docs/CHANGELOG.md holds unreleased entries above ${LATEST_TAG}"
      ;;
    *)
      die "ship_release.sh: $CHANGELOG_FILE has no unreleased entries above ${LATEST_TAG} and no ## v${V} stanza"
      ;;
  esac
  ok "Preflight passed for v${V} (latest tag ${LATEST_TAG})"
  exit 0
fi

case "$CHANGELOG_STATE" in
  already_cut)
    python3 "$CUT_PY" check --version "$V" --file "$CHANGELOG_FILE"
    ;;
  unreleased_heading)
    if [ -n "$SUMMARY" ]; then
      python3 "$CUT_PY" cut --version "$V" --title "$SUMMARY" --file "$CHANGELOG_FILE"
    else
      python3 "$CUT_PY" cut --version "$V" --file "$CHANGELOG_FILE"
    fi
    python3 "$CUT_PY" check --version "$V" --file "$CHANGELOG_FILE"
    ;;
  dated_entries)
    python3 - "$CHANGELOG_FILE" "$V" "$SUMMARY" <<'PY'
import datetime
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
ver = sys.argv[2]
summary = sys.argv[3].strip()
lines = path.read_text(encoding="utf-8").splitlines()

prev_re = re.compile(r"^##\s+(?:v|\[)(\d+\.\d+(?:\.\d+)?)(?:\]|\s|$)")
first_prev = next((i for i, ln in enumerate(lines) if prev_re.match(ln)), len(lines))

start_idx = 0
if lines and lines[0].startswith("# "):
    start_idx = 1
while start_idx < first_prev and not lines[start_idx].strip():
    start_idx += 1

unrel_lines = lines[start_idx:first_prev]
if not summary:
    for ln in unrel_lines:
        m = re.match(r"^##\s+.+?—\s+(.+?)\s*$", ln)
        if m:
            summary = m.group(1).strip()
            break
if not summary:
    summary = "release"

demoted = [
    ("#" + ln if re.match(r"^##\s+", ln) else ln)
    for ln in unrel_lines
]
while demoted and not demoted[-1].strip():
    demoted.pop()

today = datetime.date.today().strftime("%B %-d, %Y")
heading = f"## v{ver} — {today} — {summary}"

new_lines: list[str] = []
if lines and lines[0].startswith("# "):
    new_lines.extend([lines[0], ""])
new_lines.append(heading)
new_lines.append("")
new_lines.extend(demoted)
if first_prev < len(lines):
    new_lines.append("")
    new_lines.extend(lines[first_prev:])

path.write_text("\n".join(new_lines) + "\n", encoding="utf-8")
PY
    python3 "$CUT_PY" check --version "$V" --file "$CHANGELOG_FILE"
    ;;
  *)
    die "ship_release.sh: $CHANGELOG_FILE has no unreleased entries above ${LATEST_TAG} and no ## v${V} stanza"
    ;;
esac

if ! git diff --quiet -- "$CHANGELOG_FILE"; then
  git add "$CHANGELOG_FILE"
  MSG="chore(release): version ${V}"
  if [ -n "$SUMMARY" ]; then
    MSG="${MSG} - ${SUMMARY}"
  fi
  if [ -n "${CADGOOSE_RELEASE_TRAILER:-}" ]; then
    git commit -m "$MSG" -m "$CADGOOSE_RELEASE_TRAILER"
  else
    git commit -m "$MSG"
  fi
fi

REMOTE="${CADGOOSE_REMOTE:-origin}"
info "Pushing main to ${REMOTE}..."
git push "$REMOTE" "HEAD:refs/heads/main"

PUSH_RELEASE="${CADGOOSE_PUSH_RELEASE:-$REPO/scripts/push_release.sh}"
[ -x "$PUSH_RELEASE" ] || die "ship_release.sh: missing executable $PUSH_RELEASE"
exec bash "$PUSH_RELEASE" "v${V}"
