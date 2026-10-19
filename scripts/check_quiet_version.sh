#!/usr/bin/env bash
# check_quiet_version.sh — the shipped binary must print NOTHING but its version
# for `CadGoose --version` (stdout and stderr together), and exit 0.
#
# Why a gate: main() once wrote "[DEBUG] main() starting", every argv and the
# AppCli return code to stderr on every launch of the release build, `--version`
# included. A debug print is not a defect anyone sees in a test (the unit suite
# links AppCli, not main()), so the class is closed at the binary: run the thing
# a user runs and compare its whole output to the one line it may print.
#
# Two halves: the binary check above, and a source check for the siblings the
# binary check cannot reach (a "[DEBUG]" print on a path --version does not
# take): the only "[DEBUG]" literal src/ may hold is a logging macro's own
# prefix ("[DEBUG] " fmt), whose use is gated on --debug / CG_DISABLE_DEBUG_LOG.
#
#   bash scripts/check_quiet_version.sh [path/to/CadGoose]   (default build/CadGoose)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${1:-$ROOT/build/CadGoose}"

stray="$(grep -rn --include='*.cpp' --include='*.mm' --include='*.h' '\[DEBUG\]' "$ROOT/src" "$ROOT/include" \
    | grep -v '"\[DEBUG\] " fmt' || true)"
if [[ -n "$stray" ]]; then
    echo "✗ ungated [DEBUG] print in the shipped sources (use DEBUG_LOG, or LogWrite at debug level):" >&2
    printf '%s\n' "$stray" | sed 's/^/    /' >&2
    exit 1
fi

if [[ ! -x "$BIN" ]]; then
    echo "✗ check_quiet_version: $BIN is not an executable (build first)" >&2
    exit 2
fi

# A binary that fell through to the GUI would never return: bound the run.
rc=0
out="$(perl -e 'alarm 30; exec @ARGV' "$BIN" --version 2>&1 < /dev/null)" || rc=$?

if [[ $rc -ne 0 ]]; then
    echo "✗ $BIN --version exited $rc; output:" >&2
    printf '%s\n' "$out" | sed 's/^/    /' >&2
    exit 1
fi

if [[ ! "$out" =~ ^CadGoose\ v[0-9]+(\.[0-9]+)+[^[:space:]]*$ ]]; then
    echo "✗ $BIN --version must print exactly one line, 'CadGoose vX.Y', and nothing else; got:" >&2
    printf '%s\n' "$out" | sed 's/^/    /' >&2
    exit 1
fi

echo "✓ $BIN --version prints only its version line"
