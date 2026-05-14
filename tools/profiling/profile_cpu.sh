#!/bin/bash
# profile_cpu.sh - CPU profiling with Time Profiler

DURATION=${2:-30}
PID=$1

if [ -z "$PID" ]; then
    PID=$(pgrep -f "CadGoose" | head -1)
fi

if [ -z "$PID" ]; then
    echo "Error: CadGoose not running. Pass PID or start CadGoose first."
    exit 1
fi

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="/Users/ztomer/Projects/CadGoose/tools/profiling"
TRACE_FILE="${OUTPUT_DIR}/cpu_profile_${TIMESTAMP}.trace"

echo "Profiling CPU for $DURATION seconds (PID: $PID)..."
echo "Saving to: $TRACE_FILE"

xctrace record \
    --template "Time Profiler" \
    --duration $DURATION \
    --pid $PID \
    --output "$TRACE_FILE" \
    2>&1

if [ -f "$TRACE_FILE" ]; then
    echo ""
    echo "=== Top CPU Consumers ==="
    xctrace analyze --detailed "$TRACE_FILE" 2>/dev/null | head -30 || \
    echo "Profile saved to: $TRACE_FILE"
    echo "Open in Instruments: open \"$TRACE_FILE\""
else
    echo "Error: Profile failed"
    exit 1
fi