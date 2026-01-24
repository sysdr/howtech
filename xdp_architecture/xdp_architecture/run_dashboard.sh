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
echo -e "${BLUE}XDP Dashboard${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check if monitor binary exists
if [ ! -f "build/xdp_monitor" ]; then
    echo -e "${RED}Error: Monitor binary not found${NC}"
    echo "Please run: make"
    exit 1
fi

# Check if XDP is loaded
if ! ip link show dev lo 2>/dev/null | grep -q xdp; then
    echo -e "${RED}Error: XDP program not loaded${NC}"
    echo "Please run ./startup.sh first"
    exit 1
fi

# Get map FD
MAP_FD=$(bpftool map list 2>/dev/null | grep xdp_stats | awk '{print $1}' | cut -d: -f1 || echo "")

if [ -z "$MAP_FD" ]; then
    echo -e "${RED}Error: Could not find statistics map${NC}"
    exit 1
fi

echo -e "${GREEN}Starting XDP statistics monitor...${NC}"
echo "Press Ctrl+C to stop"
echo

# Run the monitor
build/xdp_monitor "$MAP_FD"
