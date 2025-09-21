#!/bin/bash

echo "🔄 Restarting GC Monitor..."

# Kill any existing processes on port 8080
echo "Stopping existing processes..."
lsof -ti:8080 | xargs kill -9 2>/dev/null || true
pkill -f integrated-monitor 2>/dev/null || true
pkill -f gc-monitor 2>/dev/null || true

# Wait a moment for cleanup
sleep 2

# Start the integrated monitor
echo "Starting integrated monitor..."
cd /Users/sumedhshende/sysd/howtech/Garbage_collection/gc-monitor
./bin/integrated-monitor &

# Wait for it to start
sleep 3

# Test if it's running
if curl -s http://localhost:8080/health > /dev/null; then
    echo "✅ Monitor started successfully on port 8080"
    echo "📊 Metrics: http://localhost:8080/metrics"
    echo "🔥 Load test: curl -X POST http://localhost:8080/loadtest"
else
    echo "❌ Failed to start monitor"
    exit 1
fi
