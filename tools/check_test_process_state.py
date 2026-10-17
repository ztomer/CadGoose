#!/usr/bin/env python3
"""No test mutates process-global state that other threads read.

ROADMAP G0.12: a test run left `e-home-unset-test/` (a config.toml and
themes/) in the repo root. ConfigPath tests used setenv/unsetenv on HOME and
CADGOOSE_CONFIG_DIR and chdir'd mid-suite. The environment and the cwd are
shared by every thread in the binary (the MCP server, the log thread, GCD
workers), setenv/unsetenv are not thread-safe, and a getenv pointer read in
the middle of one can dangle: the leaked name is the tail of the test's own
"/tmp/cadgoose-home-unset-test" read through such a pointer, a relative path
that resolved against the repo root.

The fix removed the cause: ConfigDirPath() is a pure ResolveConfigDir over
ConfigDirSources, overridden in-process by ScopedConfigDirOverride, and
Log_InitLevel's env read is the pure Log_LevelFor. This gate keeps the cause
removed: no setenv/unsetenv/putenv/clearenv, no chdir/fchdir, no
std::filesystem::current_path(<new path>) anywhere under tests/.

ALLOWED is a ratchet. Each entry names a file and why it may stand; an entry
whose file no longer contains a call is STALE and red, so the list can only
shrink. Floor: a scan that reads fewer than MIN_FILES test sources lost its
subject and is red, not clean.
"""

import pathlib
import re
import subprocess
import sys

# 130 test sources under tests/ on 2026-10-17; the floor catches blindness.
MIN_FILES = 100

ALLOWED = {
    "tests/test_main.cpp": "main() installs the CADGOOSE_CONFIG_DIR sandbox and "
    "CADGOOSE_HEADLESS_TEST before any test or thread runs",
}

SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".m", ".mm"}

# current_path() and current_path(ec) only read; any other argument is chdir.
# The read form's error_code must be spelled `ec` (the house convention), so
# the rule stays a regex and not a C++ parser.
PATTERN = re.compile(
    r"\b(?:setenv|unsetenv|putenv|clearenv|chdir|fchdir)\s*\("
    r"|\bcurrent_path\s*\(\s*(?!ec\s*\))[^)\s]"
)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def tracked_test_sources(root: pathlib.Path) -> list:
    out = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "tests"],
        cwd=root, capture_output=True, text=True, check=True, stdin=subprocess.DEVNULL,
    ).stdout.split()
    return sorted(p for p in out if pathlib.PurePath(p).suffix in SUFFIXES)


def scan(root: pathlib.Path, files: list) -> dict:
    hits = {}
    for rel in files:
        text = strip_comments((root / rel).read_text(encoding="utf-8", errors="replace"))
        for lineno, line in enumerate(text.splitlines(), 1):
            if PATTERN.search(line):
                hits.setdefault(rel, []).append(lineno)
    return hits


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    files = tracked_test_sources(root)
    if len(files) < MIN_FILES:
        print(f"✗ [test_process_state] read {len(files)} test sources, floor {MIN_FILES}: "
              "the scan lost its subject", file=sys.stderr)
        return 1
    hits = scan(root, files)
    bad = 0
    for rel, lines in sorted(hits.items()):
        if rel in ALLOWED:
            continue
        bad += 1
        print(f"✗ [test_process_state] {rel}:{','.join(map(str, lines))}: a test mutates "
              "the process environment or cwd, which every thread shares. Use "
              "ResolveConfigDir/ScopedConfigDirOverride or a pure function of the value.",
              file=sys.stderr)
    for rel in sorted(ALLOWED):
        if rel not in hits:
            bad += 1
            print(f"✗ [test_process_state] stale allowlist entry {rel}: it no longer "
                  "mutates process state; delete the entry", file=sys.stderr)
    if bad:
        return 1
    print(f"✓ [test_process_state] {len(files)} test sources, no process-state mutation "
          f"outside {len(ALLOWED)} allowlisted file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
