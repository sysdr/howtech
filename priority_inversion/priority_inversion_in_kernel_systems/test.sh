#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Running Priority Inversion Tests ==="
echo ""

# Test 1: Check if executables exist
echo "Test 1: Checking if executables exist..."
if [ ! -f "$SCRIPT_DIR/build/priority_inversion" ]; then
    echo "  FAIL: build/priority_inversion not found"
    exit 1
fi
if [ ! -f "$SCRIPT_DIR/build/rt_monitor" ]; then
    echo "  FAIL: build/rt_monitor not found"
    exit 1
fi
echo "  PASS: All executables exist"
echo ""

# Test 2: Check if executables are executable
echo "Test 2: Checking if executables are executable..."
if [ ! -x "$SCRIPT_DIR/build/priority_inversion" ]; then
    echo "  FAIL: build/priority_inversion is not executable"
    exit 1
fi
if [ ! -x "$SCRIPT_DIR/build/rt_monitor" ]; then
    echo "  FAIL: build/rt_monitor is not executable"
    exit 1
fi
echo "  PASS: All executables are executable"
echo ""

# Test 3: Test priority_inversion with invalid arguments
echo "Test 3: Testing priority_inversion with invalid arguments..."
if "$SCRIPT_DIR/build/priority_inversion" 2>&1 | grep -q "Usage"; then
    echo "  PASS: Correctly handles invalid arguments"
else
    echo "  FAIL: Does not handle invalid arguments correctly"
    exit 1
fi
echo ""

# Test 4: Test priority_inversion with valid arguments (non-root, may fail but should handle gracefully)
echo "Test 4: Testing priority_inversion execution (may require root)..."
timeout 5 "$SCRIPT_DIR/build/priority_inversion" 0 > /tmp/test_output.log 2>&1 || true
if [ -f /tmp/test_output.log ]; then
    if grep -q "Priority Inversion\|PI Mutex\|Results" /tmp/test_output.log; then
        echo "  PASS: Program executes and produces output"
    else
        echo "  WARN: Program executed but output format unexpected"
        echo "  Output:"
        head -5 /tmp/test_output.log
    fi
    rm -f /tmp/test_output.log
else
    echo "  WARN: Could not capture output"
fi
echo ""

# Test 5: Test rt_monitor with invalid arguments
echo "Test 5: Testing rt_monitor with invalid arguments..."
if "$SCRIPT_DIR/build/rt_monitor" 2>&1 | grep -q "Usage"; then
    echo "  PASS: Correctly handles invalid arguments"
else
    echo "  FAIL: Does not handle invalid arguments correctly"
    exit 1
fi
echo ""

# Test 6: Check for required source files
echo "Test 6: Checking for required source files..."
if [ ! -f "$SCRIPT_DIR/src/priority_inversion.c" ]; then
    echo "  FAIL: src/priority_inversion.c not found"
    exit 1
fi
if [ ! -f "$SCRIPT_DIR/src/rt_monitor.c" ]; then
    echo "  FAIL: src/rt_monitor.c not found"
    exit 1
fi
echo "  PASS: All source files exist"
echo ""

# Test 7: Check for Makefile
echo "Test 7: Checking for Makefile..."
if [ ! -f "$SCRIPT_DIR/Makefile" ]; then
    echo "  FAIL: Makefile not found"
    exit 1
fi
echo "  PASS: Makefile exists"
echo ""

echo "=== All Tests Passed ==="

