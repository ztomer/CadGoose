#!/usr/bin/env bash
# check_coverage.sh — delegating shim. The shared machinery lives in
# house-gates/gates/coverage_gate.sh (--lang cpp): instrumented
# configure/build, ctest under LLVM_PROFILE_FILE, profdata merge, and the
# TOTAL floor against --total-min.
#
# What the shared gate cannot express stays here as documented local steps:
#   - the per-tier floors over coverage_eligible.txt (P0 >= p0-min,
#     P1 >= p1-min), measured from the same profdata the shared gate merged;
#   - Total scoped to the ELIGIBLE file set (the shared gate's total covers
#     the whole test binary; that stricter whole-binary total is enforced by
#     the shared gate itself, with EXCLUDE-tier files appended to its ignore
#     regex so the explicit exclusion contract still holds).
#
# Flags preserved exactly (the CI workflow passes them):
#   ./scripts/check_coverage.sh [--p0-min=94] [--p1-min=53] [--total-min=85] [--build-dir=build-cov]
#
# Exit: 0 all floors met | 1 any floor missed | 2 usage/config error.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOH="${GOH_DIR:-$GOH_DIR}"
GATE="$GOH/gates/coverage_gate.sh"
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

# The shared gate joins its project root with GOH_CPP_BUILD_DIR, so the build
# dir seam must stay RELATIVE; the test-binary seam may be absolute.
export GOH_CPP_BUILD_DIR="${BUILD_DIR#$ROOT/}"
export GOH_CPP_TEST_BIN="$TEST_BIN"

# A stale cache from the old Ninja-based invocation would make the shared
# gate's own generator fail to configure; clear it rather than guess.
if [[ -f "$BUILD_DIR/CMakeCache.txt" ]] \
   && grep -q 'CMAKE_GENERATOR:INTERNAL=Ninja' "$BUILD_DIR/CMakeCache.txt"; then
    echo "==> Dropping stale Ninja cache in ${BUILD_DIR#$ROOT/} (shared gate configures its own generator)"
    rm -rf "$BUILD_DIR"
fi

# ── Shared machinery + whole-binary TOTAL floor ─────────────────────────────
FAIL=0
set +e
bash "$GATE" --lang cpp --floor "$TOTAL_MIN" --ignore "$IGNORE" "$ROOT"
GATE_RC=$?
set -e
[ "$GATE_RC" -ne 2 ] || exit 2
[ "$GATE_RC" -eq 0 ] || FAIL=1

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
