#!/usr/bin/env bash
# test_check_quiet_version.sh — scripts/check_quiet_version.sh must go red on a
# binary that prints anything besides its version line, and green on one that
# prints only that. Driven on fake binaries; red first on the real one too
# (the pre-fix build printed five "[DEBUG]" lines around "CadGoose v1.82").
#
#   bash tests/profiling/test_check_quiet_version.sh
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CHECK="$REPO/scripts/check_quiet_version.sh"
DIR="$(mktemp -d "${TMPDIR:-/tmp}/cadgoose-quiet-XXXXXX")"
trap 'rm -rf "$DIR"' EXIT
FAILURES=0

expect() {   # name, wanted rc (0 or nonzero as 1), fake body
    local name="$1" want="$2" body="$3" rc=0
    printf '#!/bin/sh\n%s\n' "$body" > "$DIR/CadGoose"
    chmod +x "$DIR/CadGoose"
    bash "$CHECK" "$DIR/CadGoose" > /dev/null 2>&1 < /dev/null || rc=$?
    if { [[ $want -eq 0 && $rc -eq 0 ]] || [[ $want -ne 0 && $rc -ne 0 ]]; }; then
        echo "✓ $name"
    else
        echo "✗ $name (rc=$rc)"; FAILURES=$((FAILURES + 1))
    fi
}

expect "version line only is green" 0 'echo "CadGoose v1.82"'
expect "stderr noise before the version is red" 1 'echo "[DEBUG] main() starting" >&2; echo "CadGoose v1.82"'
expect "stderr noise after the version is red" 1 'echo "CadGoose v1.82"; echo "[DEBUG] returned 0" >&2'
expect "no output is red" 1 'exit 0'
expect "a nonzero exit is red" 1 'echo "CadGoose v1.82"; exit 3'

[[ $FAILURES -eq 0 ]] || { echo "✗ $FAILURES case(s) failed"; exit 1; }
