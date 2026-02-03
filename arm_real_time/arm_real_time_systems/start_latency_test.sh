#!/bin/bash
# Startup script for latency test

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "build/latency_test" ]; then
    echo "Error: build/latency_test not found. Run setup.sh first."
    exit 1
fi

CPU=${1:-0}
DURATION=${2:-10}

echo "Starting latency test on CPU $CPU for $DURATION seconds..."
echo "Full path: $SCRIPT_DIR/build/latency_test"

# Check if already running
if pgrep -f "latency_test.*$CPU" > /dev/null; then
    echo "Warning: Latency test may already be running. Check with: ps aux | grep latency_test"
fi

# Run the test
"$SCRIPT_DIR/build/latency_test" "$CPU" 2>&1 | tee "results/latency_test_$(date +%Y%m%d_%H%M%S).log"

echo "Latency test completed. Results saved to results/"

