#!/bin/bash

set -eo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=== Starting Kernel Tracepoints Demo ===${NC}\n"

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${RED}Error: build directory not found. Run setup.sh first.${NC}"
    exit 1
fi

# Check if executables exist
if [ ! -f "build/target_program" ]; then
    echo -e "${RED}Error: build/target_program not found. Run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "build/perf_monitor" ]; then
    echo -e "${RED}Error: build/perf_monitor not found. Run setup.sh first.${NC}"
    exit 1
fi

# Create logs directory if it doesn't exist
mkdir -p logs

# Clean up any existing processes
echo -e "${YELLOW}Checking for existing processes...${NC}"
pkill -f "build/target_program" 2>/dev/null || true
pkill -f "build/perf_monitor" 2>/dev/null || true
sleep 1

# Check for duplicate services
EXISTING_TARGET=$(pgrep -f "build/target_program" 2>/dev/null | wc -l || echo "0")
EXISTING_MONITOR=$(pgrep -f "build/perf_monitor" 2>/dev/null | wc -l || echo "0")
EXISTING_TARGET=${EXISTING_TARGET//[^0-9]/}
EXISTING_MONITOR=${EXISTING_MONITOR//[^0-9]/}

if [ "${EXISTING_TARGET:-0}" -gt 0 ] || [ "${EXISTING_MONITOR:-0}" -gt 0 ]; then
    echo -e "${YELLOW}Warning: Found existing processes. Cleaning up...${NC}"
    pkill -9 -f "build/target_program" 2>/dev/null || true
    pkill -9 -f "build/perf_monitor" 2>/dev/null || true
    sleep 2
fi

# Start target program
echo -e "${GREEN}[1/2] Starting target program...${NC}"
"$SCRIPT_DIR/build/target_program" > logs/target_output.log 2>&1 &
TARGET_PID=$!
echo "Target program PID: $TARGET_PID"
sleep 2

# Verify target program is running
if ! kill -0 $TARGET_PID 2>/dev/null; then
    echo -e "${RED}Error: Target program failed to start!${NC}"
    cat logs/target_output.log
    exit 1
fi

# Start perf monitor (requires root or CAP_PERFMON)
echo -e "${GREEN}[2/2] Starting perf monitor...${NC}"
if [ "$EUID" -eq 0 ] || capsh --print 2>/dev/null | grep -q "cap_perfmon"; then
    "$SCRIPT_DIR/build/perf_monitor" $TARGET_PID > logs/perf_monitor.log 2>&1 &
    MONITOR_PID=$!
    echo "Perf monitor PID: $MONITOR_PID"
    echo -e "${GREEN}Demo is running!${NC}"
    echo -e "${BLUE}Target program will run for 30 seconds.${NC}"
    echo -e "${BLUE}Logs: logs/target_output.log and logs/perf_monitor.log${NC}"
else
    echo -e "${YELLOW}Warning: Perf monitor requires root or CAP_PERFMON capability${NC}"
    echo -e "${YELLOW}Running without perf monitor.${NC}"
    MONITOR_PID=""
fi

# Save PIDs to file for cleanup
echo "$TARGET_PID" > logs/target.pid
[ ! -z "$MONITOR_PID" ] && echo "$MONITOR_PID" > logs/monitor.pid

echo -e "\n${GREEN}Startup complete!${NC}"
echo "To stop: pkill -f 'build/target_program' && pkill -f 'build/perf_monitor'"
echo "Or run: ./stop.sh"

