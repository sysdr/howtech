#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

SCRIPT_DIR="$(pwd)"
PORT=${PORT:-8080}

# Check if already running
check_running() {
    if pgrep -f "python3.*http.server.*$PORT" > /dev/null || pgrep -f "metrics_collector.sh" > /dev/null; then
        echo "Warning: Services may already be running"
        echo "Checking processes..."
        pgrep -af "http.server\|metrics_collector" || true
        echo "Killing existing processes..."
        pkill -f "python3.*http.server.*$PORT" 2>/dev/null || true
        pkill -f "metrics_collector.sh" 2>/dev/null || true
        sleep 1
    fi
}

check_running

echo "Starting TSAN Demo Services..."
echo "================================"
echo

# Start metrics collector in background (runs every 10 seconds)
echo "Starting metrics collector (updates every 10 seconds)..."
(
    while true; do
        "$SCRIPT_DIR/metrics_collector.sh" >> /tmp/metrics_collector.log 2>&1
        sleep 10
    done
) &
METRICS_PID=$!
echo "Metrics collector PID: $METRICS_PID"

# Wait a bit for metrics to be collected
sleep 2

# Start web server for dashboard
echo "Starting web server on port $PORT..."
cd "$SCRIPT_DIR"
python3 -m http.server "$PORT" > /tmp/dashboard_server.log 2>&1 &
SERVER_PID=$!
echo "Web server PID: $SERVER_PID"

echo
echo "Services started!"
echo "=================="
echo "Dashboard: http://localhost:$PORT/dashboard.html"
echo "Metrics: http://localhost:$PORT/metrics.json"
echo
echo "To stop services, run: pkill -f 'http.server.*$PORT' && pkill -f metrics_collector.sh"
echo "Or use: ./stop.sh"
echo

# Create stop script
cat > stop.sh << STOPEOF
#!/bin/bash
pkill -f "python3.*http.server.*$PORT" 2>/dev/null || true
pkill -f "metrics_collector.sh" 2>/dev/null || true
echo "Services stopped"
STOPEOF
chmod +x stop.sh
