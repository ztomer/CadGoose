"""The claim checks of check_macos_floor.py: pure text -> errors, no I/O.

Split out to keep check_macos_floor.py under the line cap. It names the very literals it
hunts, so check_macos_floor.py lists it in NOT_A_SECOND_SOURCE.
"""

import re

README = "README.md"

SECOND_SOURCE = [
    (
        re.compile(r"CMAKE_OSX_DEPLOYMENT_TARGET[\s=\"']+\d"),
        "CMAKE_OSX_DEPLOYMENT_TARGET literal",
    ),
    (
        re.compile(r"MACOSX_DEPLOYMENT_TARGET\s*=\s*[\"']?\d"),
        "MACOSX_DEPLOYMENT_TARGET literal",
    ),
    (re.compile(r"-m(?:macosx|macos)-version-min=\d"), "-mmacosx-version-min literal"),
    (re.compile(r"apple-macos(?:x)?\d"), "-target triple with a literal macOS version"),
    (
        re.compile(r"LSMinimumSystemVersion</key>\s*<string>\s*\d"),
        "literal LSMinimumSystemVersion",
    ),
]
README_ROW = re.compile(r"^\|\s*macOS\s+([\d.]+)\+?\s*\|", re.M)
OS_CLAIM = re.compile(r"\bmacOS\s+(\d+(?:\.\d+)*)\+")
# A Homebrew cask definition: the tap's, never a copy here (G0.18).
CASK_DEF = re.compile(r"^\s*cask\s+[\"']", re.M)


def version(text: str) -> tuple[int, ...]:
    """'26' and '26.0' are one version; trailing zeros are not significant."""
    parts = [int(p) for p in text.strip().split(".")]
    while len(parts) > 1 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def readme_errors(text: str, floor: str, where: str = README) -> list[str]:
    rows = README_ROW.findall(text)
    if len(rows) != 1:
        return [f"{where}: expected one '| macOS N+ |' platform row, found {len(rows)}"]
    if version(rows[0]) != version(floor):
        return [
            f"{where}: the platform row claims macOS {rows[0]}+, the floor is {floor}"
        ]
    return []


def claim_errors(text: str, floor: str, where: str) -> list[str]:
    """A feature may need a NEWER macOS than the floor; nothing runs on an older one."""
    return [
        f"{where}: claims macOS {m}+, below the {floor} floor"
        for m in OS_CLAIM.findall(text)
        if version(m) < version(floor)
    ]


def cask_copy_errors(text: str, where: str) -> list[str]:
    if CASK_DEF.search(text):
        return [
            f"{where}: a Homebrew cask in this tree; the cask is ztomer/homebrew-tap's "
            "Casks/cadgoose.rb, and its gate (tools/cask_floor.sh) holds it to this floor"
        ]
    return []


def second_source_errors(text: str, where: str) -> list[str]:
    out = []
    for pattern, what in SECOND_SOURCE:
        for m in pattern.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            out.append(
                f"{where}:{line}: {what}: the floor is written only in "
                "cmake/MacOSFloor.cmake; derive it"
            )
    return out
