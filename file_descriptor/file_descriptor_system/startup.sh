#!/bin/bash
# Startup script for File Descriptor Tracking Demo
# Starts monitoring services and demo processes

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== File Descriptor Demo Startup ===${NC}\n"

# Check if binaries exist
if [ ! -f "build/fd_leak" ] || [ ! -f "build/fd_monitor" ]; then
    echo -e "${RED}Error: Binaries not found${NC}"
    echo "Please run setup.sh first to build the applications"
    exit 1
fi

# Check for existing processes
EXISTING_LEAK=$(pgrep -f "build/fd_leak" | head -1 || echo "")
EXISTING_MONITOR=$(pgrep -f "build/fd_monitor" | head -1 || echo "")

if [ -n "$EXISTING_LEAK" ] || [ -n "$EXISTING_MONITOR" ]; then
    echo -e "${YELLOW}Warning: Existing demo processes found${NC}"
    if [ -n "$EXISTING_LEAK" ]; then
        echo "  fd_leak PID: $EXISTING_LEAK"
    fi
    if [ -n "$EXISTING_MONITOR" ]; then
        echo "  fd_monitor PID: $EXISTING_MONITOR"
    fi
    echo "Killing existing processes..."
    pkill -f "build/fd_leak" 2>/dev/null || true
    pkill -f "build/fd_monitor" 2>/dev/null || true
    sleep 2
fi

# Start FD leak demo process
echo -e "${BLUE}Starting FD leak demo (30 file descriptors)...${NC}"
./build/fd_leak leak 30 > /tmp/fd_leak.log 2>&1 &
LEAK_PID=$!
sleep 2

if ! kill -0 $LEAK_PID 2>/dev/null; then
    echo -e "${RED}Failed to start fd_leak${NC}"
    cat /tmp/fd_leak.log
    exit 1
fi

echo -e "${GREEN}FD leak process started (PID: $LEAK_PID)${NC}"

# Start FD monitor
echo -e "${BLUE}Starting FD monitor...${NC}"
./build/fd_monitor watch $LEAK_PID > /tmp/fd_monitor.log 2>&1 &
MONITOR_PID=$!
sleep 1

if ! kill -0 $MONITOR_PID 2>/dev/null; then
    echo -e "${RED}Failed to start fd_monitor${NC}"
    cat /tmp/fd_monitor.log
    kill $LEAK_PID 2>/dev/null || true
    exit 1
fi

echo -e "${GREEN}FD monitor started (PID: $MONITOR_PID)${NC}"

# Save PIDs to file for cleanup
echo "$LEAK_PID" > /tmp/fd_demo_leak.pid
echo "$MONITOR_PID" > /tmp/fd_demo_monitor.pid

echo -e "\n${GREEN}Startup complete!${NC}"
echo ""
echo "Processes:"
echo "  FD Leak Demo: PID $LEAK_PID"
echo "  FD Monitor:   PID $MONITOR_PID"
echo ""
echo "To view logs:"
echo "  tail -f /tmp/fd_leak.log"
echo "  tail -f /tmp/fd_monitor.log"
echo ""
echo "To inspect FDs:"
echo "  lsof -p $LEAK_PID"
echo "  ls -la /proc/$LEAK_PID/fd/"
echo ""
echo "To stop:"
echo "  ./cleanup.sh"
echo "  or: kill $LEAK_PID $MONITOR_PID"

