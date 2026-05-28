#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-linux}"
echo "==> Configuring CMake (Release)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -GNinja 2>/dev/null
echo "==> Building..."
cmake --build "$BUILD_DIR" 2>&1 | grep -v "^\[[0-9/%]*\] Building"
echo "==> $BUILD_DIR/CadGoose"
