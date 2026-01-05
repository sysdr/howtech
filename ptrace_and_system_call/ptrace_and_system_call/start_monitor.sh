#!/bin/bash
# Startup script for syscall monitoring service

set -eu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if monitor binary exists
if [ ! -f "$SCRIPT_DIR/monitor/syscall_monitor" ]; then
    echo -e "${RED}[ERROR]${NC} Monitor binary not found. Run setup.sh first."
    exit 1
fi

# Check if failing_app exists
if [ ! -f "$SCRIPT_DIR/build/failing_app" ]; then
    echo -e "${RED}[ERROR]${NC} Test app not found. Run setup.sh first."
    exit 1
fi

# Check for duplicate processes
MONITOR_PID=$(pgrep -f "syscall_monitor" | head -1)
if [ -n "$MONITOR_PID" ]; then
    echo -e "${YELLOW}[WARN]${NC} Monitor process already running (PID: $MONITOR_PID)"
    echo -e "${YELLOW}[WARN]${NC} Killing existing process..."
    kill "$MONITOR_PID" 2>/dev/null || true
    sleep 1
fi

echo -e "${GREEN}[INFO]${NC} Starting syscall monitor service..."
echo -e "${GREEN}[INFO]${NC} Monitor will track syscalls from failing_app"
echo -e "${YELLOW}[INFO]${NC} Press Ctrl+C to stop"

# Create logs directory if it doesn't exist
mkdir -p "$SCRIPT_DIR/logs"

# Run strace and save output to log (monitor needs interactive terminal, so we'll just log strace)
# For continuous monitoring, we'll run failing_app in a loop
nohup bash -c "
    while true; do
        strace -e status=failed -y -T ./build/failing_app 2>&1 >> \"$SCRIPT_DIR/logs/strace_output.log\" || true
        sleep 2
    done
" > "$SCRIPT_DIR/logs/monitor_background.log" 2>&1 &

STRACE_PID=$!
sleep 1
if ps -p "$STRACE_PID" > /dev/null 2>&1; then
    echo "$STRACE_PID" > "$SCRIPT_DIR/logs/monitor.pid"
else
    # Try to find the actual process
    STRACE_PID=$(pgrep -f "strace.*failing_app" | head -1)
    if [ -n "$STRACE_PID" ]; then
        echo "$STRACE_PID" > "$SCRIPT_DIR/logs/monitor.pid"
    else
        echo -e "${RED}[ERROR]${NC} Failed to start monitor"
        exit 1
    fi
fi

echo -e "${GREEN}[INFO]${NC} Monitor started with PID: $STRACE_PID"
echo -e "${GREEN}[INFO]${NC} Strace output is being logged to logs/strace_output.log"
echo -e "${GREEN}[INFO]${NC} Monitor is running. Check logs/ directory for output."
echo -e "${GREEN}[INFO]${NC} To stop: kill $STRACE_PID or run stop_monitor.sh"

