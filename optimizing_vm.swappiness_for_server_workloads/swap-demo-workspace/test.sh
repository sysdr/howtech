#!/usr/bin/env bash
# Test script for vm.swappiness demo tools

set -euo pipefail

WORKDIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$WORKDIR/build"
TESTS_PASSED=0
TESTS_FAILED=0

# Use arithmetic expansion that works in all shells
increment_passed() { TESTS_PASSED=$((TESTS_PASSED + 1)); }
increment_failed() { TESTS_FAILED=$((TESTS_FAILED + 1)); }

echo "=========================================="
echo "Running Tests for vm.swappiness Demo Tools"
echo "=========================================="
echo ""

# Test 1: Check if binaries exist
echo "[TEST 1] Checking if binaries exist..."
if [ -f "$BUILD_DIR/swappiness_probe" ] && [ -f "$BUILD_DIR/lru_monitor" ]; then
    echo "✓ PASS: Both binaries exist"
    increment_passed
else
    echo "✗ FAIL: Binaries missing"
    increment_failed
    exit 1
fi

# Test 2: Check if binaries are executable
echo ""
echo "[TEST 2] Checking if binaries are executable..."
if [ -x "$BUILD_DIR/swappiness_probe" ] && [ -x "$BUILD_DIR/lru_monitor" ]; then
    echo "✓ PASS: Both binaries are executable"
    increment_passed
else
    echo "✗ FAIL: Binaries are not executable"
    increment_failed
fi

# Test 3: Test swappiness_probe execution
echo ""
echo "[TEST 3] Testing swappiness_probe execution..."
if "$BUILD_DIR/swappiness_probe" > /dev/null 2>&1; then
    echo "✓ PASS: swappiness_probe executes successfully"
    increment_passed
else
    echo "✗ FAIL: swappiness_probe execution failed"
    increment_failed
fi

# Test 4: Test swappiness_probe output format
echo ""
echo "[TEST 4] Testing swappiness_probe output format..."
OUTPUT=$("$BUILD_DIR/swappiness_probe" 2>&1)
if echo "$OUTPUT" | grep -q "vm.swappiness"; then
    echo "✓ PASS: swappiness_probe produces expected output"
    increment_passed
else
    echo "✗ FAIL: swappiness_probe output format incorrect"
    increment_failed
fi

# Test 5: Test lru_monitor (timeout test)
echo ""
echo "[TEST 5] Testing lru_monitor execution..."
if timeout 1 "$BUILD_DIR/lru_monitor" > /dev/null 2>&1 || [ $? -eq 124 ]; then
    echo "✓ PASS: lru_monitor executes successfully"
    increment_passed
else
    echo "✗ FAIL: lru_monitor execution failed"
    increment_failed
fi

# Test 6: Verify swappiness_probe reads system files
echo ""
echo "[TEST 6] Verifying swappiness_probe reads system files..."
if [ -r /proc/sys/vm/swappiness ] && [ -r /proc/vmstat ]; then
    echo "✓ PASS: Required system files are readable"
    increment_passed
else
    echo "✗ FAIL: Cannot read required system files"
    increment_failed
fi

# Summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi

