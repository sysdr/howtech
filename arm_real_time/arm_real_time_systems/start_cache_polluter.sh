#!/bin/bash
# Startup script for cache polluter

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "build/cache_polluter" ]; then
    echo "Error: build/cache_polluter not found. Run setup.sh first."
    exit 1
fi

CPU=${1:-1}

echo "Starting cache polluter on CPU $CPU..."
echo "Full path: $SCRIPT_DIR/build/cache_polluter"

# Check if already running
if pgrep -f "cache_polluter.*$CPU" > /dev/null; then
    echo "Warning: Cache polluter may already be running on CPU $CPU. Check with: ps aux | grep cache_polluter"
    read -p "Kill existing process? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f "cache_polluter.*$CPU"
        sleep 1
    else
        exit 1
    fi
fi

# Run in background
nohup "$SCRIPT_DIR/build/cache_polluter" "$CPU" > "results/cache_polluter_${CPU}_$(date +%Y%m%d_%H%M%S).log" 2>&1 &
POLLUTER_PID=$!

echo "Cache polluter started with PID: $POLLUTER_PID"
echo "To stop: kill $POLLUTER_PID"

