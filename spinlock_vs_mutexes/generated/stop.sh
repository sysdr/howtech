#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Stopping all demonstration processes..."

if [ -f "logs/pids.txt" ]; then
    while read pid; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done < logs/pids.txt
    rm -f logs/pids.txt
fi

pkill -f "spinlock_test" 2>/dev/null || true
pkill -f "mutex_test" 2>/dev/null || true
pkill -f "monitor" 2>/dev/null || true

echo "All processes stopped."
