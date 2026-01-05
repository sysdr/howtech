#!/bin/bash
# Stop script for syscall monitoring service

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check for PID file
if [ -f "$SCRIPT_DIR/logs/monitor.pid" ]; then
    PID=$(cat "$SCRIPT_DIR/logs/monitor.pid")
    if ps -p "$PID" > /dev/null 2>&1; then
        echo -e "${GREEN}[INFO]${NC} Stopping monitor (PID: $PID)..."
        kill "$PID" 2>/dev/null || true
        # Kill any child processes
        pkill -P "$PID" 2>/dev/null || true
        rm -f "$SCRIPT_DIR/logs/monitor.pid"
        echo -e "${GREEN}[INFO]${NC} Monitor stopped"
    else
        echo -e "${YELLOW}[WARN]${NC} PID file exists but process not running"
        rm -f "$SCRIPT_DIR/logs/monitor.pid"
    fi
fi

# Also kill any strace processes related to failing_app
STRACE_PIDS=$(pgrep -f "strace.*failing_app" || true)
if [ -n "$STRACE_PIDS" ]; then
    echo -e "${GREEN}[INFO]${NC} Stopping strace processes: $STRACE_PIDS"
    echo "$STRACE_PIDS" | xargs kill 2>/dev/null || true
fi

# Kill any syscall_monitor processes
MONITOR_PIDS=$(pgrep -f "syscall_monitor" || true)
if [ -n "$MONITOR_PIDS" ]; then
    echo -e "${GREEN}[INFO]${NC} Stopping monitor processes: $MONITOR_PIDS"
    echo "$MONITOR_PIDS" | xargs kill 2>/dev/null || true
fi

