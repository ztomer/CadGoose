#!/usr/bin/env bash
set -euo pipefail

# check_coverage.sh — Build with coverage, run tests, check P0/P1/total
# line coverage against per-tier thresholds. Exits 0 if all met, 1 if any below.
#
# Usage:
#   ./scripts/check_coverage.sh [--p0-min=94] [--p1-min=30] [--total-min=79] [--build-dir=build-cov]
#
# Flags:
#   --p0-min=N    Minimum line coverage % for P0 files (default: 94)
#   --p1-min=N    Minimum line coverage % for P1 files (default: 30)
#   --total-min=N Minimum line coverage % for all project files (default: 79)
#   --build-dir   CMake build directory (default: build-cov)
#
# The defaults are a RATCHET sitting below the measured floor (CI measured
# P0 95.27%, P1 54.44%, total 86.01% on 2026-08-22), so the gate fails on a
# REGRESSION rather than on noise.
#
# Keep ~1pp of headroom when raising these. Coverage is not perfectly
# deterministic: repeated runs of the same commit have varied by up to 0.4pp on
# P1 (timing-sensitive async-log tests, randomised behavior tests), and a
# threshold pressed against the measured value goes red on jitter. A gate that
# cries wolf gets switched off, which costs more than the 1pp it buys.
#
# Raise them as coverage lands; never lower them to turn a red build green.

P0_MIN=94
P1_MIN=53
TOTAL_MIN=85
BUILD_DIR="build-cov"
ELIGIBLE_FILE="$(dirname "$0")/coverage_eligible.txt"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --p0-min=*) P0_MIN="${1#*=}" ;;
        --p1-min=*) P1_MIN="${1#*=}" ;;
        --total-min=*) TOTAL_MIN="${1#*=}" ;;
        --build-dir=*) BUILD_DIR="${1#*=}" ;;
        --help|-h)
            echo "Usage: $0 [--p0-min=94] [--p1-min=30] [--total-min=79] [--build-dir=build-cov]"
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
EXCLUDED=()

# NOTE: CI runs this under macOS /bin/bash (3.2), where `set -u` makes
# "${arr[@]}" on an EMPTY array a fatal "unbound variable". Every expansion
# below is therefore guarded by a non-empty check — never expand an array here
# without one.
#
# `|| [[ -n "$tier" ]]` keeps the final line if the file lacks a trailing newline.
while IFS=: read -r tier glob || [[ -n "$tier" ]]; do
    tier="${tier## }"
    tier="${tier%% }"
    glob="${glob## }"
    glob="${glob%% }"
    [[ -z "$tier" || "$tier" =~ ^# ]] && continue
    resolved=()
    # compgen -G expands the pattern while keeping "$PWD/$glob" quoted, so paths
    # containing spaces survive. A bare `for f in "$PWD/$glob"` does NOT glob at
    # all — the quotes make it a literal string, which resolves nothing.
    while IFS= read -r f; do
        [[ -f "$f" ]] && resolved+=("$f")
    done < <(compgen -G "$PWD/$glob" || true)
    if [[ ${#resolved[@]} -eq 0 ]]; then
        echo "ERROR: tier $tier pattern '$glob' matched no files"
        exit 1
    fi
    if [[ "$tier" == "EXCLUDE" ]]; then
        EXCLUDED+=("${resolved[@]}")
    elif [[ "$tier" == "P0" ]]; then
        P0_SOURCES+=("${resolved[@]}")
    elif [[ "$tier" == "P1" ]]; then
        P1_SOURCES+=("${resolved[@]}")
    fi
    if [[ "$tier" != "EXCLUDE" ]]; then
        ALL_SOURCES+=("${resolved[@]}")
    fi
done < "$ELIGIBLE_FILE"

# Drop excluded files from the tier lists. EXCLUDE entries are order-independent:
# a file stays out whether its EXCLUDE line comes before or after the P0/P1 glob
# that would otherwise sweep it in.
if [[ ${#EXCLUDED[@]} -gt 0 ]]; then
    echo ""
    echo "==> Excluded from measurement (${#EXCLUDED[@]} file(s)) — see $ELIGIBLE_FILE for the reason:"
    filter_excluded() {
        local kept=()
        local f e skip
        for f in "$@"; do
            skip=0
            for e in "${EXCLUDED[@]}"; do
                if [[ "$f" == "$e" ]]; then skip=1; break; fi
            done
            [[ $skip -eq 0 ]] && kept+=("$f")
        done
        [[ ${#kept[@]} -gt 0 ]] && printf '%s\n' "${kept[@]}"
    }
    for e in "${EXCLUDED[@]}"; do
        echo "    ${e#$PWD/}"
    done
    echo ""

    if [[ ${#P0_SOURCES[@]} -gt 0 ]]; then
        mapfile_p0=()
        while IFS= read -r line; do [[ -n "$line" ]] && mapfile_p0+=("$line"); done \
            < <(filter_excluded "${P0_SOURCES[@]}")
        P0_SOURCES=(${mapfile_p0[@]+"${mapfile_p0[@]}"})
    fi
    if [[ ${#P1_SOURCES[@]} -gt 0 ]]; then
        mapfile_p1=()
        while IFS= read -r line; do [[ -n "$line" ]] && mapfile_p1+=("$line"); done \
            < <(filter_excluded "${P1_SOURCES[@]}")
        P1_SOURCES=(${mapfile_p1[@]+"${mapfile_p1[@]}"})
    fi
    if [[ ${#ALL_SOURCES[@]} -gt 0 ]]; then
        mapfile_all=()
        while IFS= read -r line; do [[ -n "$line" ]] && mapfile_all+=("$line"); done \
            < <(filter_excluded "${ALL_SOURCES[@]}")
        ALL_SOURCES=(${mapfile_all[@]+"${mapfile_all[@]}"})
    fi
fi

if [[ ${#ALL_SOURCES[@]} -eq 0 ]]; then
    echo "ERROR: No source files found from eligible list"
    exit 1
fi

# Generate the full report (all tiers)
echo "==> Generating multi-tier coverage report ($SUMMARY_FILE)..."
mkdir -p "$REPORT_DIR"

# Print everything except the last line of stdin. `head -n -1` would be the
# obvious spelling, but negative counts are a GNU coreutils extension and the
# macOS/CI runner's BSD head rejects them ("illegal line count"). `sed '$d'` is
# portable. run_cov_report appends the percentage as its last line, so callers
# use this to print the report body and `tail -1` to read the number.
drop_last_line() { /usr/bin/sed '$d'; }

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
    echo "$p0_result" | drop_last_line
else
    P0_COVER="0"
fi

# P1 coverage
if [[ ${#P1_SOURCES[@]} -gt 0 ]]; then
    echo ""
    echo "=== P1 Coverage Report ==="
    p1_result=$(run_cov_report "P1" "${P1_SOURCES[@]}")
    P1_COVER=$(echo "$p1_result" | tail -1)
    echo "$p1_result" | drop_last_line
else
    P1_COVER="0"
fi

# Total coverage
echo ""
echo "=== Total Coverage Report ==="
total_result=$(run_cov_report "Total" "${ALL_SOURCES[@]}")
TOTAL_COVER=$(echo "$total_result" | tail -1)
echo "$total_result" | drop_last_line

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
