#!/usr/bin/env bash
# run_tests_linux.sh — build and run the Linux test suite (CadGooseLinuxTests,
# ROADMAP M1.5), from the repository root.
#
# On Linux (CI's Linux job) it builds the target in $CG_LINUX_BUILD_DIR
# (default: build) and runs it under ctest. On macOS it runs this same script
# inside the repo's Dockerfile image, with the tree mounted read-only, so the
# local pre-push gate runs the Linux suite too instead of trusting CI alone.
# A host that can do neither is red, never a skip: a gate that silently does
# not run reads exactly like one that passed.
#
#   bash scripts/run_tests_linux.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "$(uname -s)" in
    Linux)
        BUILD_DIR="${CG_LINUX_BUILD_DIR:-build}"
        cd "$ROOT"
        if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
            cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release < /dev/null
        fi
        cmake --build "$BUILD_DIR" --target CadGooseLinuxTests --parallel "$(nproc)" < /dev/null
        # --no-tests=error: a selection that matches nothing is red, not "passed".
        ctest --test-dir "$BUILD_DIR" -R '^CadGooseLinuxTests$' --output-on-failure \
            --no-tests=error < /dev/null
        ;;
    Darwin)
        if ! docker info > /dev/null 2>&1 < /dev/null; then
            echo "✗ run_tests_linux.sh: no running Docker engine (start colima); the Linux suite cannot run." >&2
            exit 2
        fi
        IMAGE="cadgoose-linux-ci"
        docker build --quiet --tag "$IMAGE" --file "$ROOT/Dockerfile" "$ROOT/scripts" > /dev/null < /dev/null
        docker run --rm --volume "$ROOT:/work:ro" --env CG_LINUX_BUILD_DIR=/tmp/build \
            "$IMAGE" bash /work/scripts/run_tests_linux.sh < /dev/null
        ;;
    *)
        echo "✗ run_tests_linux.sh: unsupported host $(uname -s)" >&2
        exit 2
        ;;
esac
