#!/bin/bash
# Runs the test binary under the CI filter and reports its real exit status.
#
# HISTORY — do not re-add the crash swallow.
# This script used to treat exit codes 139 (SIGSEGV), 134 (SIGABRT) and 138
# (SIGBUS) as SUCCESS whenever "[  PASSED  ]" appeared, on the theory that
# "GoogleTest has a bug where complex filters crash in tear-down". That was
# wrong. The binary was corrupting its own heap, and because the crash landed
# after the last test printed, the swallow made it invisible for months.
#
# The actual causes, found by building with -DUSE_MIMALLOC=OFF (the system
# allocator traps corruption immediately, making it deterministic, where
# mimalloc limped on and died later at exit):
#   - a use-after-free in BehaviorRegistry::TickAll
#   - a double free in test_headless_rendering.mm (ItemData freed by both the
#     owning DroppedItemActor and the test)
#   - a use-after-free READ in test_actor_manager.cpp's own assertion
#
# A crash is a failure. If this script starts failing, fix the crash — do not
# restore the swallow. Reproduce with a -DUSE_MIMALLOC=OFF build.

set -euo pipefail

TEST_BINARY="./CadGooseTests"
# Use the coverage script filter (all positive patterns) which works correctly
FILTER="-MCPIntegrationTest*:LocalLLMTest*:AccessibilityGUITest*:DraggingIntegration*:WindowTrailTest*:BehaviorToggles.ToysBehaviorRegistered:PortalCleanup.BehaviorHasCleanupFunction:StalinHonk.*"

echo "Running tests with filter: $FILTER"
echo "Test binary: $TEST_BINARY"

# Run the test binary and capture output
set +e
OUTPUT=$("$TEST_BINARY" --gtest_filter="$FILTER" 2>&1)
EXIT_CODE=$?
set -e

echo "$OUTPUT"

if echo "$OUTPUT" | grep -q "\[  PASSED  \]"; then
    PASSED_COUNT=$(echo "$OUTPUT" | grep "\[  PASSED  \]" | sed -E 's/.*\[  PASSED  \] ([0-9]+) tests?.*/\1/' | tail -1)
    echo "Tests passed: $PASSED_COUNT"
fi

if [[ $EXIT_CODE -eq 0 ]]; then
    exit 0
fi

# A signal exit means the binary died even if every test reported OK — almost
# always heap corruption surfacing after the last test printed. Report it.
if [[ $EXIT_CODE -gt 128 ]]; then
    echo "FAILED: test binary died with signal $((EXIT_CODE - 128)) (exit $EXIT_CODE)."
    echo "        Tests reporting OK does NOT make this benign — the process still crashed."
    echo "        Reproduce deterministically with a -DUSE_MIMALLOC=OFF build."
else
    echo "FAILED: tests failed (exit $EXIT_CODE)"
fi
exit $EXIT_CODE