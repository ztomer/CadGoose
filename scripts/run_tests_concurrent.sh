#!/bin/bash
# Runs two copies of the CI test run AT ONCE, from the build directory, and
# fails if either fails (ROADMAP G0.3).
#
# Tests that share a machine-wide resource (a fixed TCP port, a fixed unix
# socket path, a fixed /tmp sandbox) pass alone and collide in company; the
# MCP suites bound port 31073 and /tmp/desktop-goose-mcp.sock, and the config
# sandbox was one fixed /tmp directory. tools/check_test_endpoints.py catches
# the spellings it knows; this catches the class by running it.
#
#   cd build && bash ../scripts/run_tests_concurrent.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="$(mktemp -d "${TMPDIR:-/tmp}/cadgoose-concurrent-XXXXXX")"

bash "$SCRIPT_DIR/run_tests_ci.sh" > "$LOG_DIR/a.log" 2>&1 < /dev/null &
pid_a=$!
bash "$SCRIPT_DIR/run_tests_ci.sh" > "$LOG_DIR/b.log" 2>&1 < /dev/null &
pid_b=$!

rc_a=0
rc_b=0
wait "$pid_a" || rc_a=$?
wait "$pid_b" || rc_b=$?

if [[ $rc_a -ne 0 || $rc_b -ne 0 ]]; then
    for copy in a b; do
        echo "--- copy $copy (tail) ---"
        tail -n 40 "$LOG_DIR/$copy.log"
    done
    echo "✗ concurrent test copies failed (exit $rc_a / $rc_b); logs in $LOG_DIR"
    exit 1
fi

grep -h "Tests passed:" "$LOG_DIR/a.log" "$LOG_DIR/b.log"
echo "✓ two concurrent copies of the CI test run passed"
rm -r "$LOG_DIR"
