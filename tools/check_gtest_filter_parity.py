#!/usr/bin/env python3
"""CI gtest filter parity — the filter is written ONCE or the gate is red.

Takes over from ROADMAP G0.2: the `--gtest_filter` used to be copy-pasted
between scripts/run_tests_ci.sh, cmake/Tests.cmake and run_coverage.sh, and
the copies drifted (run_coverage.sh lost the three fixed exclusions; the
three fixed exclusion groups sat excluded for months with nobody checking a
copy against reality). This check holds the collapse in place:

  1. The single source tests/gtest_ci_filter.txt must exist, yield a filter,
     and document a reason line for every exclusion it names.
  2. Every sanctioned call site must still consume that file — deleting the
     consumption (a "deleted copy") is red.
  3. No second hardcoded --gtest_filter VALUE may exist in tracked code — a
     copy written to diverge is caught before it can drift.

Floor (house rule, cf. the GOH_*_MIN floors): scanners that read repo files
must refuse to pass over what they can't see. A tracked corpus smaller than
MIN_TRACKED_FILES below means the scan lost its subject and reports "clean"
over nothing — that is red, not compliant.
"""

import os
import re
import subprocess
import sys

# Refuses an empty tree. 399 code files today (git ls-files, 2026-10-17);
# the floor catches only total blindness, as check_test_registration's
# --min-tests does for its subject.
MIN_TRACKED_FILES = 100

# The one place the filter value may be written.
SOURCE = "tests/gtest_ci_filter.txt"

# call site -> how its consumption shows up (grep fragments that identify
# "this file reads the filter source"). Each entry must name the fragment.
CONSUMERS = {
    "scripts/run_tests_ci.sh": ["tests/gtest_ci_filter.txt"],
    "cmake/Tests.cmake": ["tests/gtest_ci_filter.txt", "${CG_TEST_GTEST_FILTER}"],
    "run_coverage.sh": ["tests/gtest_ci_filter.txt"],
}

# Files matching these extensions/name scan for second hardcoded values.
# Prose (.md) is not executable and is allowed to quote the flag, but never
# a full value — README had its own divergent copy, which was rewritten to
# cite the source instead.
SCAN_SUFFIXES = {".sh", ".cmake", ".py", ".cpp", ".cc", ".c", ".h", ".hpp", ".m", ".mm", ".yml", ".yaml", ".toml"}
SCAN_EXACT = {"CMakeLists.txt"}

# Value-bearing only: RHS with any '$' or '{' interpolation is a variable
# consumption (`"$FILTER"`, `${CG_TEST_GTEST_FILTER}`, `"*${suite}*"`);
# a constant non-empty value is a hardcoded copy.
GTEST_FILTER_LIT = re.compile(r"--gtest_filter\s*=\s*(\"[^\"]*\"|'[^']*'|[^$\s;'\",]+)")


def fail(msg: str) -> int:
    print(f"✗ [gtest_filter_parity] {msg}", file=sys.stderr)
    return 1


