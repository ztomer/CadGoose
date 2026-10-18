#!/bin/bash
# The CI test run in three random orders (ROADMAP G0.13): a test that passes
# only after (or before) some other test is a red build, not a latent hazard.
#
# The fixed order hid exactly that: at e1a197c all 20 --gtest_shuffle seeds
# failed, 10 of them by SIGSEGV/SIGBUS, while the fixed order was green. The
# cure is tests/global_state_reset.h, which restores the shared state after
# every test; this is what goes red when a new piece of shared state misses
# it.
#
# A fresh seed each run explores new orders; it is printed first, and the
# same seed replays the same three orders.
#
#   cd build && bash ../scripts/run_tests_shuffled.sh [seed]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# gtest takes seeds in [1, 99999]; --gtest_repeat advances it per iteration.
SEED="${1:-$(( ((RANDOM << 15) | RANDOM) % 99999 + 1 ))}"
if ! [[ "$SEED" =~ ^[0-9]+$ ]] || (( SEED < 1 || SEED > 99999 )); then
    echo "✗ seed must be an integer in [1, 99999], got: $SEED" >&2
    exit 2
fi

echo "→ shuffle seed $SEED (replay: cd build && bash ../scripts/run_tests_shuffled.sh $SEED)"
rc=0
bash "$SCRIPT_DIR/run_tests_ci.sh" --gtest_shuffle --gtest_repeat=3 --gtest_random_seed="$SEED" || rc=$?
if [[ $rc -ne 0 ]]; then
    echo "✗ the shuffled run failed with seed $SEED; replay it with the command above"
    exit "$rc"
fi
echo "✓ three shuffled orders passed (seed $SEED)"
