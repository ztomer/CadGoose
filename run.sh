#!/bin/bash

cd "$(dirname "$0")"

if [ ! -f "build/CadGoose" ]; then
    echo "Binary not found. Running build.sh first..."
    ./build.sh
fi

echo "Starting Desktop Goose..."
./build/CadGoose "$@"