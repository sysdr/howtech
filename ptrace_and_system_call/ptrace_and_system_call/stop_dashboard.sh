#!/bin/bash
# Stop script for dashboard service

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check for PID file
if [ -f "$SCRIPT_DIR/logs/dashboard.pid" ]; then
    PID=$(cat "$SCRIPT_DIR/logs/dashboard.pid")
    if ps -p "$PID" > /dev/null 2>&1; then
        echo -e "${GREEN}[INFO]${NC} Stopping dashboard (PID: $PID)..."
        kill "$PID" 2>/dev/null || true
        rm -f "$SCRIPT_DIR/logs/dashboard.pid"
        echo -e "${GREEN}[INFO]${NC} Dashboard stopped"
    else
        echo -e "${YELLOW}[WARN]${NC} PID file exists but process not running"
        rm -f "$SCRIPT_DIR/logs/dashboard.pid"
    fi
else
    # Try to find and kill by process name
    PIDS=$(pgrep -f "dashboard.py" || true)
    if [ -n "$PIDS" ]; then
        echo -e "${GREEN}[INFO]${NC} Stopping dashboard processes: $PIDS"
        echo "$PIDS" | xargs kill 2>/dev/null || true
        echo -e "${GREEN}[INFO]${NC} Dashboard stopped"
    else
        echo -e "${YELLOW}[WARN]${NC} No dashboard process found"
    fi
fi

