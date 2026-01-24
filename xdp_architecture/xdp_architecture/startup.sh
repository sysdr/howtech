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
echo -e "${BLUE}XDP Demo Startup${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}This script requires root privileges${NC}"
    echo "Please run with: sudo ./startup.sh"
    exit 1
fi

# Check if binaries exist
if [ ! -f "build/xdp_drop.o" ]; then
    echo -e "${RED}Error: XDP program not built${NC}"
    echo "Please run ./setup.sh first"
    exit 1
fi

# Check if XDP is already loaded
if ip link show dev lo 2>/dev/null | grep -q xdp; then
    echo -e "${YELLOW}Warning: XDP program already loaded${NC}"
    echo "Unloading existing program..."
    ip link set dev lo xdp off 2>/dev/null || \
    ip link set dev lo xdpgeneric off 2>/dev/null || true
    sleep 1
fi

# Mount BPF filesystem if needed
mkdir -p /sys/fs/bpf
mount -t bpf bpf /sys/fs/bpf 2>/dev/null || true

# Load XDP program
echo -e "${GREEN}Loading XDP program on loopback interface...${NC}"
ip link set dev lo xdp obj build/xdp_drop.o sec xdp 2>/dev/null || \
ip link set dev lo xdpgeneric obj build/xdp_drop.o sec xdp

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to load XDP program${NC}"
    exit 1
fi

echo -e "${GREEN}XDP program loaded successfully!${NC}"

# Get map FD
MAP_FD=$(bpftool map list 2>/dev/null | grep xdp_stats | awk '{print $1}' | cut -d: -f1 || echo "")

if [ -n "$MAP_FD" ]; then
    echo "$MAP_FD" > /tmp/xdp_map_fd
    echo -e "${GREEN}Statistics map FD: $MAP_FD${NC}"
else
    echo -e "${YELLOW}Warning: Could not find statistics map FD${NC}"
fi

# Show XDP mode
echo
echo -e "${BLUE}XDP Mode:${NC}"
ip link show dev lo | grep -i xdp
echo

echo -e "${GREEN}Startup complete!${NC}"
echo "Use ./run_demo.sh to run the demo"
echo "Use ./run_dashboard.sh to start the dashboard"
echo "Use ./cleanup.sh to unload XDP program"
