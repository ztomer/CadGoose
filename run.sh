#!/bin/bash
# Build (if needed) and run CadGoose from the same directory build.sh produces.

cd "$(dirname "$0")"

BIN="build/CadGoose"

# Always build so we never launch a stale binary. build.sh is fast when nothing
# changed and self-heals a stale/foreign CMake cache.
./build.sh || exit 1

if [ ! -f "$BIN" ]; then
    echo "ERROR: $BIN not found after build." >&2
    exit 1
fi

echo "Starting Desktop Goose ($BIN)..."
"./$BIN" "$@"
