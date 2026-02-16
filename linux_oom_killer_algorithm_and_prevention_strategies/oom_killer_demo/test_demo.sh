#!/bin/bash
# Test script for OOM Killer Demo
set -e

WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/oom_killer_demo"
ERRORS=0

echo "=== Testing OOM Killer Demo ==="
echo ""

# Test 1: Check binaries exist
echo "Test 1: Checking binaries..."
if [ -f "$WORK_DIR/bin/oom_demo" ]; then
    echo "  ✓ oom_demo exists"
else
    echo "  ✗ oom_demo missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "$WORK_DIR/bin/oom_monitor" ]; then
    echo "  ✓ oom_monitor exists"
else
    echo "  ✗ oom_monitor missing"
    ERRORS=$((ERRORS + 1))
fi

# Test 2: Check demo runs
echo ""
echo "Test 2: Running oom_demo (quick test)..."
if timeout 5 "$WORK_DIR/bin/oom_demo" > /dev/null 2>&1; then
    echo "  ✓ oom_demo runs successfully"
else
    echo "  ✗ oom_demo failed"
    ERRORS=$((ERRORS + 1))
fi

# Test 3: Check monitor snapshot
echo ""
echo "Test 3: Running oom_monitor snapshot..."
if "$WORK_DIR/bin/oom_monitor" --snapshot > /dev/null 2>&1; then
    echo "  ✓ oom_monitor snapshot works"
else
    echo "  ✗ oom_monitor snapshot failed"
    ERRORS=$((ERRORS + 1))
fi

# Test 4: Check for duplicate services
echo ""
echo "Test 4: Checking for duplicate services..."
RUNNING=$(ps aux | grep -E "oom_demo|oom_monitor" | grep -v grep | wc -l)
if [ "$RUNNING" -eq 0 ]; then
    echo "  ✓ No duplicate services running"
else
    echo "  ⚠ Found $RUNNING running instance(s)"
fi

echo ""
if [ $ERRORS -eq 0 ]; then
    echo "=== All tests passed! ==="
    exit 0
else
    echo "=== $ERRORS test(s) failed ==="
    exit 1
fi
