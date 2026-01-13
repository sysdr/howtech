#!/bin/bash
set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Testing generated binaries..."
echo "================================"
echo ""

# Test workload_debug
echo "1. Testing workload_debug..."
if ./workload_debug > /dev/null 2>&1; then
    echo "   ✓ workload_debug runs successfully"
else
    echo "   ✗ workload_debug failed"
    exit 1
fi

# Test workload_no_fp
echo "2. Testing workload_no_fp..."
if ./workload_no_fp > /dev/null 2>&1; then
    echo "   ✓ workload_no_fp runs successfully"
else
    echo "   ✗ workload_no_fp failed"
    exit 1
fi

# Test workload_stripped
echo "3. Testing workload_stripped..."
if ./workload_stripped > /dev/null 2>&1; then
    echo "   ✓ workload_stripped runs successfully"
else
    echo "   ✗ workload_stripped failed"
    exit 1
fi

# Test monitor (check if it exists and is executable)
echo "4. Testing monitor..."
if [ -x ./monitor ]; then
    echo "   ✓ monitor binary exists and is executable"
else
    echo "   ✗ monitor binary missing or not executable"
    exit 1
fi

echo ""
echo "All tests passed!"
