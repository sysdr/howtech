#!/bin/bash

# Stop script for kernel module debugging demo

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}Stopping Kernel Module Debugging Demo...${NC}"

# Unload kernel module
if lsmod | grep -q "^debug_demo"; then
    echo -e "${BLUE}Unloading kernel module...${NC}"
    sudo rmmod debug_demo 2>/dev/null && echo -e "${GREEN}✓ Module unloaded${NC}" || echo -e "${YELLOW}Module already unloaded${NC}"
fi

# Stop dashboard
if pgrep -f "dashboard.py" > /dev/null; then
    echo -e "${BLUE}Stopping dashboard...${NC}"
    pkill -f dashboard.py
    sleep 1
    echo -e "${GREEN}✓ Dashboard stopped${NC}"
fi

# Stop monitor (if running in background)
if pgrep -f "klog_monitor" > /dev/null; then
    echo -e "${BLUE}Stopping kernel log monitor...${NC}"
    pkill -f klog_monitor
    sleep 1
    echo -e "${GREEN}✓ Monitor stopped${NC}"
fi

echo -e "${GREEN}✓ All services stopped${NC}"

