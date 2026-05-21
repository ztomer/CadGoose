#!/bin/bash

set -e

cd "$(dirname "$0")"

BUILD_TYPE="${1:-Release}"

if [ "${BUILD_TYPE}" = "Release" ]; then
    BUILD_DIR="release-build"
elif [ "${BUILD_TYPE}" = "Debug" ]; then
    BUILD_DIR="debug-build"
else
    BUILD_DIR="${BUILD_TYPE}-build"
fi

echo "Building Desktop Goose for macOS (${BUILD_TYPE})..."

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -G Ninja .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
ninja

echo ""
echo "Build complete! Binary: ${BUILD_DIR}/CadGoose"