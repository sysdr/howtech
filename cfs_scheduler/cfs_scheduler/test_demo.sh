#!/bin/bash
# Test script for CFS Scheduler Demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== CFS Scheduler Demo Test ==="
echo

# Test 1: Verify all executables exist
echo "[Test 1] Checking executables..."
test -f "$SCRIPT_DIR/build/cfs_demo" || { echo "ERROR: cfs_demo not found"; exit 1; }
test -f "$SCRIPT_DIR/build/cfs_monitor" || { echo "ERROR: cfs_monitor not found"; exit 1; }
test -f "$SCRIPT_DIR/build/vruntime_logger" || { echo "ERROR: vruntime_logger not found"; exit 1; }
echo "✓ All executables exist"
echo

# Test 2: Verify source files exist
echo "[Test 2] Checking source files..."
test -f "$SCRIPT_DIR/src/cfs_demo.c" || { echo "ERROR: cfs_demo.c not found"; exit 1; }
test -f "$SCRIPT_DIR/src/cfs_monitor.c" || { echo "ERROR: cfs_monitor.c not found"; exit 1; }
test -f "$SCRIPT_DIR/src/vruntime_logger.c" || { echo "ERROR: vruntime_logger.c not found"; exit 1; }
echo "✓ All source files exist"
echo

# Test 3: Verify build artifacts
echo "[Test 3] Checking build artifacts..."
test -f "$SCRIPT_DIR/Makefile" || { echo "ERROR: Makefile not found"; exit 1; }
test -f "$SCRIPT_DIR/Dockerfile" || { echo "ERROR: Dockerfile not found"; exit 1; }
echo "✓ Build artifacts exist"
echo

# Test 4: Test vruntime_logger (quick test)
echo "[Test 4] Testing vruntime_logger..."
mkdir -p "$SCRIPT_DIR/output"
timeout 2s "$SCRIPT_DIR/build/vruntime_logger" 2>&1 | head -3 || true
test -f "$SCRIPT_DIR/output/vruntime_log.txt" || { echo "ERROR: vruntime_log.txt not generated"; exit 1; }
echo "✓ vruntime_logger works"
echo

# Test 5: Verify log file has content
echo "[Test 5] Verifying log file..."
if [ -s "$SCRIPT_DIR/output/vruntime_log.txt" ]; then
    echo "✓ Log file has content ($(wc -l < "$SCRIPT_DIR/output/vruntime_log.txt") lines)"
else
    echo "WARNING: Log file is empty"
fi
echo

# Test 6: Check for duplicate processes
echo "[Test 6] Checking for duplicate processes..."
PROCESS_COUNT=$(ps aux | grep -E "cfs_|vruntime" | grep -v grep | wc -l)
if [ "$PROCESS_COUNT" -eq 0 ]; then
    echo "✓ No duplicate processes running"
else
    echo "WARNING: Found $PROCESS_COUNT CFS-related processes"
    ps aux | grep -E "cfs_|vruntime" | grep -v grep
fi
echo

echo "=== All Tests Passed ==="

