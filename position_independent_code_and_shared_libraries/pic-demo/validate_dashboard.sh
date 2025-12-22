#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Validating Dashboard Metrics                         ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

cd "$SCRIPT_DIR"

if [ ! -f "$BUILD_DIR/got_plt_monitor" ]; then
    echo "ERROR: got_plt_monitor not found!"
    exit 1
fi

# Capture dashboard output (suppress verbose library_function output)
DASHBOARD_OUTPUT=$("$BUILD_DIR/got_plt_monitor" 2>&1 | grep -v "Library function called" | tail -40)

VALIDATION_FAILED=0

# Check for required metrics
echo "[1/6] Checking PLT Call Overhead metric..."
if echo "$DASHBOARD_OUTPUT" | grep -q "PLT Call Overhead"; then
    echo "✓ PLT Call Overhead metric found"
else
    echo "✗ PLT Call Overhead metric MISSING"
    VALIDATION_FAILED=1
fi

echo ""
echo "[2/6] Checking CPU cycles metric..."
if echo "$DASHBOARD_OUTPUT" | grep -q "CPU cycles"; then
    echo "✓ CPU cycles metric found"
else
    echo "✗ CPU cycles metric MISSING"
    VALIDATION_FAILED=1
fi

echo ""
echo "[3/6] Checking PLT Mechanism Stages..."
if echo "$DASHBOARD_OUTPUT" | grep -q "PLT MECHANISM STAGES"; then
    echo "✓ PLT Mechanism Stages section found"
else
    echo "✗ PLT Mechanism Stages section MISSING"
    VALIDATION_FAILED=1
fi

echo ""
echo "[4/6] Checking Dynamic Linking Status..."
if echo "$DASHBOARD_OUTPUT" | grep -q "DYNAMIC LINKING STATUS"; then
    echo "✓ Dynamic Linking Status section found"
    if echo "$DASHBOARD_OUTPUT" | grep -q "Shared Libraries"; then
        echo "✓ Shared Libraries information found"
    else
        echo "✗ Shared Libraries information MISSING"
        VALIDATION_FAILED=1
    fi
else
    echo "✗ Dynamic Linking Status section MISSING"
    VALIDATION_FAILED=1
fi

echo ""
echo "[5/6] Checking Memory Efficiency section..."
if echo "$DASHBOARD_OUTPUT" | grep -q "MEMORY EFFICIENCY"; then
    echo "✓ Memory Efficiency section found"
else
    echo "✗ Memory Efficiency section MISSING"
    VALIDATION_FAILED=1
fi

echo ""
echo "[6/6] Checking Trade-offs section..."
if echo "$DASHBOARD_OUTPUT" | grep -q "TRADE-OFFS"; then
    echo "✓ Trade-offs section found"
    if echo "$DASHBOARD_OUTPUT" | grep -q "Cost:"; then
        echo "✓ Cost information found"
    else
        echo "✗ Cost information MISSING"
        VALIDATION_FAILED=1
    fi
    if echo "$DASHBOARD_OUTPUT" | grep -q "Benefit:"; then
        echo "✓ Benefit information found"
    else
        echo "✗ Benefit information MISSING"
        VALIDATION_FAILED=1
    fi
else
    echo "✗ Trade-offs section MISSING"
    VALIDATION_FAILED=1
fi

echo ""
if [ $VALIDATION_FAILED -eq 0 ]; then
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║  ✓ All Dashboard Metrics Validated Successfully!      ║"
    echo "╚════════════════════════════════════════════════════════╝"
    exit 0
else
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║  ✗ Dashboard Validation Failed                        ║"
    echo "╚════════════════════════════════════════════════════════╝"
    exit 1
fi

