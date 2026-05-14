#!/bin/bash
# quick_profile.sh - Quick 60-second CPU profile

DURATION=${1:-60}
PID=$(pgrep -f "CadGoose" | head -1)

if [ -z "$PID" ]; then
    echo "Error: CadGoose not running"
    exit 1
fi

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="/Users/ztomer/Projects/CadGoose/tools/profiling"
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
xctrace analyze --symbolicate --quiet "$TRACE_FILE" 2>/dev/null | head -20 || echo "Open in Instruments: open \"$TRACE_FILE\""