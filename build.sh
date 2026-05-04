#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "Building Desktop Goose for macOS..."

rm -rf build
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo ""
echo "Build complete! Binary: build/CadGoose"