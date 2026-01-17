#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=== Process Monitor Demo ===${NC}"
echo ""

# Check if build exists
if [ ! -f "$SCRIPT_DIR/build/monitor" ]; then
    echo -e "${YELLOW}Build not found. Building project...${NC}"
    make
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
fi

# Show system info
echo -e "${BLUE}System Information:${NC}"
echo "CPUs: $(nproc)"
echo "Clock ticks: $(getconf CLK_TCK)"
echo "Page size: $(getconf PAGESIZE)"
echo ""

# Demonstrate /proc inspection
echo -e "${YELLOW}=== Demonstrating /proc Filesystem ===${NC}"
echo "Inspecting current shell process (PID $$):"
echo ""
"$SCRIPT_DIR/build/proc_inspector" $$
echo ""

read -p "Press Enter to start stress test and monitor..."

# Start stress test in background
echo -e "${YELLOW}Starting stress test...${NC}"
"$SCRIPT_DIR/build/stress" 2 2 1 &
STRESS_PID=$!

sleep 1

echo ""
echo -e "${BLUE}Stress test running (PID: $STRESS_PID)${NC}"
echo "Inspect it: $SCRIPT_DIR/build/proc_inspector $STRESS_PID"
echo ""
echo -e "${GREEN}Launching monitor in 3 seconds...${NC}"
echo "Press 'q' or Ctrl+C in monitor to exit"
sleep 3

# Run monitor
"$SCRIPT_DIR/build/monitor"

# Cleanup stress test
echo ""
echo -e "${YELLOW}Cleaning up stress test...${NC}"
kill $STRESS_PID 2>/dev/null || true
wait $STRESS_PID 2>/dev/null || true

echo -e "${GREEN}Demo complete!${NC}"

