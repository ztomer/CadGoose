#!/usr/bin/env bash
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/_cadgoose.sh"
# quick_profile.sh - Quick 60-second CPU profile

DURATION=${1:-60}
pids="$(pgrep -f "CadGoose" || true)"   # whole, then the first: not a pipe (goh early-exit-pipe)
PID="$(head -n1 <<<"$pids")"

if [ -z "$PID" ]; then
    echo "Error: CadGoose not running"
    exit 1
fi

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="$(cg_root)/tools/profiling"
TRACE_FILE="${OUTPUT_DIR}/quick_${TIMESTAMP}.trace"

echo "Quick CPU Profile - 60 seconds (PID: $PID)"

xctrace record \
    --template "Time Profiler" \
    --duration $DURATION \
    --pid $PID \
    --output "$TRACE_FILE" \
    2>&1

echo ""
echo "Profile saved to: $TRACE_FILE"
echo ""
echo "Top symbols:"
if analysis="$(xctrace analyze --symbolicate --quiet "$TRACE_FILE" 2>/dev/null)"; then
    head -20 <<<"$analysis"   # read whole, not piped into head (goh early-exit-pipe)
else
    echo "Open in Instruments: open \"$TRACE_FILE\""
fi
