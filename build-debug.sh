#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "Building Desktop Goose for macOS (Debug + ASan)..."

BUILD_DIR="build-debug"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug
ninja

echo ""
echo "Debug build complete! Binary: ${BUILD_DIR}/CadGoose"
echo "Run with: ASAN_OPTIONS=halt_on_error=1 ./${BUILD_DIR}/CadGoose"
