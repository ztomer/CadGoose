#!/usr/bin/env bash
# local_ci.sh — run the gates both workflows enforce, here, before pushing.
#
# UNLIKE most sibling repos, CI here is ALIVE and stays that way. CadGoose is a
# PUBLIC repo, so GitHub Actions minutes are free and its runs succeed — the
# credit exhaustion that killed CI on the private repos never applied. More than
# that, build_and_release.yml is the RELEASE PIPELINE: it builds the DMG,
# publishes the release, and scripts/push_release.sh polls for that run before
# updating the homebrew tap. Disabling it would not tidy anything, it would stop
# releases working.
#
# So this script is not a replacement for CI. It is the fast pre-push copy of
# CI's gates, so a red build is found in minutes on this machine instead of
# after a push. CI remains the authority.
#
#   ./tools/local_ci.sh              gates + build + tests
#   ./tools/local_ci.sh --coverage   also the coverage gate (slow: separate build)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# House Kare style, canonical copy in house-gates. scripts/verify_ci.sh
# predates this and hand-rolls its own ANSI colours; new scripts source the lib.
GOH="${GOH_DIR:-$GOH_DIR}"
# shellcheck source=$GOH_DIR/tui/lib.sh
source "$GOH/tui/lib.sh"

WITH_COVERAGE=0
for a in "$@"; do
    case "$a" in
        --coverage) WITH_COVERAGE=1 ;;
        -h|--help)  echo "usage: ./tools/local_ci.sh [--coverage]"; exit 0 ;;
        *)          err "unknown argument: $a"; exit 2 ;;
    esac
done

fail=0
log="$(mktemp -t cadgoose-ci)"
trap 'rm -f "$log"' EXIT

run() {  # run <label> <cmd...> — on failure, SHOW the output
    local label="$1"; shift
    info "$label"
    if "$@" >"$log" 2>&1; then
        ok "$label"
    else
        tail -40 "$log" >&2
        err "$label"
        fail=1
    fi
}

section "structural gates"
run "no-emoji gate"          python3 "${GOH_DIR:-$GOH_DIR}/checks/check_no_emoji.py"
run "test registration gate" python3 tools/check_test_registration.py

section "build"
# CI configures from scratch (`rm -rf build`) so a stale cache cannot hide a
# broken CMakeLists. Reproduced: an incremental configure is exactly how a
# missing target survives locally and fails on the runner.
run "cmake configure (clean)" bash -c 'rm -rf build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release'
run "cmake build" cmake --build build --config Release --parallel "$(sysctl -n hw.ncpu)"

section "tests"
# From INSIDE build/, exactly as CI does it (`cd build` then
# `bash ../scripts/run_tests_ci.sh`). The script resolves the test binary as
# ./CadGooseTests, so running it from the repo root fails with exit 127 and a
# bare "No such file or directory" — which reads like a broken build rather
# than a wrong working directory.
run "test suite" bash -c 'cd build && bash ../scripts/run_tests_ci.sh' 

if [ "$WITH_COVERAGE" -eq 1 ]; then
    section "coverage"
    # Same thresholds as build_and_release.yml. They are per-tier, not one
    # number, so they must be passed explicitly or the gate silently uses
    # whatever the script's defaults happen to be.
    run "coverage gate (p0>=94 p1>=53 total>=85)" \
        bash scripts/check_coverage.sh --p0-min=94 --p1-min=53 --total-min=85 --build-dir=build-cov
else
    warn "coverage gate skipped — pass --coverage (it needs its own instrumented build)"
fi

if [ "$fail" -eq 0 ]; then
    section "result"
    ok "all local gates passed — CI remains the authority"
else
    section "result"
    err "failures above"
fi
exit "$fail"
