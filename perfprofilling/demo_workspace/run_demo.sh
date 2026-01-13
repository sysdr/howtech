#!/bin/bash
set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Starting profiling demonstration..."
echo "===================================="
echo ""

# Run workload and capture output
echo "Running workload_debug..."
./workload_debug > workload_output.log 2>&1
WORKLOAD_EXIT=$?

echo ""
echo "Workload completed with exit code: $WORKLOAD_EXIT"
echo "Output:"
cat workload_output.log
echo ""

# Test monitor with a long-running process
echo "Testing monitor with a long-running process..."
sleep 2 > /dev/null &
SLEEP_PID=$!
sleep 0.1
if kill -0 $SLEEP_PID 2>/dev/null; then
    echo "Monitor test: Process $SLEEP_PID is running"
    echo "You can monitor it with: ./monitor $SLEEP_PID"
    kill $SLEEP_PID 2>/dev/null || true
    wait $SLEEP_PID 2>/dev/null || true
fi

echo ""
if [ $WORKLOAD_EXIT -eq 0 ]; then
    echo "✓ Demo completed successfully"
    exit 0
else
    echo "✗ Demo failed"
    exit 1
fi
