#!/bin/bash
set -euo pipefail

echo "Running signal handling tests..."

# Test 1: Build verification
echo "[Test 1] Verifying build artifacts..."
if [ ! -f "build/signal_demo" ]; then
    echo "✗ ERROR: build/signal_demo not found"
    exit 1
fi
if [ ! -f "build/signal_monitor" ]; then
    echo "✗ ERROR: build/signal_monitor not found"
    exit 1
fi
echo "✓ Build artifacts exist"

# Test 2: Execute signal_demo
echo "[Test 2] Running signal_demo..."
if ! ./build/signal_demo > /tmp/signal_demo_output.txt 2>&1; then
    echo "✗ ERROR: signal_demo execution failed"
    cat /tmp/signal_demo_output.txt
    exit 1
fi
echo "✓ signal_demo executed successfully"

# Test 3: Check for expected output
echo "[Test 3] Verifying output..."
if ! grep -q "Signal Handler Invocation" /tmp/signal_demo_output.txt; then
    echo "✗ ERROR: Expected output not found"
    exit 1
fi
echo "✓ Output verification passed"

echo ""
echo "All tests passed!"
