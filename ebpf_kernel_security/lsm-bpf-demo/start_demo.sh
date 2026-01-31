#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}This script requires root privileges${NC}"
    exit 1
fi

PROTECTED_FILE="${1:-/tmp/protected_file.txt}"

if [ ! -f "build/loader" ]; then
    echo -e "${RED}Error: build/loader not found. Run setup.sh first.${NC}"
    exit 1
fi

echo -e "${BLUE}Starting eBPF LSM demo...${NC}"
echo -e "${YELLOW}Protected file: ${PROTECTED_FILE}${NC}"

# Start loader in background
build/loader "$PROTECTED_FILE" &
LOADER_PID=$!
echo $LOADER_PID > /tmp/ebpf_loader.pid
echo -e "${GREEN}✓ Loader started (PID: $LOADER_PID)${NC}"

sleep 2

echo -e "\n${GREEN}Demo is running. Use './stop_demo.sh' to stop.${NC}"
