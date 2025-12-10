#!/bin/bash
set -euo pipefail

echo "=== IPC Mechanisms Test Suite ==="
echo ""

# Test 1: Build all programs
echo "Test 1: Building all programs..."
make clean > /dev/null 2>&1
if make all > /dev/null 2>&1; then
    echo "✓ Build successful"
else
    echo "✗ Build failed"
    exit 1
fi

# Test 2: Run shared_memory
echo "Test 2: Running shared_memory benchmark..."
if timeout 5 ./shared_memory > /tmp/test_shm.txt 2>&1; then
    if grep -q "Throughput" /tmp/test_shm.txt; then
        echo "✓ Shared memory benchmark completed"
    else
        echo "✗ Shared memory benchmark failed"
        exit 1
    fi
else
    echo "✗ Shared memory benchmark timed out or failed"
    exit 1
fi

# Test 3: Run message_queue
echo "Test 3: Running message_queue benchmark..."
if timeout 10 ./message_queue > /tmp/test_mq.txt 2>&1; then
    if grep -q "Throughput" /tmp/test_mq.txt; then
        echo "✓ Message queue benchmark completed"
    else
        echo "✗ Message queue benchmark failed"
        exit 1
    fi
else
    echo "✗ Message queue benchmark timed out or failed"
    exit 1
fi

# Test 4: Monitor program exists and is executable
echo "Test 4: Checking monitor program..."
if [ -x ./monitor ]; then
    echo "✓ Monitor program exists and is executable"
else
    echo "✗ Monitor program missing or not executable"
    exit 1
fi

# Test 5: Verify all source files exist
echo "Test 5: Verifying source files..."
for file in src/shared_memory.c src/message_queue.c src/monitor.c; do
    if [ -f "$file" ]; then
        echo "  ✓ $file exists"
    else
        echo "  ✗ $file missing"
        exit 1
    fi
done

# Test 6: Verify Makefile and Dockerfile
echo "Test 6: Verifying build files..."
for file in Makefile Dockerfile; do
    if [ -f "$file" ]; then
        echo "  ✓ $file exists"
    else
        echo "  ✗ $file missing"
        exit 1
    fi
done

# Cleanup
rm -f /tmp/test_shm.txt /tmp/test_mq.txt

echo ""
echo "=== All tests passed! ==="

