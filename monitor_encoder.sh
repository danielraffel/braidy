#!/bin/bash

echo "=========================================="
echo "Real-time Encoder Debug Monitor"
echo "=========================================="
echo ""
echo "Monitoring the latest debug log for encoder activity..."
echo "Look for:"
echo "  - [PARAM SYNC] entries (should only appear for host changes)"
echo "  - Algorithm changes"
echo "  - Any jumps back to CSAW"
echo ""

# Find the latest debug log
LOG_DIR="/Users/danielraffel/Library/Logs/Braidy"
if [ -d "$LOG_DIR" ]; then
    LATEST_LOG=$(ls -t "$LOG_DIR"/debug_*.log 2>/dev/null | head -1)
    
    if [ -n "$LATEST_LOG" ]; then
        echo "Monitoring: $LATEST_LOG"
        echo "Press Ctrl+C to stop monitoring"
        echo "=========================================="
        echo ""
        
        # Clear the log for a fresh start
        > "$LATEST_LOG"
        
        # Monitor the log in real-time
        tail -f "$LATEST_LOG" | grep -E "\[PARAM SYNC\]|\[DISPLAY\]|Algorithm changed|CSAW"
    else
        echo "No debug logs found in $LOG_DIR"
    fi
else
    echo "Debug log directory not found: $LOG_DIR"
fi