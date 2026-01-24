#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Cleaning up XDP demo...${NC}"

# Unload XDP program from loopback
if ip link show dev lo 2>/dev/null | grep -q xdp; then
    echo -e "${GREEN}Unloading XDP program from loopback interface...${NC}"
    ip link set dev lo xdp off 2>/dev/null || \
    ip link set dev lo xdpgeneric off 2>/dev/null || true
    echo -e "${GREEN}XDP program unloaded${NC}"
else
    echo "No XDP program loaded on loopback"
fi

# Kill any running processes
pkill -f "packet_gen" 2>/dev/null || true
pkill -f "xdp_monitor" 2>/dev/null || true
pkill -f "xdp_dashboard" 2>/dev/null || true

echo -e "${GREEN}Cleanup complete!${NC}"
