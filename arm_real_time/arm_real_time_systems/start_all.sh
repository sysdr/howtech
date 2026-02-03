#!/bin/bash
# Startup script to run all services

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Starting ARM Real-Time Demo Services"
echo "=========================================="

# Check for duplicate services
echo "Checking for duplicate services..."
LATENCY_RUNNING=$(pgrep -c -f "latency_test" || echo "0")
IRQ_RUNNING=$(pgrep -c -f "irq_monitor" || echo "0")
POLLUTER_RUNNING=$(pgrep -c -f "cache_polluter" || echo "0")

if [ "$LATENCY_RUNNING" -gt 0 ]; then
    echo "Warning: $LATENCY_RUNNING latency_test process(es) already running"
fi
if [ "$IRQ_RUNNING" -gt 0 ]; then
    echo "Warning: $IRQ_RUNNING irq_monitor process(es) already running"
fi
if [ "$POLLUTER_RUNNING" -gt 0 ]; then
    echo "Warning: $POLLUTER_RUNNING cache_polluter process(es) already running"
fi

# Start services
echo ""
echo "Starting services..."

# Start cache polluter on CPU 1 in background
if [ "$POLLUTER_RUNNING" -eq 0 ]; then
    echo "Starting cache polluter..."
    bash "$SCRIPT_DIR/start_cache_polluter.sh" 1
    sleep 2
fi

# Start IRQ monitor in background
if [ "$IRQ_RUNNING" -eq 0 ]; then
    echo "Starting IRQ monitor..."
    nohup bash "$SCRIPT_DIR/start_irq_monitor.sh" > "results/irq_monitor_$(date +%Y%m%d_%H%M%S).log" 2>&1 &
    sleep 1
fi

# Start latency test (foreground, will run for 10 seconds)
if [ "$LATENCY_RUNNING" -eq 0 ]; then
    echo "Starting latency test..."
    bash "$SCRIPT_DIR/start_latency_test.sh" 0 10
fi

echo ""
echo "=========================================="
echo "All services started"
echo "=========================================="
echo "Check running processes: ps aux | grep -E 'latency_test|irq_monitor|cache_polluter'"

