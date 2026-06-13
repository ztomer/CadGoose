#!/usr/bin/env bash
set -euo pipefail

# check_coverage.sh — Build with coverage, run tests, check P0 line coverage
# against a minimum threshold. Exits 0 if met, 1 if below.
#
# Usage:
#   ./scripts/check_coverage.sh [--p0-min=50] [--build-dir=build-cov]
#
# Flags:
#   --p0-min=N   Minimum line coverage % for P0 files (default: 50)
#   --build-dir  CMake build directory (default: build-cov)

P0_MIN=80
BUILD_DIR="build-cov"
ELIGIBLE_FILE="$(dirname "$0")/coverage_eligible.txt"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --p0-min=*) P0_MIN="${1#*=}" ;;
        --build-dir=*) BUILD_DIR="${1#*=}" ;;
        --help|-h)
            echo "Usage: $0 [--p0-min=50] [--build-dir=build-cov]"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

if [[ ! -f "$ELIGIBLE_FILE" ]]; then
    echo "ERROR: Eligible file list not found: $ELIGIBLE_FILE"
    exit 1
fi

COV_FILE="default.profraw"
REPORT_DIR="coverage-report"
REPORT_FILE="$REPORT_DIR/summary.txt"

rm -rf "$COV_FILE"

echo "==> Configuring with code coverage..."
cmake -B "$BUILD_DIR" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCODE_COVERAGE=ON 2>&1 | tail -1

echo "==> Building CadGooseTests..."
cmake --build "$BUILD_DIR" --target CadGooseTests 2>&1 | tail -1

echo "==> Running tests..."
# 4 known order-dependent behavior registration failures excluded
LLVM_PROFILE_FILE="$COV_FILE" "$BUILD_DIR/CadGooseTests" \
    --gtest_filter="-MCPIntegrationTest*:LocalLLMTest*:AccessibilityGUITest*:DraggingIntegration*:WindowTrailTest*:BehaviorToggles.ToysBehaviorRegistered:PortalCleanup.BehaviorHasCleanupFunction:StalinHonk.*" \
    2>&1 | tail -1 || true

echo "==> Merging profile data..."
xcrun llvm-profdata merge -sparse "$COV_FILE" -o default.profdata

# Collect P0-only source paths from the eligible list (skip comments, empty lines)
P0_SOURCES=()
while IFS= read -r line; do
    line="${line%%#*}"  # strip comments
    line="${line## }"   # trim leading
    line="${line%% }"   # trim trailing
    [[ -z "$line" ]] && continue
    # Resolve glob against source tree
    for f in $PWD/$line; do
        [[ -f "$f" ]] && P0_SOURCES+=("$f")
    done
done < "$ELIGIBLE_FILE"

if [[ ${#P0_SOURCES[@]} -eq 0 ]]; then
    echo "ERROR: No P0 source files found from eligible list"
    exit 1
fi

echo "==> Generating P0-only coverage report ($REPORT_FILE)..."
mkdir -p "$REPORT_DIR"
xcrun llvm-cov report "$BUILD_DIR/CadGooseTests" \
    -instr-profile=default.profdata \
    -ignore-filename-regex="(vendor|build|tests|googletest)" \
    "${P0_SOURCES[@]}" \
    > "$REPORT_FILE"

echo ""
echo "=== P0 Coverage Report ==="
cat "$REPORT_FILE"
echo ""

# Extract the TOTAL line's coverage percentage
P0_COVER=$(tail -1 "$REPORT_FILE" | awk '{print $10}')
P0_COVER="${P0_COVER%\%}"

if [[ -z "$P0_COVER" ]]; then
    echo "ERROR: Could not parse coverage from report"
    exit 1
fi

echo "P0 line coverage: ${P0_COVER}%  (threshold: ${P0_MIN}%)"

if (( $(echo "$P0_COVER < $P0_MIN" | bc -l) )); then
    echo "FAILED: P0 coverage ${P0_COVER}% is below threshold ${P0_MIN}%"
    exit 1
else
    echo "PASSED: P0 coverage ${P0_COVER}% meets threshold ${P0_MIN}%"
fi
