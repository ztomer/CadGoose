#!/usr/bin/env bash
set -euo pipefail

# build_debug.sh — macOS DEBUG build for CadGoose.
#
# A Debug-type build with all debug logging ENABLED (CG_DISABLE_DEBUG_LOG is OFF),
# so CG_DEBUG / DebugLog / LogTick / DEBUG_LOG emit as usual (still gated at
# runtime by debug.toTerminal / g_debugMode / --debug). Use this for local
# development and diagnosing the goose's behavior.
#
# For an AddressSanitizer build, add ENABLE_ASAN:
#   EXTRA_CMAKE_ARGS="-DENABLE_ASAN=ON" ./build_debug.sh
#
# Usage: ./build_debug.sh [build-dir]   (default: build-debug)
#        SKIP_DEPS=1 ./build_debug.sh    to skip the Homebrew dependency check

cd "$(dirname "$0")"

BUILD_TYPE=Debug \
    exec ./build.sh "${1:-build-debug}"
