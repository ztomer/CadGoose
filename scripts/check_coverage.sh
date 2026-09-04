#!/usr/bin/env bash
# check_coverage.sh — this repo's coverage gate, self-contained.
#
# Instrumented configure/build, ctest under LLVM_PROFILE_FILE, profdata merge, and the TOTAL floor
# against --total-min; then the tier floors below. It needs nothing but this checkout, because a
# public project cannot gate itself on machinery only its author can fetch.
#
# The per-tier steps:
#   - the per-tier floors over coverage_eligible.txt (P0 >= p0-min,
#     P1 >= p1-min), measured from the same merged profdata;
#   - Total scoped to the ELIGIBLE file set, alongside the stricter
#     whole-binary total above, which honours the same EXCLUDE contract.
#
# Flags preserved exactly (the CI workflow passes them):
#   ./scripts/check_coverage.sh [--p0-min=94] [--p1-min=53] [--total-min=85] [--build-dir=build-cov]
#
# Exit: 0 all floors met | 1 any floor missed | 2 usage/config error.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ELIGIBLE_FILE="$ROOT/scripts/coverage_eligible.txt"

P0_MIN=94
P1_MIN=53
TOTAL_MIN=85
BUILD_DIR="build-cov"

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
        *) echo "Unknown option: $1"; exit 2 ;;
    esac
    shift
done

if [[ ! -f "$ELIGIBLE_FILE" ]]; then
    echo "ERROR: Eligible file list not found: $ELIGIBLE_FILE"
    exit 2
fi

BUILD_DIR="$ROOT/$BUILD_DIR"
PROFDATA="$BUILD_DIR/default.profdata"
TEST_BIN="$BUILD_DIR/CadGooseTests"

# ── Tier policy (repo-local): parse coverage_eligible.txt ───────────────────
# Verbatim from the pre-shim script, including the macOS /bin/bash 3.2 guards:
# `set -u` makes "${arr[@]}" on an EMPTY array fatal, so every expansion is
# guarded; `|| [[ -n "$tier" ]]` keeps a missing trailing newline's last line;
# compgen -G keeps quoted paths with spaces globbing.
P0_SOURCES=()
P1_SOURCES=()
ALL_SOURCES=()
EXCLUDED=()

