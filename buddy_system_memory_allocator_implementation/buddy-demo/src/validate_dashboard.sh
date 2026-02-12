#!/bin/bash
# Validation script for buddy_monitor dashboard

echo "=== Validating Buddy Monitor Dashboard ==="
echo

# Check if buddy_monitor exists
if [ ! -f ./buddy_monitor ]; then
    echo "ERROR: buddy_monitor not found"
    exit 1
fi

# Check if /proc/buddyinfo is accessible
if [ ! -r /proc/buddyinfo ]; then
    echo "ERROR: /proc/buddyinfo is not readable"
    exit 1
fi

echo "✓ buddy_monitor executable exists"
echo "✓ /proc/buddyinfo is accessible"

# Check if buddyinfo has data
BUDDYINFO_LINES=$(cat /proc/buddyinfo | wc -l)
if [ "$BUDDYINFO_LINES" -eq 0 ]; then
    echo "ERROR: /proc/buddyinfo is empty"
    exit 1
fi

echo "✓ /proc/buddyinfo has $BUDDYINFO_LINES zones"

# Verify buddyinfo format (should have Node and zone)
if ! grep -q "Node.*zone" /proc/buddyinfo; then
    echo "ERROR: /proc/buddyinfo format is unexpected"
    exit 1
fi

echo "✓ /proc/buddyinfo format is correct"

# Check if alloc_stress exists for testing
if [ ! -f ./alloc_stress ]; then
    echo "WARNING: alloc_stress not found (optional for testing)"
else
    echo "✓ alloc_stress test program exists"
fi

# Test that buddy_monitor can be executed (non-interactive check)
echo
echo "Testing buddy_monitor initialization..."
timeout 1 ./buddy_monitor 2>&1 > /dev/null
EXIT_CODE=$?
if [ $EXIT_CODE -eq 124 ] || [ $EXIT_CODE -eq 0 ]; then
    echo "✓ buddy_monitor can start (timeout expected for interactive program)"
else
    echo "ERROR: buddy_monitor failed to start (exit code: $EXIT_CODE)"
    exit 1
fi

echo
echo "=== Dashboard Validation Complete ==="
echo "All core metrics should be displayed:"
echo "  - Total Free Memory (MB)"
echo "  - Fragmentation Index (%)"
echo "  - Largest Free Block (MB)"
echo "  - Per-zone order blocks (O0-O10)"
echo
echo "To view the dashboard, run: ./buddy_monitor"
echo "Or from parent directory: ../demo.sh"