def tracked(root: str) -> list[str]:
    out = subprocess.run(
        ["git", "ls-files"], cwd=root, capture_output=True, text=True, check=False,
        stdin=subprocess.DEVNULL,
    )
    if out.returncode != 0:
        return []
    return [ln.strip() for ln in out.stdout.splitlines() if ln.strip()]


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    files = tracked(root)
    if len(files) < MIN_TRACKED_FILES:
        return fail(
            f"scanned {len(files)} tracked file(s), expected at least "
            f"{MIN_TRACKED_FILES} — 'compliant' over a tree it cannot see "
            f"is the empty-scope pass this floor exists to refuse."
        )

    # 1. The single source exists and yields a non-empty filter.
    src_path = os.path.join(root, *SOURCE.split("/"))
    if not os.path.isfile(src_path):
        return fail(f"the filter source {SOURCE} is gone — nothing defines the CI filter.")
    reason_lines, filter_lines = [], []
    for line in open(src_path, encoding="utf-8"):
        line = line.rstrip("\n")
        if line.strip().startswith("#"):
            if line.lstrip("# ").startswith("-"):
                reason_lines.append(line)
        elif line.strip():
            filter_lines.append(line.strip())
    if not filter_lines:
        return fail(f"{SOURCE} yields an empty filter (comments only?) — the CI suite would run blindly.")
    if len(filter_lines) > 1:
        return fail(
            f"{SOURCE} carries {len(filter_lines)} filter line(s) — one list, one "
            f"line: consumers join lines with ':' and this gtest treats a leading "
            f"'-' as negating every token of the joined list (verified empirically "
            f"2026-10-17, googletest 1.18). Splitting one exclusion onto its own "
            f"line looks like tidiness and quietly changes what runs."
        )

    # 1b. One reason per remaining exclusion: every exclusion token in the
    #     filter must be named on at least one documented reason line.
    tokens = [t for t in filter_lines[0].split(":") if t.strip()]
    # gtest semantics (verified 2026-10-17, gtest 1.18, on this binary): when
    # the filter's first token starts with '-', EVERY token is an exclusion —
    # "-A*:B*" excludes A* AND B*; it does NOT mean "exclude A*, run B*".
    # Without a leading '-', only tokens written with their own '-' exclude.
    if tokens[0].startswith("-"):
        excl_names = [t.lstrip("-") for t in tokens]
    else:
        excl_names = [t[1:] for t in tokens if t.startswith("-")]
    negatives = sorted({n.strip("*") for n in excl_names if n.strip()})
    for name in negatives:
        if not any(name in ln for ln in reason_lines):
            return fail(
                f"exclusion {name} in {SOURCE} has no documented reason line "
                f"(comment lines naming it start with '-' inside the header)."
            )

    # 2. Every call site still consumes the source. A deleted consumption is
    #    the "one copy is deleted" end of the defect the roadmap names.
    for target, frags in CONSUMERS.items():
        path = os.path.join(root, *target.split("/"))
        if not os.path.isfile(path):
            return fail(f"call site {target} is gone — {SOURCE} would have no (or fewer) consumers.")
        text = open(path, encoding="utf-8").read()
        for frag in frags:
            if frag not in text:
                return fail(
                    f"call site {target} no longer references {frag!r} — it has "
                    f"stopped or was never made to consume the single filter source {SOURCE}."
                )

    # 3. No second hardcoded value in tracked code. Parameterized uses
    #    (`"$FILTER"`, `${CG_TEST_GTEST_FILTER}`, `"*${suite}*"`) contain '$'
    #    and are excluded by the regex itself.
    scan = sorted(
        f for f in files
        if os.path.splitext(f)[1] in SCAN_SUFFIXES or os.path.basename(f) in SCAN_EXACT
    )
    hardcoded: dict[tuple[str, int], str] = {}
    for rel in scan:
        path = os.path.join(root, rel.replace("/", os.sep))
        try:
            with open(path, encoding="utf-8", errors="replace") as fh:
                for i, line in enumerate(fh, 1):
                    m = GTEST_FILTER_LIT.search(line)
                    if m:
                        val = m.group(1).strip("\"'").strip()
                        if val and "$" not in val and "{" not in val:
                            hardcoded[(rel, i)] = val
        except OSError:
            continue
    if hardcoded:
        sites = ", ".join(f"{rel}:{ln}={val!r}" for (rel, ln), val in sorted(hardcoded.items()))
        return fail(
            f"second hardcoded gtest filter value(s) in tracked code — every "
            f"call site must consume {SOURCE}: {sites}"
        )

    print(
        f"→ [gtest_filter_parity] OK — filter defined once in {SOURCE} "
        f"({len(negatives)} documented exclusion(s): {', '.join(negatives) or 'none'}), "
        f"{len(CONSUMERS)} call site(s) consume it, {len(scan)} tracked code file(s) scanned."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
