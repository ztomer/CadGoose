#!/usr/bin/env python3
"""Fail if any production source under src/ is in no coverage tier.

Reads scripts/coverage_eligible.txt. Every .cpp/.mm under src/ must be either
listed by a P0/P1 glob or named by an EXCLUDE glob (with a reason in the file).
"""
import fnmatch
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELIGIBLE = os.path.join(ROOT, "scripts", "coverage_eligible.txt")

tiered = set()
excluded = set()

with open(ELIGIBLE) as f:
    for raw in f:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        tier, globpat = line.split(":", 1)
        tier = tier.strip()
        globpat = globpat.strip()
        full = os.path.join(ROOT, globpat)
        # expand (non-recursive globs as the shell parser does)
        import glob as _glob
        for path in _glob.glob(full):
            rel = os.path.relpath(path, ROOT)
            if tier == "EXCLUDE":
                excluded.add(rel)
            else:
                tiered.add(rel)

# collect production sources under src/
prod = set()
for dirpath, _, files in os.walk(os.path.join(ROOT, "src")):
    for name in files:
        if name.endswith((".cpp", ".mm")):
            rel = os.path.relpath(os.path.join(dirpath, name), ROOT)
            prod.add(rel)

untiered = sorted(p for p in prod if p not in tiered and p not in excluded)

if untiered:
    print("UNTIERED production sources (not in any P0/P1 tier and not EXCLUDED):")
    for p in untiered:
        print(f"  {p}")
    print(f"\n{len(untiered)} untiered file(s). Add to a tier or EXCLUDE with a reason.")
    sys.exit(1)

print(f"OK: all {len(prod)} production sources under src/ are tiered or excluded.")
sys.exit(0)
