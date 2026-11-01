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
#
# Seams (tests/profiling/test_run_tests_linux_runtime.sh): CG_HOST_OS names the
# host (default `uname -s`); DOCKER and COLIMA name the engine and runtime.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "${CG_HOST_OS:-$(uname -s)}" in
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
        # A docker that is down is started (colima) and stopped again only if
        # goh started it and no other gate or container is on it (goh
        # lib/container_runtime.sh, GOH.CR1); a host that cannot is red.
        GOH="${GOH_DIR:-$HOME/Projects/gates_of_heck}"
        # shellcheck source=/dev/null
        . "$GOH/lib/container_runtime.sh"
        trap goh_container_runtime_down EXIT
        DOCKER="${DOCKER:-docker}"
        if ! goh_container_runtime_up "cadgoose linux suite"; then
            echo "✗ run_tests_linux.sh: no Docker engine to run the Linux suite on." >&2
            exit 2
        fi
        IMAGE="cadgoose-linux-ci"
        "$DOCKER" build --quiet --tag "$IMAGE" --file "$ROOT/Dockerfile" "$ROOT/scripts" > /dev/null < /dev/null
        "$DOCKER" run --rm --volume "$ROOT:/work:ro" --env CG_LINUX_BUILD_DIR=/tmp/build \
            "$IMAGE" bash /work/scripts/run_tests_linux.sh < /dev/null
        ;;
    *)
        echo "✗ run_tests_linux.sh: unsupported host $(uname -s)" >&2
        exit 2
        ;;
esac
