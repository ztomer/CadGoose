#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-debug}"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -GNinja
cmake --build "$BUILD_DIR" --verbose
echo "==> $BUILD_DIR/CadGoose"
