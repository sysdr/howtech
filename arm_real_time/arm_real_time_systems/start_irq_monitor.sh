#!/bin/bash
# Startup script for IRQ monitor

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "build/irq_monitor" ]; then
    echo "Error: build/irq_monitor not found. Run setup.sh first."
    exit 1
fi

echo "Starting IRQ monitor..."
echo "Full path: $SCRIPT_DIR/build/irq_monitor"

# Check if already running
if pgrep -f "irq_monitor" > /dev/null; then
    echo "Warning: IRQ monitor may already be running. Check with: ps aux | grep irq_monitor"
    read -p "Kill existing process? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f "irq_monitor"
        sleep 1
    else
        exit 1
    fi
fi

# Run the monitor
"$SCRIPT_DIR/build/irq_monitor"

