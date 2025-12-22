#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Running PIC/PIE Demo Tests                           ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

cd "$SCRIPT_DIR"

# Test 1: Verify main-pie works
echo "[TEST 1] Testing main-pie executable..."
if [ ! -f "$BUILD_DIR/main-pie" ]; then
    echo "ERROR: main-pie not found!"
    exit 1
fi
OUTPUT=$("$BUILD_DIR/main-pie" 2>&1)
if echo "$OUTPUT" | grep -q "Library function called"; then
    echo "✓ main-pie test passed"
else
    echo "ERROR: main-pie test failed"
    echo "$OUTPUT"
    exit 1
fi
echo ""

# Test 2: Verify got_plt_monitor works and shows metrics
echo "[TEST 2] Testing got_plt_monitor (dashboard metrics)..."
if [ ! -f "$BUILD_DIR/got_plt_monitor" ]; then
    echo "ERROR: got_plt_monitor not found!"
    exit 1
fi
MONITOR_OUTPUT=$("$BUILD_DIR/got_plt_monitor" 2>&1)
if echo "$MONITOR_OUTPUT" | grep -q "PLT Call Overhead"; then
    echo "✓ got_plt_monitor shows PLT overhead metrics"
else
    echo "ERROR: got_plt_monitor metrics not found"
    exit 1
fi
if echo "$MONITOR_OUTPUT" | grep -q "CPU cycles"; then
    echo "✓ got_plt_monitor shows CPU cycle metrics"
else
    echo "ERROR: CPU cycle metrics not found"
    exit 1
fi
if echo "$MONITOR_OUTPUT" | grep -q "Shared Libraries"; then
    echo "✓ got_plt_monitor shows shared library info"
else
    echo "ERROR: Shared library info not found"
    exit 1
fi
echo ""

# Test 3: Verify reloc_analyzer works
echo "[TEST 3] Testing reloc_analyzer..."
if [ ! -f "$BUILD_DIR/reloc_analyzer" ]; then
    echo "ERROR: reloc_analyzer not found!"
    exit 1
fi
RELOC_OUTPUT=$("$BUILD_DIR/reloc_analyzer" "$BUILD_DIR/main-pie" 2>&1)
if echo "$RELOC_OUTPUT" | grep -q "RELOCATION TYPE"; then
    echo "✓ reloc_analyzer works correctly"
else
    echo "ERROR: reloc_analyzer failed"
    echo "$RELOC_OUTPUT"
    exit 1
fi
echo ""

# Test 4: Verify shared library is loaded correctly
echo "[TEST 4] Verifying shared library loading..."
if ldd "$BUILD_DIR/main-pie" | grep -q "libdemo-pic.so"; then
    echo "✓ Shared library libdemo-pic.so is correctly linked"
else
    echo "ERROR: Shared library not linked correctly"
    exit 1
fi
echo ""

echo "╔════════════════════════════════════════════════════════╗"
echo "║  All Tests Passed! ✓                                  ║"
echo "╚════════════════════════════════════════════════════════╝"

