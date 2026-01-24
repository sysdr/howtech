#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f netcorr_user ]; then
    echo -e "${RED}Error: netcorr_user not found. Run setup.sh first.${NC}"
    exit 1
fi

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Starting eBPF Network Correlation Monitor${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo

# Check if we have necessary permissions
if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}Note: This demo needs root privileges to load eBPF programs${NC}"
    echo -e "${YELLOW}Attempting to run with sudo...${NC}"
    echo
    
    # Start monitor in background
    sudo ./netcorr_user &
    MONITOR_PID=$!
    
    sleep 2
    
    echo -e "${BLUE}Running test client in 3 seconds...${NC}"
    sleep 3
    
    ./test_client
    
    sleep 2
    
    echo
    echo -e "${YELLOW}Stopping monitor...${NC}"
    sleep 3
    sudo kill -INT $MONITOR_PID 2>/dev/null
    
else
    # We're already root
    ./netcorr_user &
    MONITOR_PID=$!
    
    sleep 2
    
    echo -e "${BLUE}Running test client in 3 seconds...${NC}"
    sleep 3
    
    ./test_client
    
    sleep 2
    
    echo
    echo -e "${YELLOW}Stopping monitor...${NC}"
    sleep 3
    kill -INT $MONITOR_PID 2>/dev/null
fi

wait $MONITOR_PID 2>/dev/null

echo
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}  Demo Complete!${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
