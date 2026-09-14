#!/usr/bin/env python3
"""Shim: wires this repo's layout into the house test-registration gate.

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

# The house suite (github.com/ztomer/gates_of_heck, public) via GOH_DIR: CI
# checks it out beside the tree (see .github/workflows), a contributor clones
# it once. Until 2026-09-14 a pinned copy lived under tools/house_gates/ and
# had already drifted behind the house version's population floor.
GOH = os.environ.get("GOH_DIR") or os.path.expanduser("~/Projects/gates_of_heck")
CHECKER = os.path.join(GOH, "checks", "check_tests_registered.py")
if not os.path.isfile(CHECKER):
    print(f"✗ check_test_registration: house checker not found at {CHECKER}\n"
          "  git clone https://github.com/ztomer/gates_of_heck && export GOH_DIR=$PWD/gates_of_heck",
          file=sys.stderr)
    sys.exit(2)

# --min-tests: the population floor (119 test files today). A gate that reports
# compliance over zero files is the defect check_empty_scope.py exists to catch.
rc = subprocess.call([
    sys.executable, CHECKER,
    "--buildsystem", "cmake",
    "--tests-dir", "tests",
    "--makefile", "CMakeLists.txt",
    "--min-tests", "20",
])
if rc != 0:
    sys.exit(rc)
