#!/bin/bash
# Standalone dashboard starter - ensures only one instance runs

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Kill any existing dashboards
pkill -9 -f "dashboard.py" 2>/dev/null
sleep 2

# Verify cleanup
REMAINING=$(pgrep -f "dashboard.py" | wc -l)
if [ "$REMAINING" -gt 0 ]; then
    echo "Force killing remaining processes..."
    pkill -9 -f "dashboard.py" 2>/dev/null
    sleep 1
fi

# Start dashboard
cd src
python3 dashboard.py > ../logs/dashboard.log 2>&1 &
DASH_PID=$!

# Wait for it to start
sleep 5

# Verify it's running and listening
if ps -p $DASH_PID > /dev/null 2>&1; then
    # Check if port is listening
    if ss -tlnp 2>/dev/null | grep -q ":8080 " || netstat -tlnp 2>/dev/null | grep -q ":8080 "; then
        # Test if it responds
        if curl -s http://127.0.0.1:8080/api/metrics > /dev/null 2>&1; then
            echo "✓ Dashboard started successfully (PID: $DASH_PID)"
            echo "✓ Dashboard available at: http://localhost:8080"
            exit 0
        fi
    fi
fi

echo "✗ Dashboard failed to start properly"
tail -20 ../logs/dashboard.log
exit 1

