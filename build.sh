#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "Building Desktop Goose for macOS..."

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo ""
echo "Build complete! Binary: build/CadGoose"