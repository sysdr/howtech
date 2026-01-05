#!/bin/bash
# Startup script for dashboard service

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}[ERROR]${NC} Python 3 is required but not found"
    exit 1
fi

# Check if Flask is installed
if ! python3 -c "import flask" 2>/dev/null; then
    echo -e "${YELLOW}[WARN]${NC} Flask not found. Installing..."
    pip3 install flask --user 2>/dev/null || {
        echo -e "${RED}[ERROR]${NC} Failed to install Flask. Please install manually: pip3 install flask"
        exit 1
    }
fi

# Check if dashboard.py exists
if [ ! -f "$SCRIPT_DIR/dashboard.py" ]; then
    echo -e "${RED}[ERROR]${NC} dashboard.py not found"
    exit 1
fi

# Check for duplicate processes
DASHBOARD_PID=$(pgrep -f "dashboard.py" | head -1)
if [ -n "$DASHBOARD_PID" ]; then
    echo -e "${YELLOW}[WARN]${NC} Dashboard already running (PID: $DASHBOARD_PID)"
    echo -e "${YELLOW}[WARN]${NC} Killing existing process..."
    kill "$DASHBOARD_PID" 2>/dev/null || true
    sleep 1
fi

echo -e "${GREEN}[INFO]${NC} Starting dashboard server..."
echo -e "${GREEN}[INFO]${NC} Dashboard will be available at http://localhost:5000"

# Create logs directory if it doesn't exist
mkdir -p "$SCRIPT_DIR/logs"

# Run dashboard in background
nohup python3 "$SCRIPT_DIR/dashboard.py" > "$SCRIPT_DIR/logs/dashboard.log" 2>&1 &

DASHBOARD_PID=$!
echo -e "${GREEN}[INFO]${NC} Dashboard started with PID: $DASHBOARD_PID"
echo "$DASHBOARD_PID" > "$SCRIPT_DIR/logs/dashboard.pid"

sleep 2
if ps -p "$DASHBOARD_PID" > /dev/null 2>&1; then
    echo -e "${GREEN}[INFO]${NC} Dashboard is running. Access at http://localhost:5000"
    echo -e "${GREEN}[INFO]${NC} To stop: kill $DASHBOARD_PID or run stop_dashboard.sh"
else
    echo -e "${RED}[ERROR]${NC} Dashboard failed to start. Check logs/dashboard.log"
    exit 1
fi

