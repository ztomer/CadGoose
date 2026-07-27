#!/usr/bin/env bash
set -euo pipefail

# check_coverage.sh — Build with coverage, run tests, check P0/P1/total
# line coverage against per-tier thresholds. Exits 0 if all met, 1 if any below.
#
# Usage:
#   ./scripts/check_coverage.sh [--p0-min=95] [--p1-min=80] [--total-min=90] [--build-dir=build-cov]
#
# Flags:
#   --p0-min=N    Minimum line coverage % for P0 files (default: 95)
#   --p1-min=N    Minimum line coverage % for P1 files (default: 80)
#   --total-min=N Minimum line coverage % for all project files (default: 90)
#   --build-dir   CMake build directory (default: build-cov)

P0_MIN=95
P1_MIN=80
TOTAL_MIN=90
BUILD_DIR="build-cov"
ELIGIBLE_FILE="$(dirname "$0")/coverage_eligible.txt"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --p0-min=*) P0_MIN="${1#*=}" ;;
        --p1-min=*) P1_MIN="${1#*=}" ;;
        --total-min=*) TOTAL_MIN="${1#*=}" ;;
        --build-dir=*) BUILD_DIR="${1#*=}" ;;
        --help|-h)
            echo "Usage: $0 [--p0-min=95] [--p1-min=80] [--total-min=90] [--build-dir=build-cov]"
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
SUMMARY_FILE="$REPORT_DIR/multimetric.txt"

rm -rf "$COV_FILE"

echo "==> Configuring with code coverage..."
cmake -B "$BUILD_DIR" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCODE_COVERAGE=ON 2>&1 | tail -1

echo "==> Building CadGooseTests..."
cmake --build "$BUILD_DIR" --target CadGooseTests 2>&1 | tail -1

echo "==> Running tests..."
LLVM_PROFILE_FILE="$COV_FILE" "$BUILD_DIR/CadGooseTests" \
    --gtest_filter="-WindowTrailTest.*" \
    2>&1 | tail -1 || true

echo "==> Merging profile data..."
xcrun llvm-profdata merge -sparse "$COV_FILE" -o default.profdata

# Parse the eligible file: collect sources by tier
P0_SOURCES=()
P1_SOURCES=()
ALL_SOURCES=()

while IFS=: read -r tier glob; do
    tier="${tier## }"
    tier="${tier%% }"
    glob="${glob## }"
    glob="${glob%% }"
    [[ -z "$tier" || "$tier" =~ ^# ]] && continue
    resolved=()
    for f in "$PWD/$glob"; do
        [[ -f "$f" ]] && resolved+=("$f")
    done
    if [[ "$tier" == "P0" ]]; then
        P0_SOURCES+=("${resolved[@]}")
    elif [[ "$tier" == "P1" ]]; then
        P1_SOURCES+=("${resolved[@]}")
    fi
    ALL_SOURCES+=("${resolved[@]}")
done < "$ELIGIBLE_FILE"

if [[ ${#ALL_SOURCES[@]} -eq 0 ]]; then
    echo "ERROR: No source files found from eligible list"
    exit 1
fi

# Generate the full report (all tiers)
echo "==> Generating multi-tier coverage report ($SUMMARY_FILE)..."
mkdir -p "$REPORT_DIR"

all_report_args=(
    -instr-profile=default.profdata
    -ignore-filename-regex="(vendor|build|tests|googletest)"
    "${ALL_SOURCES[@]}"
)

# Helper: run coverage report for a set of files and extract the total %
run_cov_report() {
    local label="$1"
    shift
    if [[ $# -eq 0 ]]; then
        echo "$label: N/A (no files)" >&2
        echo "0"
        return
    fi
    local report
    report=$(xcrun llvm-cov report "$BUILD_DIR/CadGooseTests" \
        -instr-profile=default.profdata \
        -ignore-filename-regex="(vendor|build|tests|googletest)" \
        "$@" 2>/dev/null)
    echo "$report"
    local pct
    pct=$(echo "$report" | tail -1 | awk '{print $10}')
    pct="${pct%\%}"
    if [[ -z "$pct" ]]; then
        echo "0"
    else
        echo "$pct"
    fi
}

# P0 coverage
if [[ ${#P0_SOURCES[@]} -gt 0 ]]; then
    echo ""
    echo "=== P0 Coverage Report ==="
    p0_result=$(run_cov_report "P0" "${P0_SOURCES[@]}")
    P0_COVER=$(echo "$p0_result" | tail -1)
    echo "$p0_result" | head -n -1
else
    P0_COVER="0"
fi

# P1 coverage
if [[ ${#P1_SOURCES[@]} -gt 0 ]]; then
    echo ""
    echo "=== P1 Coverage Report ==="
    p1_result=$(run_cov_report "P1" "${P1_SOURCES[@]}")
    P1_COVER=$(echo "$p1_result" | tail -1)
    echo "$p1_result" | head -n -1
else
    P1_COVER="0"
fi

# Total coverage
echo ""
echo "=== Total Coverage Report ==="
total_result=$(run_cov_report "Total" "${ALL_SOURCES[@]}")
TOTAL_COVER=$(echo "$total_result" | tail -1)
echo "$total_result" | head -n -1

# Summary
{
    echo "Coverage Summary"
    echo "================"
    echo "P0:    ${P0_COVER}%  (threshold: ${P0_MIN}%)"
    echo "P1:    ${P1_COVER}%  (threshold: ${P1_MIN}%)"
    echo "Total: ${TOTAL_COVER}%  (threshold: ${TOTAL_MIN}%)"
} | tee "$SUMMARY_FILE"

# Check all thresholds
ALL_PASS=true

if (( $(echo "$P0_COVER < $P0_MIN" | bc -l) )); then
    echo "FAILED: P0 coverage ${P0_COVER}% is below threshold ${P0_MIN}%"
    ALL_PASS=false
else
    echo "PASSED: P0 coverage ${P0_COVER}% meets threshold ${P0_MIN}%"
fi

if (( $(echo "$P1_COVER < $P1_MIN" | bc -l) )); then
    echo "FAILED: P1 coverage ${P1_COVER}% is below threshold ${P1_MIN}%"
    ALL_PASS=false
else
    echo "PASSED: P1 coverage ${P1_COVER}% meets threshold ${P1_MIN}%"
fi

if (( $(echo "$TOTAL_COVER < $TOTAL_MIN" | bc -l) )); then
    echo "FAILED: Total coverage ${TOTAL_COVER}% is below threshold ${TOTAL_MIN}%"
    ALL_PASS=false
else
    echo "PASSED: Total coverage ${TOTAL_COVER}% meets threshold ${TOTAL_MIN}%"
fi

if [[ "$ALL_PASS" == "true" ]]; then
    echo "All coverage thresholds met."
    exit 0
else
    exit 1
fi
