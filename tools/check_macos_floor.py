#!/usr/bin/env python3
"""The minimum macOS is built, not claimed (ROADMAP G0.14).

README said "macOS 10.15+" and the bundle's Info.plist said 10.15, but
nothing set a deployment target, so the binary's LC_BUILD_VERSION `minos`
was whatever SDK built it (27.0 on the author's Mac), the bundled Homebrew
libmimalloc carried the build host's OS, and the Swift object said 15. Owner ruling 2026-10-18: the floor is macOS 26, written in
cmake/MacOSFloor.cmake and nowhere else.

What is red:
  always      the README's platform row is not the floor; a README/docs
              "macOS N+" claim below it; the cask's `depends_on macos` is
              not the floor; any tracked file other than MacOSFloor.cmake
              writes a floor literal (a deployment-target flag, a
              -target ...-macosN triple, an LSMinimumSystemVersion string):
              a second source is how the claim and the build drifted apart.
  --bundle    Info.plist's LSMinimumSystemVersion is not the floor; ANY
              Mach-O anywhere in the .app (the executable, a bundled dylib,
              a helper under Resources) has a minos other than the floor, or
              none; the bundle's own executable is not among them (the walk
              lost its subject).
  --dmg       the same, for every .app at the root of the mounted DMG.
  --probe     calibration: builds fixture bundles with clang and vtool, plants
              each defect above once, and is red unless every plant goes red
              and the clean fixture stays green.

    python3 tools/check_macos_floor.py                      static checks
    python3 tools/check_macos_floor.py --bundle build/CadGoose.app
    python3 tools/check_macos_floor.py --dmg build/CadGoose-v1.2.dmg --probe
"""

import argparse
import contextlib
import pathlib
import plistlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
FLOOR_FILE = ROOT / "cmake" / "MacOSFloor.cmake"
README = "README.md"
CASK = "tools/homebrew/cadgoose.rb"
TAG = "[macos_floor]"

# Homebrew's macOS symbols, by major. A floor whose major is missing here is
# red until it is added, rather than letting the cask go unchecked.
CODENAMES = {14: "sonoma", 15: "sequoia", 26: "tahoe"}

# Prose that may name old floors: history, not a claim.
PROSE_HISTORY = {"docs/CHANGELOG.md", "ROADMAP.md"}
# The one source, and this file (whose probe plants the very literals it hunts).
NOT_A_SECOND_SOURCE = {"cmake/MacOSFloor.cmake", "tools/check_macos_floor.py"}

SECOND_SOURCE = [
    (re.compile(r"CMAKE_OSX_DEPLOYMENT_TARGET[\s=\"']+\d"), "CMAKE_OSX_DEPLOYMENT_TARGET literal"),
    (re.compile(r"MACOSX_DEPLOYMENT_TARGET\s*=\s*[\"']?\d"), "MACOSX_DEPLOYMENT_TARGET literal"),
    (re.compile(r"-m(?:macosx|macos)-version-min=\d"), "-mmacosx-version-min literal"),
    (re.compile(r"apple-macos(?:x)?\d"), "-target triple with a literal macOS version"),
    (re.compile(r"LSMinimumSystemVersion</key>\s*<string>\s*\d"), "literal LSMinimumSystemVersion"),
]
README_ROW = re.compile(r"^\|\s*macOS\s+([\d.]+)\+?\s*\|", re.M)
OS_CLAIM = re.compile(r"\bmacOS\s+(\d+(?:\.\d+)*)\+")
# Homebrew reads a bare symbol as a minimum; its ">= :name" string form is deprecated.
CASK_DEP = re.compile(r"depends_on\s+macos:\s*(\S+)")

MACHO_MAGIC = {
    b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xfe\xed\xfa\xce",
    b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf",
}


def version(text: str) -> tuple[int, ...]:
    """'26' and '26.0' are one version; trailing zeros are not significant."""
    parts = [int(p) for p in text.strip().split(".")]
    while len(parts) > 1 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def run(cmd: list[str], *, cwd: pathlib.Path | None = None,
        source: str | None = None) -> subprocess.CompletedProcess:
    """`source`, when given, is the child's stdin; otherwise it reads nothing."""
    if source is not None:
        return subprocess.run(cmd, cwd=cwd, input=source, capture_output=True, text=True,
                              check=False)
    return subprocess.run(cmd, cwd=cwd, stdin=subprocess.DEVNULL, capture_output=True,
                          text=True, check=False)


