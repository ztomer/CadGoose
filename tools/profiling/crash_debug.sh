#!/usr/bin/env bash
# crash_debug.sh — shim. The canonical harness lives in $GOH_DIR/tools/profiling
# (branch unify/profiling); fixes land there, never here.
GOH="${GOH_DIR:-}"
CANON="$GOH/tools/profiling/crash_debug.sh"
if [ ! -x "$CANON" ]; then
    echo "✗ crash_debug.sh: canonical harness missing under $GOH/tools/profiling (is GOH_DIR right?)" >&2
    exit 2
fi
exec "$CANON" "$@"
