#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Stopping kernel tracepoints demo..."

# Stop processes
pkill -f "build/target_program" 2>/dev/null || true
pkill -f "build/perf_monitor" 2>/dev/null || true

# Wait a bit
sleep 1

# Force kill if still running
pkill -9 -f "build/target_program" 2>/dev/null || true
pkill -9 -f "build/perf_monitor" 2>/dev/null || true

# Clean up PID files
rm -f logs/target.pid logs/monitor.pid

echo "Stopped."

