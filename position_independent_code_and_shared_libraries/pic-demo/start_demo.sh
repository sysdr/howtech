#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Starting PIC/PIE Demonstration                       ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

cd "$SCRIPT_DIR"

# Check if executables exist
if [ ! -f "$BUILD_DIR/main-pie" ]; then
    echo "ERROR: main-pie not found. Please run 'make all' first."
    exit 1
fi

if [ ! -f "$BUILD_DIR/got_plt_monitor" ]; then
    echo "ERROR: got_plt_monitor not found. Please run 'make all' first."
    exit 1
fi

# Check for duplicate processes
echo "[1/3] Checking for duplicate demo processes..."
MAIN_PIE_PIDS=$(pgrep -f "main-pie" || true)
MONITOR_PIDS=$(pgrep -f "got_plt_monitor" || true)

if [ -n "$MAIN_PIE_PIDS" ]; then
    echo "WARNING: Found existing main-pie processes: $MAIN_PIE_PIDS"
    echo "Killing existing processes..."
    pkill -f "main-pie" || true
    sleep 1
fi

if [ -n "$MONITOR_PIDS" ]; then
    echo "WARNING: Found existing got_plt_monitor processes: $MONITOR_PIDS"
    echo "Killing existing processes..."
    pkill -f "got_plt_monitor" || true
    sleep 1
fi
echo "✓ No duplicate processes found (or cleaned up)"
echo ""

# Run main demo
echo "[2/3] Running main demo program..."
echo "─────────────────────────────────────────────────────────"
"$BUILD_DIR/main-pie"
echo ""

# Run dashboard/monitor
echo "[3/3] Running GOT/PLT Monitor (Dashboard)..."
echo "─────────────────────────────────────────────────────────"
"$BUILD_DIR/got_plt_monitor"
echo ""

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Demo Complete!                                        ║"
echo "╚════════════════════════════════════════════════════════╝"