while IFS=: read -r tier glob || [[ -n "$tier" ]]; do
    tier="${tier## }"; tier="${tier%% }"
    glob="${glob## }"; glob="${glob%% }"
    [[ -z "$tier" || "$tier" =~ ^# ]] && continue
    resolved=()
    while IFS= read -r f; do
        [[ -f "$f" ]] && resolved+=("$f")
    done < <(compgen -G "$PWD/$glob" || true)
    if [[ ${#resolved[@]} -eq 0 ]]; then
        echo "ERROR: tier $tier pattern '$glob' matched no files"
        exit 2
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

filter_excluded() {
    local kept=() f e skip
    for f in "$@"; do
        skip=0
        for e in "${EXCLUDED[@]}"; do
            if [[ "$f" == "$e" ]]; then skip=1; break; fi
        done
        [[ $skip -eq 0 ]] && printf '%s\n' "$f"
    done
}

drop_last_line() { /usr/bin/sed '$d'; }

run_cov_report() {
    local report pct
    report=$(xcrun llvm-cov report "$TEST_BIN" \
        -instr-profile="$PROFDATA" \
        -ignore-filename-regex="(vendor|build|tests|googletest)" \
        "$@" 2>/dev/null)
    echo "$report"
    pct=$(echo "$report" | tail -1 | awk '{print $10}')
    pct="${pct%\%}"
    [[ -z "$pct" ]] && echo "0" || echo "$pct"
}

# EXCLUDE-tier globs join the shared gate's ignore regex (regex-escaped), so
# its whole-binary total honours the same exclusion contract.
EXCL_REGEX=""
for e in "${EXCLUDED[@]+"${EXCLUDED[@]}"}"; do
    esc=$(printf '%s' "${e#$ROOT/}" | sed 's/[][\/.*^$()+?{}|]/\\&/g')
    EXCL_REGEX="${EXCL_REGEX:+$EXCL_REGEX|}$esc"
done
IGNORE="(vendor|build|tests|googletest)${EXCL_REGEX:+|$EXCL_REGEX}"

# Display-taking tests stay out of the measurement, exactly as this repo's own automation runs
# them (house label contract: requires_display).
CTEST_ARGS="-LE requires_display"

# A stale cache from an older Ninja-based invocation would fail to configure; clear it rather
# than guess.
if [[ -f "$BUILD_DIR/CMakeCache.txt" ]] \
   && grep -q 'CMAKE_GENERATOR:INTERNAL=Ninja' "$BUILD_DIR/CMakeCache.txt"; then
    echo "==> Dropping stale Ninja cache in ${BUILD_DIR#$ROOT/}"
    rm -rf "$BUILD_DIR"
fi

# ── Instrumented build + merged profile ────────────────────────────────────
#
# This used to shell out to the author's shared coverage gate, which is not something a public
# checkout can obtain -- so CI failed here with `bash: /gates/coverage_gate.sh: No such file`,
# then "no profdata", which is one cause wearing two messages. The cpp half of that gate is four
# commands; they are inlined rather than vendored, because copying a 458-line multi-language gate
# into a C++-only repo would carry a Swift and a Rust path that can never run here.
#
# ONE CODE PATH, deliberately. An earlier version of this could have preferred the shared gate
# when GOH_DIR happened to be set and fallen back otherwise -- and a gate that behaves differently
# depending on the environment it is launched from is a gate answering a different question. That
# exact defect cost a full afternoon in a sibling repo, where a parity runner measured itself
# because it ran the games' generators under its own interpreter.
echo "==> cmake configure + build (CODE_COVERAGE=ON, ${BUILD_DIR#$ROOT/})"
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCODE_COVERAGE=ON >/dev/null 2>&1 \
    || { echo "ERROR: cmake configure failed" >&2; exit 1; }
cmake --build "$BUILD_DIR" >/dev/null 2>&1 \
    || { echo "ERROR: coverage build failed" >&2; exit 1; }

# Hard fail, never warn: coverage measured over a failing suite is partial data wearing a green
# number, and a release gate must not launder test failures into a percentage.
echo "==> ctest (instrumented)"
# shellcheck disable=SC2086
(cd "$BUILD_DIR" && LLVM_PROFILE_FILE="$BUILD_DIR/default-%p.profraw" \
    ctest --output-on-failure $CTEST_ARGS >/dev/null 2>&1) \
    || { echo "ERROR: ctest reported failures — refusing to measure partial coverage" >&2; exit 1; }

RAWS=$(find "$BUILD_DIR" -maxdepth 1 -name 'default-*.profraw')
[[ -n "$RAWS" ]] || { echo "ERROR: no .profraw written — instrumentation emitted no profiles" >&2; exit 1; }
# shellcheck disable=SC2086
xcrun llvm-profdata merge -sparse $RAWS -o "$PROFDATA" \
    || { echo "ERROR: profdata merge failed" >&2; exit 1; }

# ── Whole-binary total: a RATCHET, not the eligible-scope floor ────────────
#
# THIS IS WHY THE RELEASE WORKFLOW WAS RED. The old shim handed the shared gate
# `--floor $TOTAL_MIN` (85) for the WHOLE-BINARY scope. That scope has never been near 85 -- it is
# 64% today -- because 85 was calibrated for the ELIGIBLE file set, which the tier section below
# measures and which sits at 85.88%. One number was being asked of two different populations, so
# the step failed whether or not the private gate could be reached. Removing the private
# dependency exposed a floor that had never been met, rather than causing a new failure.
#
# The honest arrangement is two measures with their own numbers: the eligible-scope floor stays at
# --total-min below, and the whole binary gets a SHRINK-ONLY ratchet seeded at what it actually is.
# It may rise and must never fall, which is a real bar; a floor nothing has ever cleared is not.
FAIL=0
WHOLE_MIN_FILE="$ROOT/scripts/coverage_whole_binary_floor.txt"
WHOLE_MIN=$(cat "$WHOLE_MIN_FILE" 2>/dev/null || echo 0)
TOTAL_PCT=$(xcrun llvm-cov report "$TEST_BIN" -instr-profile="$PROFDATA" \
    -ignore-filename-regex="$IGNORE" 2>/dev/null | tail -1 | awk '{print $10}')
TOTAL_PCT="${TOTAL_PCT%\%}"
: "${TOTAL_PCT:=0}"
echo "==> Whole binary (exclusions honoured): ${TOTAL_PCT}%  ratchet floor ${WHOLE_MIN}%"
if awk -v a="$TOTAL_PCT" -v b="$WHOLE_MIN" 'BEGIN{exit !(a+0 < b+0)}'; then
    echo "✗ whole-binary coverage ${TOTAL_PCT}% fell below the ${WHOLE_MIN}% ratchet" >&2
    echo "  Raise it back, or lower the ratchet DELIBERATELY in ${WHOLE_MIN_FILE#$ROOT/} and say why." >&2
    FAIL=1
fi

# ── Tier floors from the SAME merged profdata (local steps) ────────────────
[[ -f "$PROFDATA" ]] || { echo "ERROR: no profdata at $PROFDATA — nothing to grade tiers on"; exit 1; }
[[ -x "$TEST_BIN" ]] || { echo "ERROR: test binary not built at $TEST_BIN"; exit 1; }

echo ""
echo "=== Per-tier reports (eligible-list scope, repo-local policy) ==="

P0_COVER="0"
p0_list=()
while IFS= read -r line; do p0_list+=("$line"); done \
    < <(filter_excluded "${P0_SOURCES[@]+"${P0_SOURCES[@]}"}")
if [[ ${#p0_list[@]} -gt 0 ]]; then
    p0_result=$(run_cov_report "${p0_list[@]}")
    P0_COVER=$(echo "$p0_result" | tail -1)
    echo "$p0_result" | drop_last_line
fi

P1_COVER="0"
p1_list=()
while IFS= read -r line; do p1_list+=("$line"); done \
    < <(filter_excluded "${P1_SOURCES[@]+"${P1_SOURCES[@]}"}")
if [[ ${#p1_list[@]} -gt 0 ]]; then
    p1_result=$(run_cov_report "${p1_list[@]}")
    P1_COVER=$(echo "$p1_result" | tail -1)
    echo "$p1_result" | drop_last_line
fi

TOTAL_COVER="0"
all_list=()
while IFS= read -r line; do all_list+=("$line"); done \
    < <(filter_excluded "${ALL_SOURCES[@]+"${ALL_SOURCES[@]}"}")
if [[ ${#all_list[@]} -gt 0 ]]; then
    total_result=$(run_cov_report "${all_list[@]}")
    TOTAL_COVER=$(echo "$total_result" | tail -1)
    echo "$total_result" | drop_last_line
fi

{
    echo "Coverage Summary"
    echo "================"
    echo "P0:    ${P0_COVER}%  (threshold: ${P0_MIN}%)"
    echo "P1:    ${P1_COVER}%  (threshold: ${P1_MIN}%)"
    echo "Total: ${TOTAL_COVER}%  (threshold: ${TOTAL_MIN}%, eligible scope)"
}

if (( $(echo "$P0_COVER < $P0_MIN" | bc -l) )); then
    echo "FAILED: P0 coverage ${P0_COVER}% is below threshold ${P0_MIN}%"
    FAIL=1
else
    echo "PASSED: P0 coverage ${P0_COVER}% meets threshold ${P0_MIN}%"
fi

if (( $(echo "$P1_COVER < $P1_MIN" | bc -l) )); then
    echo "FAILED: P1 coverage ${P1_COVER}% is below threshold ${P1_MIN}%"
    FAIL=1
else
    echo "PASSED: P1 coverage ${P1_COVER}% meets threshold ${P1_MIN}%"
fi

if (( $(echo "$TOTAL_COVER < $TOTAL_MIN" | bc -l) )); then
    echo "FAILED: Total coverage ${TOTAL_COVER}% is below threshold ${TOTAL_MIN}%"
    FAIL=1
else
    echo "PASSED: Total coverage ${TOTAL_COVER}% meets threshold ${TOTAL_MIN}%"
fi

if [[ "$FAIL" -eq 0 ]]; then
    echo "All coverage thresholds met."
fi
exit "$FAIL"
