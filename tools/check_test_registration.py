#!/usr/bin/env python3
"""Shim: wires this repo's layout into the VENDORED test-registration gate.

Wired to this repo's layout (cmake, tests/ at the root, CMakeLists.txt).
The shared checker only accepts registration named INSIDE an
add_executable/add_test block — stricter than the ancestor here, which
matched a path anywhere in the file (a stale comment could stand in for a
real target). This repo's CMakeLists lists its test sources explicitly, so
the strict mode passes; keep it that way.
"""

import os
import subprocess
import sys

# tools/house_gates/, not GOH_DIR: this must run in CI and in a fork, neither of which can reach
# the author's private suite. Requiring GOH_DIR here is what failed the release workflow.
CHECKER = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "house_gates", "check_tests_registered.py")
if not os.path.isfile(CHECKER):
    print(f"✗ check_test_registration: vendored checker missing at {CHECKER}", file=sys.stderr)
    sys.exit(2)

sys.exit(subprocess.call([
    sys.executable, CHECKER,
    "--buildsystem", "cmake",
    "--tests-dir", "tests",
    "--makefile", "CMakeLists.txt",
]))
