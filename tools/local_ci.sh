#!/usr/bin/env bash
# local_ci.sh — run the gates both workflows enforce, here, before pushing.
#
# UNLIKE most sibling repos, CI here is ALIVE and stays that way. CadGoose is a
# PUBLIC repo, so GitHub Actions minutes are free and its runs succeed — the
# credit exhaustion that killed CI on the private repos never applied. More than
# that, build_and_release.yml is the RELEASE PIPELINE: it builds the DMG,
# publishes the release, and scripts/push_release.sh polls for that run before
# updating the homebrew tap. Disabling it would not tidy anything, it would stop
# releases working.
#
# So this script is not a replacement for CI. It is the fast pre-push copy of
# CI's gates, so a red build is found in minutes on this machine instead of
# after a push. CI remains the authority.
#
# The step LIST lives in .gatesrc as GOH_CI_STEPS; the shared runner
# (house-gates/gates/local_ci.sh) owns the skeleton — fail accumulator,
# log-on-failure, Kare styling — that used to be copy-pasted per repo.
#
#   ./tools/local_ci.sh              steps from .gatesrc (gates + build + tests)
#   ./tools/local_ci.sh --coverage   also the coverage gate (slow: separate build)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOH="${GOH_DIR:-$GOH_DIR}"

ARGS=()
case "${1:-}" in
    --coverage) ARGS+=(--step 'bash scripts/check_coverage.sh --p0-min=94 --p1-min=53 --total-min=85 --build-dir=build-cov') ;;
    -h|--help)  echo "usage: ./tools/local_ci.sh [--coverage]"; exit 0 ;;
    "")         ;;
    *)          echo "✗ unknown argument: $1" >&2; exit 2 ;;
esac

if [ "${1:-}" != "--coverage" ]; then
    printf '%s\n' \
        "⚠ coverage gate skipped — pass --coverage (it needs its own instrumented build)" >&2
fi

# The shared runner executes step commands in the invocation CWD; every step
# in GOH_CI_STEPS is repo-root-relative.
cd "$ROOT"
exec "$GOH/gates/local_ci.sh" "${ARGS[@]+"${ARGS[@]}"}" "$ROOT"