def read_floor() -> str:
    res = run(["cmake", "-P", str(FLOOR_FILE)])
    floor = res.stdout.strip()
    if res.returncode != 0 or not re.fullmatch(r"\d+(?:\.\d+)*", floor):
        sys.exit(f"✗ {TAG} cmake/MacOSFloor.cmake did not print a floor "
                 f"(exit {res.returncode}): {res.stdout!r} {res.stderr.strip()}")
    return floor


# ── static: the claims and the one source ──────────────────────────────────

def readme_errors(text: str, floor: str, where: str = README) -> list[str]:
    rows = README_ROW.findall(text)
    if len(rows) != 1:
        return [f"{where}: expected one '| macOS N+ |' platform row, found {len(rows)}"]
    if version(rows[0]) != version(floor):
        return [f"{where}: the platform row claims macOS {rows[0]}+, the floor is {floor}"]
    return []


def claim_errors(text: str, floor: str, where: str) -> list[str]:
    """A feature may need a NEWER macOS than the floor; nothing runs on an older one."""
    return [f"{where}: claims macOS {m}+, below the {floor} floor"
            for m in OS_CLAIM.findall(text) if version(m) < version(floor)]


def cask_errors(text: str, floor: str, where: str = CASK) -> list[str]:
    major = version(floor)[0]
    if major not in CODENAMES:
        return [f"{TAG} no Homebrew codename for macOS {major}: add it to CODENAMES"]
    found = CASK_DEP.findall(text)
    want = f":{CODENAMES[major]}"
    if found != [want]:
        return [f"{where}: needs exactly `depends_on macos: {want}`, has {found or 'none'}"]
    return []


def second_source_errors(text: str, where: str) -> list[str]:
    out = []
    for pattern, what in SECOND_SOURCE:
        for m in pattern.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            out.append(f"{where}:{line}: {what}: the floor is written only in "
                       "cmake/MacOSFloor.cmake; derive it")
    return out


def tracked_text_files() -> list[str]:
    res = run(["git", "ls-files", "--cached", "--others", "--exclude-standard"], cwd=ROOT)
    if res.returncode != 0:
        sys.exit(f"✗ {TAG} git ls-files failed: {res.stderr.strip()}")
    return [p for p in res.stdout.split("\n") if p and not p.startswith("Assets/")]


def static_errors(floor: str) -> tuple[list[str], int]:
    errors = readme_errors((ROOT / README).read_text(encoding="utf-8"), floor)
    errors += cask_errors((ROOT / CASK).read_text(encoding="utf-8"), floor)
    scanned = 0
    for rel in tracked_text_files():
        path = ROOT / rel
        if rel in NOT_A_SECOND_SOURCE or not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        scanned += 1
        if rel.endswith(".md") and rel not in PROSE_HISTORY:
            errors += claim_errors(text, floor, rel)
        if rel not in PROSE_HISTORY:
            errors += second_source_errors(text, rel)
    return errors, scanned


# ── built: every Mach-O in the bundle ──────────────────────────────────────

def is_macho(path: pathlib.Path) -> bool:
    try:
        with path.open("rb") as f:
            return f.read(4) in MACHO_MAGIC
    except OSError:
        return False


def build_versions(path: pathlib.Path) -> list[tuple[str, str]] | None:
    """(platform, minos) of every slice, or None if otool says it is not Mach-O."""
    res = run(["otool", "-arch", "all", "-l", str(path)])
    if res.returncode != 0 or "is not an object file" in res.stdout + res.stderr:
        return None
    found = []
    cmd = platform = None
    for raw in res.stdout.splitlines():
        line = raw.strip()
        if line.startswith("cmd "):
            cmd, platform = line.split()[1], None
        elif cmd == "LC_BUILD_VERSION" and line.startswith("platform "):
            platform = line.split()[1]
        elif cmd == "LC_BUILD_VERSION" and line.startswith("minos "):
            found.append((platform or "?", line.split()[1]))
        elif cmd == "LC_VERSION_MIN_MACOSX" and line.startswith("version "):
            found.append(("1", line.split()[1]))
    return found


