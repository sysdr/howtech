#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}Starting Spinlock vs Mutex Demonstration...${NC}"
echo ""

# Check if executables exist
if [ ! -f "build/spinlock_test" ] || [ ! -f "build/mutex_test" ] || [ ! -f "build/monitor" ]; then
    echo -e "${YELLOW}Build files not found. Running setup...${NC}"
    bash setup.sh
fi

# Ensure logs directory exists
mkdir -p logs

# Kill any existing processes
pkill -f "spinlock_test" 2>/dev/null || true
pkill -f "mutex_test" 2>/dev/null || true
pkill -f "monitor" 2>/dev/null || true
sleep 1

echo -e "${YELLOW}>>> Starting Spinlock Test${NC}"
"$SCRIPT_DIR/build/spinlock_test" > logs/spinlock.log 2>&1 &
SPIN_PID=$!
echo "Spinlock PID: $SPIN_PID"

sleep 0.5

echo -e "${YELLOW}>>> Starting Mutex Test${NC}"
"$SCRIPT_DIR/build/mutex_test" > logs/mutex.log 2>&1 &
MUTEX_PID=$!
echo "Mutex PID: $MUTEX_PID"

sleep 0.5

echo -e "${YELLOW}>>> Starting Monitor${NC}"
"$SCRIPT_DIR/build/monitor" $SPIN_PID $MUTEX_PID &
MONITOR_PID=$!
echo "Monitor PID: $MONITOR_PID"

echo ""
echo -e "${GREEN}✓ All processes started!${NC}"
echo "PIDs saved to logs/pids.txt"
echo "$SPIN_PID" > logs/pids.txt
echo "$MUTEX_PID" >> logs/pids.txt
echo "$MONITOR_PID" >> logs/pids.txt

echo ""
echo "To stop all processes: ./stop.sh"
echo "To view logs: tail -f logs/*.log"
