#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}XDP Demo Runner${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}This script requires root privileges${NC}"
    echo "Please run with: sudo ./run_demo.sh"
    exit 1
fi

# Check if binaries exist
if [ ! -f "build/packet_gen" ]; then
    echo -e "${RED}Error: Packet generator not built${NC}"
    echo "Please run: make"
    exit 1
fi

# Check if XDP is loaded
if ! ip link show dev lo 2>/dev/null | grep -q xdp; then
    echo -e "${YELLOW}XDP program not loaded. Loading now...${NC}"
    ./startup.sh
fi

# Create output directory if it doesn't exist
mkdir -p output

# Get map FD
MAP_FD=$(bpftool map list 2>/dev/null | grep xdp_stats | awk '{print $1}' | cut -d: -f1 || echo "")

DURATION=${1:-10}
TARGET_IP=${2:-127.0.0.1}

echo -e "${BLUE}Running demo for $DURATION seconds...${NC}"
echo -e "${BLUE}Target IP: $TARGET_IP${NC}"
echo

# Start packet generator
timeout "$DURATION" build/packet_gen "$TARGET_IP" > output/packet_gen.log 2>&1 &
PG_PID=$!

# Start monitor if map FD available
if [ -n "$MAP_FD" ]; then
    sleep 1
    timeout $((DURATION - 1)) build/xdp_monitor "$MAP_FD" 2>/dev/null || true
fi

# Wait for packet generator
wait $PG_PID 2>/dev/null || true

echo
echo -e "${GREEN}Demo complete!${NC}"
