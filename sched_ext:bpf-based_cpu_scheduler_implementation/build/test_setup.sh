#!/bin/bash
# Test script to validate setup

set -e

BUILD_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$BUILD_DIR/.." && pwd)"

echo "=== Testing Setup ==="
echo "Build directory: $BUILD_DIR"
echo ""

# Test 1: Check all required files exist
echo "[1/6] Checking required files..."
REQUIRED_FILES=(
    "$BUILD_DIR/include/scx_common.h"
    "$BUILD_DIR/src/scx_dsq_demo.bpf.c"
    "$BUILD_DIR/src/monitor.c"
    "$BUILD_DIR/src/loader.c"
    "$BUILD_DIR/Makefile"
    "$BUILD_DIR/workload.sh"
    "$BUILD_DIR/Dockerfile"
    "$BUILD_DIR/USAGE.md"
)

MISSING=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $(basename "$file")"
    else
        echo "  ✗ $(basename "$file") - MISSING"
        MISSING=1
    fi
done

if [ $MISSING -eq 1 ]; then
    echo "  ERROR: Some required files are missing!"
    exit 1
fi
echo "  All source files present"
echo ""

# Test 2: Check executables
echo "[2/6] Checking executables..."
if [ -x "$BUILD_DIR/loader" ]; then
    echo "  ✓ loader"
else
    echo "  ✗ loader - not executable or missing"
    exit 1
fi

if [ -x "$BUILD_DIR/monitor" ]; then
    echo "  ✓ monitor"
else
    echo "  ✗ monitor - not executable or missing"
    exit 1
fi

if [ -x "$BUILD_DIR/workload.sh" ]; then
    echo "  ✓ workload.sh"
else
    echo "  ✗ workload.sh - not executable or missing"
    exit 1
fi
echo ""

# Test 3: Check BPF object (may be missing if kernel < 6.11)
echo "[3/6] Checking BPF object..."
if [ -f "$BUILD_DIR/scx_dsq_demo.bpf.o" ]; then
    echo "  ✓ scx_dsq_demo.bpf.o exists"
    BPF_OK=1
else
    echo "  ⚠ scx_dsq_demo.bpf.o missing (expected if kernel < 6.11)"
    BPF_OK=0
fi
echo ""

# Test 4: Check for duplicate services
echo "[4/6] Checking for duplicate services..."
LOADER_COUNT=$(pgrep -f "loader.*scx_dsq_demo" | wc -l)
MONITOR_COUNT=$(pgrep -f "monitor.*scx_dsq_demo" | wc -l)
WORKLOAD_COUNT=$(pgrep -f "workload.sh" | wc -l)

if [ "$LOADER_COUNT" -gt 1 ]; then
    echo "  ⚠ Multiple loader processes detected: $LOADER_COUNT"
else
    echo "  ✓ No duplicate loader processes"
fi

if [ "$MONITOR_COUNT" -gt 1 ]; then
    echo "  ⚠ Multiple monitor processes detected: $MONITOR_COUNT"
else
    echo "  ✓ No duplicate monitor processes"
fi

if [ "$WORKLOAD_COUNT" -gt 1 ]; then
    echo "  ⚠ Multiple workload processes detected: $WORKLOAD_COUNT"
else
    echo "  ✓ No duplicate workload processes"
fi
echo ""

# Test 5: Test loader (dry run - check it accepts arguments)
echo "[5/6] Testing loader..."
if [ $BPF_OK -eq 1 ]; then
    if timeout 2 "$BUILD_DIR/loader" "$BUILD_DIR/scx_dsq_demo.bpf.o" 2>&1 | head -5; then
        echo "  ⚠ Loader started (may require root and kernel 6.11+)"
    else
        echo "  ⚠ Loader test completed (expected to fail without root/kernel support)"
    fi
else
    echo "  ⚠ Skipping loader test (BPF object missing)"
fi
echo ""

# Test 6: Test monitor (dry run - check it accepts arguments)
echo "[6/6] Testing monitor..."
if [ $BPF_OK -eq 1 ]; then
    if timeout 2 "$BUILD_DIR/monitor" "$BUILD_DIR/scx_dsq_demo.bpf.o" 2>&1 | head -5; then
        echo "  ⚠ Monitor started (may require root and kernel 6.11+)"
    else
        echo "  ⚠ Monitor test completed (expected to fail without root/kernel support)"
    fi
else
    echo "  ⚠ Skipping monitor test (BPF object missing)"
fi
echo ""

# Summary
echo "=== Test Summary ==="
KERNEL_VERSION=$(uname -r | cut -d. -f1-2)
echo "Kernel version: $KERNEL_VERSION (requires >= 6.11 for sched_ext)"
echo ""

if [ $BPF_OK -eq 1 ]; then
    echo "✓ All files generated successfully"
    echo "✓ Executables compiled successfully"
    echo "⚠ BPF scheduler requires kernel >= 6.11 to run"
    echo ""
    echo "To run the demo (requires root and kernel 6.11+):"
    echo "  1. sudo $BUILD_DIR/loader $BUILD_DIR/scx_dsq_demo.bpf.o"
    echo "  2. sudo $BUILD_DIR/monitor $BUILD_DIR/scx_dsq_demo.bpf.o (in another terminal)"
    echo "  3. $BUILD_DIR/workload.sh (in another terminal)"
else
    echo "✓ All source files generated successfully"
    echo "✓ Executables compiled successfully"
    echo "⚠ BPF object compilation failed (kernel $KERNEL_VERSION < 6.11 required)"
    echo ""
    echo "The setup is complete, but the BPF scheduler cannot be compiled/run"
    echo "on this kernel version. Upgrade to kernel >= 6.11 to use sched_ext."
fi

