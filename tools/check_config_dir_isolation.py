#!/usr/bin/env python3
"""The app's settings directory is its own, named after its bundle id.

ROADMAP D1.1: two apps that share one settings directory share one
config.toml, and the two schemas have diverged. This app's directory is
~/Library/Application Support/<bundle id>; the shared directory it used to read
and write (~/Library/Application Support/CadGoose) belongs to the macOS
successor fork now, and this app reads it once, to migrate out of it.

What this gate holds:

  1. The bundle id is written down once, in include/app_identity.h, and it is
     reverse-DNS. A bare "CadGoose" would be the shared directory again.
  2. The bundler reads that id from there and hardcodes none. An Info.plist
     that disagrees with the settings directory is exactly the drift this gate
     is for: the app would ship under one id and keep its settings under
     another.
  3. The app's own directory is built from the bundle id, and the shared
     directory's name appears in the sources exactly once: in the legacy path
     the migration reads. `Application Support` is named exactly twice in the
     tree (the two directories); a third is a resolution nobody checked.

Every check reads the tree. None of them can pass without reading it.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
IDENTITY_HEADER = ROOT / "include" / "app_identity.h"
BUNDLER = ROOT / "scripts" / "create_bundle.sh"
SOURCES = ROOT / "src"

# The one line both the C++ and the shell read.
ID_DEFINE = re.compile(r'^\s*#define\s+CADGOOSE_BUNDLE_ID\s+"([^"]*)"\s*$', re.M)
# Reverse-DNS: at least one dot, and no bare name. "CadGoose" is the shared
# directory; a bundle id that is not reverse-DNS could be one.
REVERSE_DNS = re.compile(r"^[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?"
                         r"(\.[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?)+$")
# The app's own directory: Application Support, then the bundle id.
OWN_DIRECTORY = re.compile(r'"Application Support"\s*/\s*bundleId')
# The shared directory: Application Support, then the name it used to have.
SHARED_DIRECTORY = re.compile(r'"Application Support"\s*/\s*"CadGoose"')


def fail(message: str) -> None:
    print(f"✗ [config_dir_isolation] {message}", file=sys.stderr)
    sys.exit(1)


def main() -> int:
    if not IDENTITY_HEADER.is_file():
        fail(f"{IDENTITY_HEADER.relative_to(ROOT)} is missing: the bundle id has "
             "to be written down somewhere")
    header = IDENTITY_HEADER.read_text()
    found = ID_DEFINE.findall(header)
    if len(found) != 1:
        fail(f"include/app_identity.h defines CADGOOSE_BUNDLE_ID "
             f"{len(found)} times; exactly one definition is expected")
    bundle_id = found[0]
    if not REVERSE_DNS.match(bundle_id):
        fail(f"the bundle id {bundle_id!r} is not reverse-DNS; a bare name "
             "would be the shared settings directory again")

    if not BUNDLER.is_file():
        fail(f"{BUNDLER.relative_to(ROOT)} is missing")
    bundler = BUNDLER.read_text()
    if "include/app_identity.h" not in bundler:
        fail("scripts/create_bundle.sh does not read the bundle id from "
             "include/app_identity.h; the id is written down once, and the "
             "bundler reads it there")
    if bundle_id in bundler:
        fail("scripts/create_bundle.sh hardcodes the bundle id; it must read "
             "it from include/app_identity.h, so the two cannot drift")

    sources = sorted(p for p in SOURCES.rglob("*") if p.suffix in (".cpp", ".mm", ".h"))
    if not sources:
        fail("no sources were read: the check has lost its subject")
    hits = 0
    shared_hits = 0
    support_total = 0
    for path in sources:
        text = path.read_text(errors="replace")
        rel = path.relative_to(ROOT)
        hits += len(OWN_DIRECTORY.findall(text))
        shared_hits += len(SHARED_DIRECTORY.findall(text))
        # "Application Support" is the parent of both directories. Count per
        # file: a running total would blame the second file for the first
        # file's two mentions, and the tree has one source per mention.
        support_here = text.count('"Application Support"')
        support_total += support_here
        if support_here > 2:
            fail(f"{rel} names the Application Support directory {support_here} "
                 "times; the app's own directory and the shared one are the "
                 "only two")
    if hits != 1:
        fail(f"the app's own settings directory is built from the bundle id in "
             f"{hits} places; exactly one is expected (the resolution)")
    if shared_hits != 1:
        fail(f"the shared directory's name appears in {shared_hits} places; "
             "exactly one is expected (the legacy path the migration reads)")
    if support_total != 2:
        fail(f"the Application Support directory is named in {support_total} "
             "places; exactly two are expected (the resolution and the legacy "
             "path), and a third is a second resolution nobody checked")

    print(f"✓ [config_dir_isolation] bundle id {bundle_id}, one own directory, "
          f"one shared path, in {len(sources)} sources")
    return 0


if __name__ == "__main__":
    sys.exit(main())
