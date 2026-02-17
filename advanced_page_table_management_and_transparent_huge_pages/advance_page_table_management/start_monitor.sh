#!/usr/bin/env bash
# Startup script for THP Monitor (Dashboard)
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MONITOR_BIN="$DEMO_DIR/build/monitor"

if [ ! -f "$MONITOR_BIN" ]; then
    echo "Error: Monitor binary not found at $MONITOR_BIN"
    echo "Please run: make all"
    exit 1
fi

if [ ! -x "$MONITOR_BIN" ]; then
    chmod +x "$MONITOR_BIN"
fi

# Check if running in interactive terminal
if [ ! -t 0 ]; then
    echo "Warning: Monitor requires an interactive terminal"
    echo "Run with: $MONITOR_BIN"
    exit 1
fi

echo "Starting THP Monitor (Dashboard) from: $MONITOR_BIN"
echo "Press 'q' or Ctrl+C to exit"
exec "$MONITOR_BIN" "$@"

