#!/usr/bin/env python3
"""Every fetched dependency is a hashed release archive, never a git clone.

ROADMAP G0.10: toml11 came in through FetchContent GIT_REPOSITORY + GIT_TAG,
which cloned its whole history and every submodule (nlohmann/json, doctest,
a Hugo theme): 368 MB and 78-86 s on every clean configure, including the
pre-push gate's, for a header-only library whose release tarball is 0.35 MB
(configure now takes 1.5 s). A git fetch is also pinned only by a movable
tag. This refuses, in every CMake file, a FetchContent_Declare that uses
GIT_REPOSITORY or lacks a URL with a URL_HASH SHA256. The floor: at least one
declaration must be found, or the scan lost its subject.
"""

import pathlib
import re
import subprocess
import sys

DECLARE = re.compile(r"FetchContent_Declare\s*\((.*?)\)", re.S)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    files = [
        p for p in subprocess.run(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard",
             "CMakeLists.txt", "*.cmake", "*/CMakeLists.txt"],
            cwd=root, capture_output=True, text=True, check=True, stdin=subprocess.DEVNULL,
        ).stdout.split()
    ]
    found = 0
    bad = 0
    for rel in files:
        text = re.sub(r"#[^\n]*", "", (root / rel).read_text(encoding="utf-8"))
        for m in DECLARE.finditer(text):
            found += 1
            body = m.group(1)
            name = body.split()[0] if body.split() else "?"
            if "GIT_REPOSITORY" in body:
                bad += 1
                print(f"✗ [fetchcontent] {rel}: {name} is a git clone; fetch the release "
                      "archive with URL + URL_HASH", file=sys.stderr)
            elif not re.search(r"\bURL\s+\S", body) or not re.search(r"URL_HASH\s+SHA256=[0-9a-f]{64}", body):
                bad += 1
                print(f"✗ [fetchcontent] {rel}: {name} has no URL pinned by URL_HASH SHA256",
                      file=sys.stderr)
    if found == 0:
        print("✗ [fetchcontent] no FetchContent_Declare found: the scan lost its subject",
              file=sys.stderr)
        return 1
    if bad:
        return 1
    print(f"✓ [fetchcontent] {found} dependency fetch(es), each a hashed archive")
    return 0


if __name__ == "__main__":
    sys.exit(main())
