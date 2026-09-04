#!/usr/bin/env bash
# Per-repo gate entry point. Declares which toolchains this repo contains and
# delegates; it holds no gate logic of its own.
#   --staged : pre-commit scope (fast) — layer 1 only
#   --full   : pre-push scope — every layer
set -euo pipefail
# The author's shared house-gate suite, if this machine has one. It is NOT part of this project:
# a contributor without it still gets the vendored gates below and a clean run, because a public
# repo may not require a checkout nobody else can obtain.
GOH="${GOH_DIR:-${GOH:-}}"

if [ -n "$GOH" ] && [ -x "$GOH/gates/structural.sh" ]; then
    "$GOH/gates/structural.sh" "$@"
else
    echo "· house gate suite not configured (set GOH_DIR); running this repo's own gates only"
fi

# Vendored, so it runs for everyone -- see tools/house_gates/.
python3 "$(dirname "${BASH_SOURCE[0]}")/house_gates/check_no_emoji.py"

case "${1:-}" in
  --full)
    # Add per-language layers for what this repo actually contains:
    #   "$GOH/gates/rust_gate.sh"  .
    #   "$GOH/gates/py_gate.sh"    .
    #   "$GOH/gates/swift_gate.sh" .
    # Layer 3 (genuinely local checks): create ./tools/repo_gates.sh and
    # uncomment:
    #   ./tools/repo_gates.sh
    ;;
esac
