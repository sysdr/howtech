#!/bin/bash

echo "🚀 GC Monitoring Demo - Integrated Load Test"
echo "=============================================="
echo ""

# Check if integrated monitor is running
if ! curl -s http://localhost:8080/health > /dev/null; then
    echo "❌ Integrated monitor not running. Please start it first:"
    echo "   ./bin/integrated-monitor"
    exit 1
fi

echo "✅ Integrated monitor is running"
echo ""

# Start monitoring in background
echo "🔍 Starting real-time monitoring..."
./simple_monitor.sh &
MONITOR_PID=$!

# Wait a moment for monitor to start
sleep 2

echo ""
echo "🔥 Starting integrated load test..."
curl -X POST http://localhost:8080/loadtest

echo ""
echo "⏳ Monitoring for 45 seconds..."
sleep 45

# Stop monitoring
kill $MONITOR_PID 2>/dev/null

echo ""
echo "✅ Demo completed!"
echo ""
echo "💡 To run again:"
echo "   1. Start monitor: ./bin/integrated-monitor"
echo "   2. Run demo: ./demo.sh"
