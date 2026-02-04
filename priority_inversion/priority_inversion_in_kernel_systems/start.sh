#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if executables exist
if [ ! -f "$SCRIPT_DIR/build/priority_inversion" ]; then
    echo "Error: $SCRIPT_DIR/build/priority_inversion not found. Run ./setup.sh first."
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/build/rt_monitor" ]; then
    echo "Error: $SCRIPT_DIR/build/rt_monitor not found. Run ./setup.sh first."
    exit 1
fi

# Check for duplicate services
check_duplicates() {
    # Check for actual executable processes, not script processes
    local count=$(pgrep -f "build/priority_inversion" | wc -l)
    if [ "$count" -gt 0 ]; then
        echo "Warning: Found $count existing priority_inversion process(es)"
        pgrep -f "build/priority_inversion" | xargs ps -p 2>/dev/null || true
        echo "Killing existing processes..."
        pkill -f "build/priority_inversion" || true
        sleep 1
    fi
    
    local monitor_count=$(pgrep -f "build/rt_monitor" | wc -l)
    if [ "$monitor_count" -gt 0 ]; then
        echo "Warning: Found $monitor_count existing rt_monitor process(es)"
        pgrep -f "build/rt_monitor" | xargs ps -p 2>/dev/null || true
        echo "Killing existing processes..."
        pkill -f "build/rt_monitor" || true
        sleep 1
    fi
}

check_duplicates

# Start the demo with monitoring
echo "Starting Priority Inversion Demo..."
echo "Press Ctrl+C to stop"

# Run demo in background and capture output
"$SCRIPT_DIR/build/priority_inversion" 1 > demo.log 2>&1 &
DEMO_PID=$!

# Give it a moment to start
sleep 1

# Start monitor
"$SCRIPT_DIR/build/rt_monitor" priority_inversion &
MONITOR_PID=$!

# Wait for demo to complete or user interrupt
trap "kill $DEMO_PID $MONITOR_PID 2>/dev/null; exit" INT TERM

wait $DEMO_PID
DEMO_EXIT=$?

# Kill monitor
kill $MONITOR_PID 2>/dev/null || true

if [ $DEMO_EXIT -eq 0 ]; then
    echo ""
    echo "Demo completed successfully!"
    echo "Check demo.log for output"
else
    echo ""
    echo "Demo exited with error code $DEMO_EXIT"
    echo "Check demo.log for details"
fi

