#!/usr/bin/env python3
"""Shim: the canonical test-registration gate lives in house-gates.

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

GOH = os.environ.get("GOH_DIR", os.path.expanduser("$GOH_DIR"))
CHECKER = os.path.join(GOH, "checks", "check_tests_registered.py")
if not os.path.isfile(CHECKER):
    print(f"✗ check_test_registration: canonical checker missing at {CHECKER} "
          "(is GOH_DIR right?)", file=sys.stderr)
    sys.exit(2)

sys.exit(subprocess.call([
    sys.executable, CHECKER,
    "--buildsystem", "cmake",
    "--tests-dir", "tests",
    "--makefile", "CMakeLists.txt",
]))
