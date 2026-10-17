#!/usr/bin/env python3
"""A new source file is never silently untracked.

ROADMAP G0.9: .gitignore carried a blanket `*.md` with a whitelist, so every
new document was invisible to `git status` until someone remembered to add
an exception (ROADMAP.md itself needed one, and the list still named a
deleted docs/CLEANUP_PLAN.md). Generated files are ignored by name, next to
their writer (tools/profiling/.gitignore: soak_results_*.md); no rule may
hide a KIND of source.

This asks git itself, with --no-index so tracked look-alikes cannot answer,
whether a new file of each source kind in each source directory would be
ignored. Any yes is red. PROBES is the subject; an empty one is red too.
"""

import pathlib
import subprocess
import sys

# One hypothetical new file per (source directory, source kind) this repo has.
PROBES = [
    "G0_9_probe.md",
    "docs/G0_9_probe.md",
    "src/common/G0_9_probe.cpp",
    "src/common/G0_9_probe.mm",
    "src/platform/macos/G0_9_probe.mm",
    "src/platform/linux/G0_9_probe.cpp",
    "include/G0_9_probe.h",
    "tests/common/test_G0_9_probe.cpp",
    "tests/common/test_G0_9_probe.mm",
    "scripts/G0_9_probe.sh",
    "scripts/G0_9_probe.cmake",
    "cmake/G0_9_probe.cmake",
    "tools/G0_9_probe.py",
    "tools/profiling/G0_9_probe.md",
    "config/themes/G0_9_probe.toml",
    ".github/workflows/G0_9_probe.yml",
]


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    if not PROBES:
        print("✗ [new_sources_visible] no probes: the check has lost its subject", file=sys.stderr)
        return 1
    # check-ignore exits 0 when at least one path is ignored, 1 when none is.
    res = subprocess.run(
        ["git", "check-ignore", "--no-index", "--verbose", *PROBES],
        cwd=root, capture_output=True, text=True, stdin=subprocess.DEVNULL,
    )
    if res.returncode not in (0, 1):
        print(f"✗ [new_sources_visible] git check-ignore failed: {res.stderr.strip()}",
              file=sys.stderr)
        return 1
    hidden = [line for line in res.stdout.splitlines() if line.strip()]
    if hidden:
        for line in hidden:
            rule, path = line.split("\t", 1)
            print(f"✗ [new_sources_visible] a new {path} would be silently untracked "
                  f"(rule {rule}); ignore generated files by name, never a source kind",
                  file=sys.stderr)
        return 1
    print(f"✓ [new_sources_visible] {len(PROBES)} new-source probes, none ignored")
    return 0


if __name__ == "__main__":
    sys.exit(main())
