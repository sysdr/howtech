#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MONITOR_BIN="$SCRIPT_DIR/build/monitor"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║           Starting Spinlock Demo Services                    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if monitor binary exists
if [ ! -f "$MONITOR_BIN" ]; then
    echo -e "${RED}Error: Monitor binary not found at $MONITOR_BIN${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check if monitor is already running
if pgrep -f "build/monitor" > /dev/null; then
    echo -e "${YELLOW}Warning: Monitor is already running${NC}"
    echo -e "${YELLOW}Killing existing monitor process...${NC}"
    pkill -f "build/monitor"
    sleep 1
fi

# Start monitor in background
echo -e "${GREEN}Starting monitor in background...${NC}"
cd "$SCRIPT_DIR"
nohup ./build/monitor > /tmp/monitor.log 2>&1 &
MONITOR_PID=$!

sleep 2

# Verify monitor is running
if ps -p $MONITOR_PID > /dev/null; then
    echo -e "${GREEN}✓ Monitor started successfully (PID: $MONITOR_PID)${NC}"
    echo -e "${BLUE}Monitor output is being logged to /tmp/monitor.log${NC}"
    echo -e "${YELLOW}To view live output: tail -f /tmp/monitor.log${NC}"
    echo ""
    echo -e "${GREEN}Available demo commands:${NC}"
    echo "  ${BLUE}./build/spinlock_livelock${NC}  - Run spinlock contention demo"
    echo "  ${BLUE}./build/rcu_stall${NC}          - Run RCU grace period stall demo"
    echo ""
    echo -e "${GREEN}To stop monitor: pkill -f 'build/monitor'${NC}"
else
    echo -e "${RED}✗ Failed to start monitor${NC}"
    if [ -f /tmp/monitor.log ]; then
        echo -e "${YELLOW}Error log:${NC}"
        cat /tmp/monitor.log
    fi
    exit 1
fi

