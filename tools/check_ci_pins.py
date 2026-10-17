#!/usr/bin/env python3
"""CI runs on named releases, never on moving references.

ROADMAP G0.6: the workflows used actions/checkout@v4, actions/cache@v4 and
actions/upload-artifact@v4 (three majors behind on 2026-10-17: v7.0.1,
v6.1.0, v7.0.2) and `runs-on: ubuntu-latest`. A major tag (`@v4`) and a
`-latest` label are moving references: the maintainer decides what they
mean, and nobody notices when the newest release has moved on. This
refuses, in every workflow and composite action under .github/:

  - a `uses:` of a remote action not pinned to an exact release tag
    (vX.Y.Z) or a full 40-hex commit sha;
  - a `runs-on:` label containing `latest`.

Whether each pin is the NEWEST release is a network question this gate does
not ask; bump the pins deliberately (house rule: latest means the newest
release, moved in its own commit). Floor: at least one `uses:` must be read.
"""

import pathlib
import re
import sys

USES = re.compile(r"^\s*-?\s*uses:\s*([^\s#]+)", re.M)
RUNS_ON = re.compile(r"^\s*runs-on:\s*([^\n#]+)", re.M)
EXACT = re.compile(r"@(v\d+\.\d+\.\d+|[0-9a-f]{40})$")


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    files = sorted(list((root / ".github").rglob("*.yml")) + list((root / ".github").rglob("*.yaml")))
    seen = 0
    bad = 0
    for f in files:
        rel = f.relative_to(root)
        text = f.read_text(encoding="utf-8")
        for m in USES.finditer(text):
            ref = m.group(1).strip("'\"")
            if ref.startswith("./") or ref.startswith("docker://"):
                continue
            seen += 1
            if not EXACT.search(ref):
                bad += 1
                print(f"✗ [ci_pins] {rel}: `uses: {ref}` is a moving reference; pin an exact "
                      "release tag (vX.Y.Z) or a commit sha", file=sys.stderr)
        for m in RUNS_ON.finditer(text):
            label = m.group(1).strip()
            if "latest" in label:
                bad += 1
                print(f"✗ [ci_pins] {rel}: `runs-on: {label}` moves under you; name the "
                      "runner image (e.g. ubuntu-26.04)", file=sys.stderr)
    if seen == 0:
        print("✗ [ci_pins] read no `uses:`: the scan lost its subject", file=sys.stderr)
        return 1
    if bad:
        return 1
    print(f"✓ [ci_pins] {seen} action references pinned to releases, no -latest runner")
    return 0


if __name__ == "__main__":
    sys.exit(main())
