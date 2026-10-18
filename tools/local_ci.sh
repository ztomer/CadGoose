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
# (the shared house suite's gates/local_ci.sh) owns the skeleton — fail accumulator,
# log-on-failure, Kare styling — that used to be copy-pasted per repo.
#
#   ./tools/local_ci.sh              every step in .gatesrc, coverage included
#
# There is no opt-in coverage flag any more (G0.16): the coverage gate is a
# GOH_CI_STEPS step like the rest, because a gate CI enforces and the pre-push
# hook skips is how a push passed 12 local steps and went red in CI.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOH="${GOH_DIR:-}"

case "${1:-}" in
    -h|--help)  echo "usage: ./tools/local_ci.sh"; exit 0 ;;
    "")         ;;
    *)          echo "✗ unknown argument: $1" >&2; exit 2 ;;
esac

# The shared runner executes step commands in the invocation CWD; every step
# in GOH_CI_STEPS is repo-root-relative.
cd "$ROOT"
exec "$GOH/gates/local_ci.sh" "$ROOT"
