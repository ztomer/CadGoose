#!/usr/bin/env bash
# run_tests_tsan.sh — run the ThreadSanitizer suite (ROADMAP M1.2) from the
# build directory. halt_on_error turns any reported data race into a nonzero
# exit, so a race is red, not a warning in a log nobody reads.
set -euo pipefail

TEST_BINARY="./CadGooseTsanTests"
if [[ ! -x "$TEST_BINARY" ]]; then
    echo "FAILED: $TEST_BINARY not built (run from the build directory)." >&2
    exit 2
fi
TSAN_OPTIONS="halt_on_error=1 second_deadlock_stack=1" "$TEST_BINARY" "$@" < /dev/null
