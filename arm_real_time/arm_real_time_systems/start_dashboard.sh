#!/bin/bash
# Startup script for dashboard

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "dashboard.py" ]; then
    echo "Error: dashboard.py not found."
    exit 1
fi

# Check if already running
if pgrep -f "dashboard.py" > /dev/null; then
    echo "Warning: Dashboard may already be running. Check with: ps aux | grep dashboard"
    read -p "Kill existing process? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f "dashboard.py"
        sleep 1
    else
        exit 1
    fi
fi

# Install dependencies if needed
if ! python3 -c "import flask" 2>/dev/null; then
    echo "Installing Flask dependencies..."
    python3 -m pip install --user flask flask-cors
fi

echo "Starting dashboard..."
echo "Full path: $SCRIPT_DIR/dashboard.py"
echo "Dashboard will be available at: http://localhost:8080"

# Run dashboard in background
nohup python3 "$SCRIPT_DIR/dashboard.py" > "$SCRIPT_DIR/results/dashboard.log" 2>&1 &
DASHBOARD_PID=$!

sleep 2

# Check if it started successfully
if ps -p $DASHBOARD_PID > /dev/null; then
    echo "Dashboard started with PID: $DASHBOARD_PID"
    echo "To stop: kill $DASHBOARD_PID"
    echo "Logs: $SCRIPT_DIR/results/dashboard.log"
else
    echo "Error: Dashboard failed to start. Check logs: $SCRIPT_DIR/results/dashboard.log"
    exit 1
fi

