#!/usr/bin/env python3
"""Every gate CI runs, the local pre-push gate runs too.

ROADMAP G0.16: the G0.13 push passed all 12 local steps and went red in CI's
Coverage Gate (P0 95.21 -> 94.86, P1 26.63 -> 23.81, eligible total 64.03 ->
62.56). The coverage gate was in CI and not in GOH_CI_STEPS: tools/local_ci.sh
ran it only under a --coverage flag the pre-push hook never passed, and said
so in a warning line nobody reads. A local gate weaker than CI is a gate that
lets you push red (house engineering rule #17).

This reads every `run:` command in .github/workflows/ and every step of
.gatesrc's GOH_CI_STEPS, and collects the repo scripts each one invokes
(`scripts/...sh|py`, `tools/...sh|py`, matched after any `../`). A script CI
runs that no local step runs is red, unless EXEMPT names it with the reason it
cannot run before a push. EXEMPT is a ratchet: an entry CI no longer runs, or
that the local gate now runs, is red too. Floor: a scan that finds fewer than
MIN_CI_SCRIPTS in CI has lost its subject.
"""

import pathlib
import re
import shlex
import sys

MIN_CI_SCRIPTS = 9  # 10 in .github/workflows/ on 2026-10-18

EXEMPT = {
    "scripts/create_bundle.sh": "packaging, not a gate: it assembles the release .app "
    "that only CI publishes; nothing it checks is a property of the pushed commit",
}

SCRIPT = re.compile(r"(?:\.\./)*((?:scripts|tools)/[\w./-]+\.(?:sh|py))\b")
RUN_INLINE = re.compile(r"^(\s*)-?\s*run:\s*(\S.*)$")
RUN_BLOCK = re.compile(r"^(\s*)-?\s*run:\s*[|>][-+]?\s*$")


def ci_commands(text: str) -> list[str]:
    """The text of every `run:` in one workflow, inline or block scalar."""
    lines = text.splitlines()
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        block = RUN_BLOCK.match(line)
        inline = RUN_INLINE.match(line)
        if block:
            indent = len(block.group(1))
            body = []
            i += 1
            while i < len(lines) and (not lines[i].strip() or
                                      len(lines[i]) - len(lines[i].lstrip()) > indent):
                body.append(lines[i])
                i += 1
            out.append("\n".join(body))
            continue
        if inline:
            out.append(inline.group(2))
        i += 1
    return out


def local_steps(gatesrc: str) -> list[str]:
    """GOH_CI_STEPS, split as the shared runner splits it (on ':')."""
    for line in gatesrc.splitlines():
        if line.startswith("GOH_CI_STEPS="):
            value = shlex.split(line.split("=", 1)[1], comments=True)
            return value[0].split(":") if value else []
    return []


def scripts_in(commands: list[str]) -> set[str]:
    found = set()
    for cmd in commands:
        found.update(m.group(1) for m in SCRIPT.finditer(cmd))
    return found


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    workflows = sorted((root / ".github" / "workflows").glob("*.y*ml"))
    ci = set()
    for wf in workflows:
        ci |= scripts_in(ci_commands(wf.read_text(encoding="utf-8")))
    local = scripts_in(local_steps((root / ".gatesrc").read_text(encoding="utf-8")))

    if len(ci) < MIN_CI_SCRIPTS:
        print(f"✗ [ci_local_parity] found {len(ci)} scripts in CI's run: steps, floor "
              f"{MIN_CI_SCRIPTS}: the scan lost its subject", file=sys.stderr)
        return 1
    if not local:
        print("✗ [ci_local_parity] no GOH_CI_STEPS in .gatesrc", file=sys.stderr)
        return 1

    bad = 0
    for script in sorted(ci - local):
        if script in EXEMPT:
            continue
        bad += 1
        print(f"✗ [ci_local_parity] CI runs {script} and the local pre-push gate does not: "
              "add it to GOH_CI_STEPS in .gatesrc, or a push can pass here and fail there",
              file=sys.stderr)
    for script in sorted(EXEMPT):
        if script not in ci:
            bad += 1
            print(f"✗ [ci_local_parity] stale exemption {script}: CI no longer runs it",
                  file=sys.stderr)
        elif script in local:
            bad += 1
            print(f"✗ [ci_local_parity] stale exemption {script}: the local gate runs it now",
                  file=sys.stderr)
    if bad:
        return 1
    print(f"✓ [ci_local_parity] {len(ci)} scripts in CI, every gate among them runs locally "
          f"({len(EXEMPT)} exempt with a reason)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
