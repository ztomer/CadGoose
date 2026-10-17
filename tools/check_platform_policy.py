#!/usr/bin/env python3
"""The platform policy accepts exactly what it should, on one machine.

ROADMAP G0.5: CMake rejected only non-macOS/non-Linux systems, so an Intel
Mac configured and failed late at the arm64-only Swift object, and any
32-bit or other-arch Linux would build untested. cmake/PlatformPolicy.cmake
is now the one policy, with a CG_TARGET_OS/CG_TARGET_ARCH seam. This drives
it in script mode over the whole matrix, BOTH directions (a gate that
over-rejects is as wrong as one that under-rejects), then runs one real
configure through the seam to prove CMakeLists.txt still calls it first.
"""

import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
POLICY = ROOT / "cmake" / "PlatformPolicy.cmake"

# (os, arch, pointer_size, accepted)
MATRIX = [
    ("Darwin", "arm64", "8", True),
    ("Linux", "x86_64", "8", True),
    ("Linux", "aarch64", "8", True),
    ("Linux", "amd64", "8", True),       # Debian's spelling of x86_64
    ("Linux", "arm64", "8", True),       # Apple's spelling of aarch64
    ("Darwin", "x86_64", "8", False),    # Intel Mac
    ("Darwin", "arm64;x86_64", "8", False),  # universal binary
    ("Darwin", "", "8", False),
    ("Linux", "i686", "4", False),
    ("Linux", "x86_64", "4", False),     # -m32 on a 64-bit host
    ("Linux", "armv7l", "4", False),
    ("Linux", "riscv64", "8", False),    # 64-bit but never built or tested
    ("FreeBSD", "amd64", "8", False),
    ("Windows", "AMD64", "8", False),
]


def decide(os_name: str, arch: str, pointer: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["cmake", f"-DCG_TARGET_OS={os_name}", f"-DCG_TARGET_ARCH={arch}",
         f"-DCG_TARGET_POINTER_SIZE={pointer}", "-P", str(POLICY)],
        capture_output=True, text=True, stdin=subprocess.DEVNULL, check=False,
    )


def main() -> int:
    bad = 0
    for os_name, arch, pointer, accepted in MATRIX:
        res = decide(os_name, arch, pointer)
        ok = (res.returncode == 0) == accepted
        if not ok:
            bad += 1
            want = "accept" if accepted else "reject"
            print(f"✗ [platform_policy] {os_name}/{arch or '<none>'}/{pointer}: expected "
                  f"{want}, cmake exited {res.returncode}\n{res.stderr.strip()}", file=sys.stderr)
        elif not accepted and "unsupported target platform" not in res.stderr:
            bad += 1
            print(f"✗ [platform_policy] {os_name}/{arch}: rejected for the wrong reason:\n"
                  f"{res.stderr.strip()}", file=sys.stderr)
    # Wiring: a real configure through the seam must stop at the policy, before
    # any dependency is fetched or any target defined.
    with tempfile.TemporaryDirectory(prefix="cadgoose-policy-") as build:
        res = subprocess.run(
            ["cmake", "-S", str(ROOT), "-B", build, "-DCG_TARGET_ARCH=x86_64"],
            capture_output=True, text=True, stdin=subprocess.DEVNULL, check=False,
        )
    if res.returncode == 0 or "unsupported target platform" not in res.stderr:
        bad += 1
        print("✗ [platform_policy] a configure with CG_TARGET_ARCH=x86_64 was not stopped by "
              f"the policy (exit {res.returncode}); CMakeLists.txt must include "
              "cmake/PlatformPolicy.cmake right after project()", file=sys.stderr)
    elif "FetchContent" in res.stdout or "toml11" in res.stdout:
        bad += 1
        print("✗ [platform_policy] the policy ran after dependencies were fetched", file=sys.stderr)
    if bad:
        return 1
    print(f"✓ [platform_policy] {len(MATRIX)} combinations decided as the policy says, "
          "and the real configure stops at it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
