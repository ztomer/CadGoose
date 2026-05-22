#!/bin/bash

cd "$(dirname "$0")"

if [ ! -f "release-build/CadGoose" ]; then
    echo "Binary not found. Running build.sh first..."
    ./build.sh
fi

echo "Starting Desktop Goose..."
codesign -d --entitlements :- ./release-build/CadGoose
./release-build/CadGoose "$@"
