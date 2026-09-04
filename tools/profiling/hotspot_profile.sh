#!/usr/bin/env bash
# hotspot_profile.sh — shim. The canonical harness lives in house-gates/tools/profiling
# (branch unify/profiling); fixes land there, never here.
GOH="${GOH_DIR:-$GOH_DIR}"
CANON="$GOH/tools/profiling/hotspot_profile.sh"
if [ ! -x "$CANON" ]; then
    echo "✗ hotspot_profile.sh: canonical harness missing under $GOH/tools/profiling (is GOH_DIR right?)" >&2
    exit 2
fi
exec "$CANON" "$@"
