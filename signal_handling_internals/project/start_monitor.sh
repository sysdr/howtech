#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MONITOR_BIN="${SCRIPT_DIR}/build/signal_monitor"

if [ ! -f "$MONITOR_BIN" ]; then
    echo "ERROR: signal_monitor not found at $MONITOR_BIN"
    echo "Please run ./setup.sh first"
    exit 1
fi

# Check if already running
if pgrep -f "signal_monitor" > /dev/null; then
    echo "WARNING: signal_monitor appears to be already running"
    echo "Running processes:"
    ps aux | grep signal_monitor | grep -v grep
    read -p "Kill existing processes and start new? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f signal_monitor
        sleep 1
    else
        echo "Aborted"
        exit 1
    fi
fi

echo "Starting signal_monitor..."
cd "$SCRIPT_DIR"
exec "$MONITOR_BIN"
