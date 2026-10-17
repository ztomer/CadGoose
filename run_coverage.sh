#!/usr/bin/env bash
set -euo pipefail

# Build with code coverage, run all tests, generate HTML report.
# Requires Xcode command-line tools (llvm-profdata, llvm-cov).

BUILD_DIR="build-cov"
REPORT_DIR="coverage-report"
COV_FILE="default.profraw"

rm -rf "$BUILD_DIR" "$REPORT_DIR" "$COV_FILE"

echo "==> Configuring with code coverage..."
cmake -B "$BUILD_DIR" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCODE_COVERAGE=ON

echo "==> Building CadGooseTests..."
cmake --build "$BUILD_DIR" --target CadGooseTests

echo "==> Running tests (LLVM_PROFILE_FILE=$COV_FILE)..."
# Same single filter source as run_tests_ci.sh / Tests.cmake — no value copy
# here (ROADMAP G0.2); this copy had already drifted from the other two.
FILTER="$(sed '/^#/d; /^[[:space:]]*$/d; s/[[:space:]]//g' tests/gtest_ci_filter.txt | paste -sd: -)"
LLVM_PROFILE_FILE="$COV_FILE" "$BUILD_DIR/CadGooseTests" \
    --gtest_filter="$FILTER"

echo "==> Merging and generating report..."
xcrun llvm-profdata merge -sparse "$COV_FILE" -o default.profdata

# Source files to include (common sources + platform/macos for macOS tests)
SOURCES_DIRS="$PWD/src/common $PWD/src/platform/macos $PWD/include"

echo "==> Generating HTML report..."
mkdir -p "$REPORT_DIR"
xcrun llvm-cov show "$BUILD_DIR/CadGooseTests" \
    -instr-profile=default.profdata \
    -output-dir="$REPORT_DIR" \
    -format=html \
    -ignore-filename-regex="(vendor|build|tests|googletest)" \
    $SOURCES_DIRS

echo "==> Generating text summary..."
xcrun llvm-cov report "$BUILD_DIR/CadGooseTests" \
    -instr-profile=default.profdata \
    -ignore-filename-regex="(vendor|build|tests|googletest)" \
    $SOURCES_DIRS \
    > "$REPORT_DIR/summary.txt"

echo ""
echo "=== Coverage Summary ==="
cat "$REPORT_DIR/summary.txt"
echo ""
echo "HTML report: $REPORT_DIR/index.html"
echo "Text summary: $REPORT_DIR/summary.txt"
