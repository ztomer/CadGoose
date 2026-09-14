#!/usr/bin/env bash
# Per-repo gate entry point. Declares which toolchains this repo contains and
# delegates; it holds no gate logic of its own.
#   --staged : pre-commit scope (fast) — layer 1 only
#   --full   : pre-push scope — every layer
set -euo pipefail
# The house gate suite (github.com/ztomer/gates_of_heck, public). Nothing is
# vendored: CI checks it out beside the tree, a contributor clones it once.
GOH="${GOH_DIR:-${GOH:-$HOME/Projects/gates_of_heck}}"
if [ ! -x "$GOH/gates/structural.sh" ]; then
    echo "✗ house gate suite not found at $GOH" >&2
    echo "  git clone https://github.com/ztomer/gates_of_heck && export GOH_DIR=\$PWD/gates_of_heck" >&2
    exit 2
fi
export GOH_DIR="$GOH"

case "${1:-}" in
  --full)
    # Every layer: the CI copy (build + tests + gates) from .gatesrc.
    exec "$(dirname "${BASH_SOURCE[0]}")/local_ci.sh"
    ;;
  *)
    exec "$GOH/gates/structural.sh" "$@"
    ;;
esac
