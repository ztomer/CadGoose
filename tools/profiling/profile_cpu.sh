#!/usr/bin/env bash
# profile_cpu.sh — shim. The canonical harness lives in house-gates/tools/profiling
# (branch unify/profiling); fixes land there, never here.
GOH="${GOH_DIR:-$GOH_DIR}"
CANON="$GOH/tools/profiling/profile_cpu.sh"
if [ ! -x "$CANON" ]; then
    echo "✗ profile_cpu.sh: canonical harness missing under $GOH/tools/profiling (is GOH_DIR right?)" >&2
    exit 2
fi
exec "$CANON" "$@"
