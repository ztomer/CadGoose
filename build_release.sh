#!/usr/bin/env bash
set -euo pipefail

# build_release.sh — macOS RELEASE build for CadGoose (this is the CI / shipping
# build).
#
# Release builds compile out ALL debug-level logging at the preprocessor level
# (-DCG_DISABLE_DEBUG_LOG=ON): CG_DEBUG / CG_DEBUG_ASYNC / DebugLog / DebugLogv /
# LogTick and the DEBUG_LOG macros expand to ((void)0), so there is zero
# per-frame logging overhead in the binary (no call, no argument evaluation, no
# string formatting). CG_ERROR / CG_WARN / CG_INFO are unaffected.
#
# The flag is applied to the CadGoose app target only, so CadGooseTests still
# builds with full logging and the log unit tests keep passing.
#
# Usage: ./build_release.sh [build-dir]   (default: build-release)
#        SKIP_DEPS=1 ./build_release.sh    to skip the Homebrew dependency check

cd "$(dirname "$0")"

BUILD_TYPE=Release \
EXTRA_CMAKE_ARGS="-DCG_DISABLE_DEBUG_LOG=ON" \
    exec ./build.sh "${1:-build-release}"