def bundle_errors(app: pathlib.Path, floor: str) -> tuple[list[str], int]:
    errors = []
    plist_path = app / "Contents" / "Info.plist"
    try:
        with plist_path.open("rb") as f:
            plist = plistlib.load(f)
    except (OSError, plistlib.InvalidFileException) as exc:
        return [f"{plist_path}: unreadable ({exc})"], 0
    lsmin = str(plist.get("LSMinimumSystemVersion", ""))
    if not lsmin or not re.fullmatch(r"\d+(?:\.\d+)*", lsmin) or version(lsmin) != version(floor):
        errors.append(f"{plist_path}: LSMinimumSystemVersion is {lsmin or 'missing'}, "
                      f"the floor is {floor}")
    exe = app / "Contents" / "MacOS" / str(plist.get("CFBundleExecutable", ""))

    machos = 0
    saw_exe = False
    for path in sorted(app.rglob("*")):
        if path.is_symlink() or not path.is_file() or not is_macho(path):
            continue
        slices = build_versions(path)
        if slices is None:
            continue
        machos += 1
        saw_exe |= path == exe
        rel = path.relative_to(app.parent)
        if not slices:
            errors.append(f"{rel}: no LC_BUILD_VERSION: built for no stated macOS")
        for platform, minos in slices:
            if platform not in ("1", "MACOS"):
                errors.append(f"{rel}: platform {platform}, not macOS")
            elif version(minos) != version(floor):
                errors.append(f"{rel}: minos {minos}, the floor is {floor}")
    if not saw_exe:
        errors.append(f"{app.name}: its executable {exe.name or '?'} was not among the "
                      f"{machos} Mach-O file(s) read: the walk lost its subject")
    return errors, machos


@contextlib.contextmanager
def mounted(dmg: pathlib.Path):
    with tempfile.TemporaryDirectory(prefix="cadgoose-dmg-") as mnt:
        res = run(["hdiutil", "attach", "-readonly", "-nobrowse", "-noautoopen",
                   "-mountpoint", mnt, str(dmg)])
        if res.returncode != 0:
            sys.exit(f"✗ {TAG} hdiutil attach {dmg} failed: {res.stderr.strip()}")
        try:
            yield pathlib.Path(mnt)
        finally:
            run(["hdiutil", "detach", "-force", mnt])


# ── calibration ────────────────────────────────────────────────────────────

