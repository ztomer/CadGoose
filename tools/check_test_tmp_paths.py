#!/usr/bin/env python3
"""No test names a literal /tmp/ path.

CG.TMPRACE (2026-11-02): the pre-push gate runs copies of the test binaries
concurrently, and test_renderer.mm wrote and OCRed /tmp/test_text_render.png,
so one copy read the other's image (test_renderer.mm:142 red in copy A, green
in copy B, twice). Its siblings shared /tmp/multi_goose_test,
/tmp/trail_test_frames and /tmp/soak_fetch_test the same way.

Every scratch path now comes from the one per-process directory in
tests/test_scratch.h (TestScratchDir / TestScratchPath / TestScratchMkdtemp);
a shell test derives its own from ${TMPDIR:-/tmp} with mktemp. So no code line
under tests/ may contain "/tmp/" at all -- not even a mkdtemp template, whose
base would bypass the helper. There is no allowlist; the floor refuses a blind
scan.
"""

import pathlib
import re
import subprocess
import sys

MIN_FILES = 135  # 150 test sources on 2026-11-07 (nine tenths)

C_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".m", ".mm"}
HASH_SUFFIXES = {".sh", ".py"}
RULE = re.compile(r"/tmp/")


def strip_c_comments(text: str) -> str:
    text = re.sub(
        r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S
    )
    return re.sub(r"(?<!:)//[^\n]*", "", text)


def strip_hash_comments(text: str) -> str:
    return re.sub(r"(?m)^[ \t]*#[^\n]*", "", text)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    files = sorted(
        p
        for p in subprocess.run(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "tests"],
            cwd=root,
            capture_output=True,
            text=True,
            check=True,
            stdin=subprocess.DEVNULL,
        ).stdout.split()
        if pathlib.PurePath(p).suffix in C_SUFFIXES | HASH_SUFFIXES
    )
    if len(files) < MIN_FILES:
        print(
            f"✗ [test_tmp_paths] read {len(files)} test sources, floor {MIN_FILES}: "
            "the scan lost its subject",
            file=sys.stderr,
        )
        return 1
    bad = 0
    for rel in files:
        raw = (root / rel).read_text(encoding="utf-8", errors="replace")
        suffix = pathlib.PurePath(rel).suffix
        text = (
            strip_c_comments(raw) if suffix in C_SUFFIXES else strip_hash_comments(raw)
        )
        for lineno, line in enumerate(text.splitlines(), 1):
            if RULE.search(line):
                bad += 1
                print(
                    f"✗ [test_tmp_paths] {rel}:{lineno}: a literal /tmp/ path is shared by "
                    "every concurrent test copy; use TestScratchPath() (tests/test_scratch.h) "
                    "or mktemp under ${TMPDIR:-/tmp}",
                    file=sys.stderr,
                )
    if bad:
        return 1
    print(f"✓ [test_tmp_paths] {len(files)} test sources, no literal /tmp/ path")
    return 0


if __name__ == "__main__":
    sys.exit(main())
