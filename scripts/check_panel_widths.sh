#!/bin/bash
# Check that width values in preferences panel are computed at runtime, not hardcoded.
# Exits 0 if all clear, non-zero with violations listed.

ERRORS=0

echo "--- Preferences Runtime Width Check ---"
echo "Verifying no hardcoded pane-level widths remain in NSMakeRect calls."
echo ""

check_file() {
    local file="$1"
    grep -n 'NSMakeRect' "$file" | while read -r line; do
        local nums=$(echo "$line" | grep -oE '\b[0-9]{3,4}\b')
        for n in $nums; do
            # Flag any raw 3-4 digit number used as a width/coordinate
            # in the config_gui files (likely a hardcoded pane dimension)
            if [ "$n" -ge 300 ] && [ "$n" -le 700 ] 2>/dev/null; then
                # Skip known safe values (small widget sizes, reasonable positions)
                if [ "$n" -eq 260 ] || [ "$n" -eq 200 ] || [ "$n" -eq 130 ] || [ "$n" -eq 100 ] || [ "$n" -eq 80 ] || [ "$n" -eq 60 ] || [ "$n" -eq 44 ] || [ "$n" -eq 40 ] || [ "$n" -eq 36 ] || [ "$n" -eq 30 ] || [ "$n" -eq 24 ] || [ "$n" -eq 20 ] || [ "$n" -eq 16 ] || [ "$n" -eq 14 ]; then
                    continue
                fi
                echo "WARN: $file:$line"
                echo "  Suspicious hardcoded value: $n"
                ERRORS=$((ERRORS + 1))
            fi
        done
    done
}

echo "Checking src/platform/macos/config_gui*.mm..."
for f in src/platform/macos/config_gui.mm src/platform/macos/config_gui_detail.mm \
          src/platform/macos/config_gui_views.mm src/platform/macos/config_gui_ai.mm \
          src/platform/macos/config_gui_colors.mm; do
    check_file "$f"
done

echo ""
if [ "$ERRORS" -eq 0 ]; then
    echo "✓ All widths seem computed at runtime."
else
    echo "✗ $ERRORS potential width issues found (review above)."
fi
exit $ERRORS
