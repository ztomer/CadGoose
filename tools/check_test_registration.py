#!/usr/bin/env python3
"""Gate: every test source under tests/ must be registered in CMakeLists.txt.

Two sibling test files (~1200 lines total) were silently dropped from the
build at some point and nobody noticed, because nothing compared the set of
files on disk against the set of files CMake actually compiles. This gate
closes that hole.

A test file is "registered" if its repo-relative path appears anywhere in
CMakeLists.txt (as a target source or in a comment naming a deliberate
exclusion with a reason).

Usage:
    python3 tools/check_test_registration.py [--makefile CMakeLists.txt]

Exit 0 if all registered, exit 1 listing any orphaned test files.
"""

import argparse
import re
import sys
from pathlib import Path

TEST_FILE_RE = re.compile(r"^(test_.+|.*_test)\.(cpp|cc|mm|m)$")

# Files deliberately not compiled, allowed only when named here WITH a reason.
# Keep this list empty if possible; every entry must state why the file exists
# on disk but not in any target.
ALLOWED_UNREGISTERED = {
    # example entry: "tests/foo/test_bar.cpp": "reason"
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--makefile", default="CMakeLists.txt")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    makefile = root / args.makefile
    if not makefile.is_file():
        print(f"ERROR: {makefile} not found", file=sys.stderr)
        return 1

    cmake_text = makefile.read_text(encoding="utf-8", errors="replace")

    orphans = []
    for path in sorted((root / "tests").rglob("*")):
        rel = path.relative_to(root).as_posix()
        if not path.is_file() or not TEST_FILE_RE.match(path.name):
            continue
        if rel in cmake_text:
            continue
        if rel in ALLOWED_UNREGISTERED:
            continue
        orphans.append(rel)

    if not orphans:
        print("OK: all test files under tests/ are registered in "
              f"{args.makefile}")
        return 0

    print(f"FAIL: {len(orphans)} test file(s) exist under tests/ but are NOT "
          f"registered in {args.makefile} — they compile into no target and "
          "never run:")
    for rel in orphans:
        note = ALLOWED_UNREGISTERED.get(rel)
        suffix = f"  [{note}]" if note else ""
        print(f"  {rel}{suffix}")
    print("\nEither add them to a target in CMakeLists.txt, fix whatever made "
          "them unbuildable, or delete them.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