def probe(floor: str) -> list[str]:
    """Every planted defect goes red; the clean fixture stays green."""
    major = version(floor)[0]
    below, above = f"{major - 1}.0", f"{major + 1}.0"
    failures = []
    with tempfile.TemporaryDirectory(prefix="cadgoose-floor-probe-") as tmp:
        work = pathlib.Path(tmp)

        def cc(out: pathlib.Path, *extra: str) -> None:
            res = run(["xcrun", "clang", "-arch", "arm64", f"-mmacosx-version-min={floor}",
                       *extra, "-x", "c", "-", "-o", str(out)],
                      source="int f(void){return 0;}\nint main(void){return f();}\n")
            if res.returncode != 0:
                sys.exit(f"✗ {TAG} probe: clang failed: {res.stderr.strip()}")

        def restamp(path: pathlib.Path, minos: str | None) -> None:
            args = (["-remove-build-version", "macos"] if minos is None
                    else ["-set-build-version", "macos", minos, minos, "-replace"])
            res = run(["xcrun", "vtool", *args, "-output", str(path), str(path)])
            if res.returncode != 0:
                sys.exit(f"✗ {TAG} probe: vtool failed: {res.stderr.strip()}")

        exe_src, lib_src = work / "exe", work / "lib.dylib"
        cc(exe_src)
        cc(lib_src, "-dynamiclib")

        def fixture(name: str, lsmin: str = floor) -> pathlib.Path:
            app = work / name / "Probe.app"
            (app / "Contents" / "MacOS").mkdir(parents=True)
            (app / "Contents" / "Frameworks").mkdir()
            (app / "Contents" / "Resources" / "Assets").mkdir(parents=True)
            (app / "Contents" / "MacOS" / "Probe").write_bytes(exe_src.read_bytes())
            (app / "Contents" / "Frameworks" / "libok.dylib").write_bytes(lib_src.read_bytes())
            (app / "Contents" / "Resources" / "Assets" / "sound.wav").write_bytes(b"RIFF")
            with (app / "Contents" / "Info.plist").open("wb") as f:
                plistlib.dump({"CFBundleExecutable": "Probe", "LSMinimumSystemVersion": lsmin}, f)
            return app

        def expect(name: str, app: pathlib.Path, needle: str | None) -> None:
            errors, _ = bundle_errors(app, floor)
            if needle is None and errors:
                failures.append(f"probe {name}: the clean fixture went red: {errors}")
            elif needle is not None and not any(needle in e for e in errors):
                failures.append(f"probe {name}: planted defect stayed green "
                                f"(wanted an error naming {needle!r}, got {errors})")

        expect("clean", fixture("clean"), None)
        app = fixture("old-dylib")
        (app / "Contents" / "Frameworks" / "libold.dylib").write_bytes(lib_src.read_bytes())
        restamp(app / "Contents" / "Frameworks" / "libold.dylib", below)
        expect("dylib below the floor", app, f"libold.dylib: minos {below}")
        app = fixture("new-exe")
        restamp(app / "Contents" / "MacOS" / "Probe", above)
        expect("executable above the floor", app, f"Probe: minos {above}")
        app = fixture("hidden-helper")
        helper = app / "Contents" / "Resources" / "Assets" / "helper"
        helper.write_bytes(exe_src.read_bytes())
        restamp(helper, below)
        expect("helper under Resources", app, f"helper: minos {below}")
        app = fixture("unstamped")
        restamp(app / "Contents" / "Frameworks" / "libok.dylib", None)
        expect("no build version", app, "libok.dylib: no LC_BUILD_VERSION")
        expect("plist below the floor", fixture("plist", lsmin=below), "LSMinimumSystemVersion")
        app = fixture("no-exe")
        (app / "Contents" / "MacOS" / "Probe").unlink()
        expect("executable missing", app, "lost its subject")

    other = next(name for m, name in CODENAMES.items() if m != major)
    static_plants = [
        ("README row below", readme_errors(f"| macOS {below}+ | Supported |\n", floor)),
        ("README with no row", readme_errors("macOS only\n", floor)),
        ("docs claim below", claim_errors(f"needs macOS {below}+ to run", floor, "x.md")),
        ("cask without depends_on", cask_errors('cask "x" do\nend\n', floor)),
        ("cask on another OS", cask_errors(f"depends_on macos: :{other}", floor)),
        ("cask in the deprecated form", cask_errors(f'depends_on macos: ">= :{CODENAMES[major]}"',
                                                    floor)),
        ("second source: triple", second_source_errors(f"-target arm64-apple-macos{floor}", "x")),
        ("second source: plist", second_source_errors(
            f"<key>LSMinimumSystemVersion</key>\n    <string>{floor}</string>", "x")),
        ("second source: flag", second_source_errors(f"-DCMAKE_OSX_DEPLOYMENT_TARGET={floor}", "x")),
    ]
    failures += [f"probe {name}: planted defect stayed green" for name, errs in static_plants if not errs]
    clean_static = (readme_errors(f"| macOS {major}+ | Supported |\n", floor)
                    + claim_errors(f"macOS {major}+ and macOS {above}+", floor, "x.md")
                    + cask_errors(f"depends_on macos: :{CODENAMES[major]}", floor)
                    + second_source_errors("-target arm64-apple-macos${CMAKE_OSX_DEPLOYMENT_TARGET}", "x"))
    if clean_static:
        failures.append(f"probe: clean static text went red: {clean_static}")
    if not failures:
        print(f"✓ {TAG} probe: {7 + len(static_plants)} planted defects each went red, "
              "the clean fixtures stayed green")
    return failures


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    ap.add_argument("--bundle", type=pathlib.Path, action="append", default=[],
                    help="a built .app to read (repeatable)")
    ap.add_argument("--dmg", type=pathlib.Path, help="a DMG whose .app(s) to read")
    ap.add_argument("--probe", action="store_true", help="prove the check can go red")
    args = ap.parse_args()

    floor = read_floor()
    errors, scanned = static_errors(floor)
    if scanned < 100:
        errors.append(f"{TAG} read {scanned} tracked text files, floor 100: the scan lost its subject")

    def check(app: pathlib.Path, label: str) -> None:
        errs, machos = bundle_errors(app, floor)
        errors.extend(errs)
        if not errs:
            print(f"✓ {TAG} {label}: Info.plist and all {machos} Mach-O file(s) say macOS {floor}")

    for app in args.bundle:
        check(app, str(app))
    if args.dmg:
        with mounted(args.dmg) as mnt:
            apps = sorted(mnt.glob("*.app"))
            if not apps:
                errors.append(f"{args.dmg}: no .app at the root of the DMG")
            for app in apps:
                check(app, f"{args.dmg.name}:{app.name}")
    if args.probe:
        errors += probe(floor)

    for e in errors:
        print(f"✗ {TAG} {e}", file=sys.stderr)
    if errors:
        return 1
    print(f"✓ {TAG} the floor is macOS {floor}: README, cask and {scanned} tracked files agree, "
          "and nothing else writes it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
