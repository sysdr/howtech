#!/bin/bash

set -e

echo "=================================="
echo "SLAB Allocator Demo - Test Suite"
echo "=================================="

ERRORS=0

# Test 1: Check if build artifacts exist
echo ""
echo "Test 1: Checking build artifacts..."
if [ -f "build/allocator_benchmark" ]; then
    echo "  ✓ allocator_benchmark exists"
else
    echo "  ✗ allocator_benchmark missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "build/slab_monitor" ]; then
    echo "  ✓ slab_monitor exists"
else
    echo "  ✗ slab_monitor missing"
    ERRORS=$((ERRORS + 1))
fi

# Test 2: Run allocator benchmark
echo ""
echo "Test 2: Running allocator benchmark..."
if ./build/allocator_benchmark > /tmp/benchmark_output.txt 2>&1; then
    echo "  ✓ Benchmark executed successfully"
    if grep -q "malloc allocations" /tmp/benchmark_output.txt; then
        echo "  ✓ Benchmark output contains expected data"
    else
        echo "  ✗ Benchmark output incomplete"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo "  ✗ Benchmark failed"
    ERRORS=$((ERRORS + 1))
fi

# Test 3: Check if monitor binary is executable
echo ""
echo "Test 3: Checking monitor executable..."
if [ -x "build/slab_monitor" ]; then
    echo "  ✓ Monitor is executable"
else
    echo "  ✗ Monitor is not executable"
    ERRORS=$((ERRORS + 1))
fi

# Test 4: Check if /proc/slabinfo is accessible (for monitor)
echo ""
echo "Test 4: Checking /proc/slabinfo access..."
if [ -r "/proc/slabinfo" ]; then
    echo "  ✓ /proc/slabinfo is readable"
    SLABINFO_LINES=$(wc -l < /proc/slabinfo)
    echo "  ✓ Found $SLABINFO_LINES lines in /proc/slabinfo"
else
    echo "  ⚠ /proc/slabinfo requires root access (expected)"
fi

# Test 5: Check source files
echo ""
echo "Test 5: Checking source files..."
if [ -f "src/slab_demo.c" ]; then
    echo "  ✓ src/slab_demo.c exists"
else
    echo "  ✗ src/slab_demo.c missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "src/allocator_benchmark.c" ]; then
    echo "  ✓ src/allocator_benchmark.c exists"
else
    echo "  ✗ src/allocator_benchmark.c missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "monitor/slab_monitor.c" ]; then
    echo "  ✓ monitor/slab_monitor.c exists"
else
    echo "  ✗ monitor/slab_monitor.c missing"
    ERRORS=$((ERRORS + 1))
fi

# Test 6: Check scripts
echo ""
echo "Test 6: Checking scripts..."
for script in build.sh demo.sh monitor.sh; do
    if [ -f "$script" ] && [ -x "$script" ]; then
        echo "  ✓ $script exists and is executable"
    else
        echo "  ✗ $script missing or not executable"
        ERRORS=$((ERRORS + 1))
    fi
done

# Summary
echo ""
echo "=================================="
if [ $ERRORS -eq 0 ]; then
    echo "✓ All tests passed!"
    echo "=================================="
    exit 0
else
    echo "✗ $ERRORS test(s) failed"
    echo "=================================="
    exit 1
fi

