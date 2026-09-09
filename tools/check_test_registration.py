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
import re
import subprocess
import sys

# Same population definition as the vendored checker below: a gate that
# reports compliance over zero files is the defect check_empty_scope.py
# exists to catch, and this repo always ships tests — an empty tests/
# means the tree moved, not that everything is registered.
TEST_FILE_RE = re.compile(r"^(test_.+|.+_test)\.(cpp|cc|cxx|c|m|mm|rs|py|swift)$")

# tools/house_gates/, not GOH_DIR: this must run in CI and in a fork, neither of which can reach
# the author's private suite. Requiring GOH_DIR here is what failed the release workflow.
CHECKER = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "house_gates", "check_tests_registered.py")
if not os.path.isfile(CHECKER):
    print(f"✗ check_test_registration: vendored checker missing at {CHECKER}", file=sys.stderr)
    sys.exit(2)

rc = subprocess.call([
    sys.executable, CHECKER,
    "--buildsystem", "cmake",
    "--tests-dir", "tests",
    "--makefile", "CMakeLists.txt",
])
if rc != 0:
    sys.exit(rc)

found = any(
    TEST_FILE_RE.match(name)
    for _, _, filenames in os.walk("tests")
    for name in filenames
)
if not found:
    print("✗ check_test_registration: no test files under tests/ — "
          "the tree moved or the convention changed; fix the paths, "
          "do not bless the empty set", file=sys.stderr)
    sys.exit(1)
