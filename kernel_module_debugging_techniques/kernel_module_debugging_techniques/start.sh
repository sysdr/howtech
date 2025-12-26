#!/bin/bash

# Startup script for kernel module debugging demo
# This script starts all necessary services and components

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   Starting Kernel Module Debugging Demo                      ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"

# Check if setup has been run
if [ ! -f "src/debug_demo.c" ]; then
    echo -e "${YELLOW}Running setup first...${NC}"
    bash setup.sh
fi

# Check for duplicate services
echo -e "\n${BLUE}Checking for existing services...${NC}"

# Check if module is already loaded
if lsmod | grep -q "^debug_demo"; then
    echo -e "${YELLOW}Module debug_demo is already loaded${NC}"
    read -p "Unload existing module? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo rmmod debug_demo 2>/dev/null || true
        echo -e "${GREEN}✓ Module unloaded${NC}"
    fi
fi

# Check if dashboard is running
DASHBOARD_PIDS=$(pgrep -f "dashboard.py" || true)
if [ -n "$DASHBOARD_PIDS" ]; then
    echo -e "${YELLOW}Dashboard is already running (PIDs: $DASHBOARD_PIDS)${NC}"
    echo -e "${YELLOW}Stopping existing dashboard instances...${NC}"
    pkill -f dashboard.py
    sleep 2
    # Verify they're stopped
    if pgrep -f "dashboard.py" > /dev/null; then
        echo -e "${RED}✗ Failed to stop existing dashboard${NC}"
    else
        echo -e "${GREEN}✓ Existing dashboards stopped${NC}"
    fi
fi

# Check if monitor is running
if pgrep -f "klog_monitor" > /dev/null; then
    echo -e "${YELLOW}Kernel log monitor is already running${NC}"
fi

# Load kernel module if built
if [ -f "src/debug_demo.ko" ]; then
    echo -e "\n${BLUE}Loading kernel module...${NC}"
    if sudo insmod src/debug_demo.ko; then
        echo -e "${GREEN}✓ Module loaded successfully${NC}"
        sleep 1
        echo -e "${CYAN}Recent kernel logs:${NC}"
        dmesg | grep "debug_demo:" | tail -10
    else
        echo -e "${RED}✗ Failed to load module${NC}"
        echo -e "${YELLOW}This is expected in WSL or container environments${NC}"
    fi
else
    echo -e "${YELLOW}Module not built (src/debug_demo.ko not found)${NC}"
    echo -e "${YELLOW}This is expected in WSL or container environments${NC}"
fi

# Start dashboard if available
if [ -f "src/dashboard.py" ]; then
    echo -e "\n${BLUE}Starting metrics dashboard...${NC}"
    # Use the dedicated dashboard starter script for better reliability
    if [ -f "start_dashboard.sh" ]; then
        if bash start_dashboard.sh 2>&1 | grep -q "✓ Dashboard started"; then
            echo -e "${GREEN}✓ Dashboard started successfully${NC}"
            echo -e "${CYAN}Dashboard available at: http://localhost:8080${NC}"
        else
            echo -e "${YELLOW}⚠ Dashboard startup had issues, check logs/dashboard.log${NC}"
        fi
    else
        # Fallback to direct start
        pkill -9 -f "dashboard.py" 2>/dev/null
        sleep 2
        cd src
        if command -v python3 &> /dev/null; then
            python3 dashboard.py > ../logs/dashboard.log 2>&1 &
            sleep 5
            if curl -s http://127.0.0.1:8080/api/metrics > /dev/null 2>&1; then
                echo -e "${GREEN}✓ Dashboard started${NC}"
                echo -e "${CYAN}Dashboard available at: http://localhost:8080${NC}"
            else
                echo -e "${YELLOW}⚠ Dashboard may not be responding, check logs/dashboard.log${NC}"
            fi
        fi
        cd ..
    fi
fi

echo -e "\n${GREEN}✓ Startup complete!${NC}"
echo -e "\n${CYAN}Available commands:${NC}"
echo -e "  • View logs: ${YELLOW}dmesg | grep debug_demo${NC}"
echo -e "  • Monitor: ${YELLOW}sudo ./src/klog_monitor${NC}"
echo -e "  • Dashboard: ${YELLOW}http://localhost:8080${NC}"
echo -e "  • Stop: ${YELLOW}./stop.sh${NC}"

