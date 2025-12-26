#!/bin/bash

# Demo script for kernel module debugging techniques

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
echo -e "${CYAN}║   Kernel Module Debugging Demo                                ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"

# Check if module is built
if [ ! -f "src/debug_demo.ko" ]; then
    echo -e "${YELLOW}Module not built. Building...${NC}"
    cd src
    if make 2>/dev/null; then
        echo -e "${GREEN}✓ Module built${NC}"
    else
        echo -e "${RED}✗ Build failed. This is expected in WSL/containers.${NC}"
        echo -e "${YELLOW}Continuing with demo using available tools...${NC}"
    fi
    cd ..
fi

echo -e "\n${BLUE}=== Demo 1: Loading Module with Default Parameters ===${NC}"
if [ -f "src/debug_demo.ko" ]; then
    # Unload if already loaded
    if lsmod | grep -q "^debug_demo"; then
        sudo rmmod debug_demo 2>/dev/null || true
    fi
    
    echo -e "${CYAN}Loading module...${NC}"
    if sudo insmod src/debug_demo.ko; then
        echo -e "${GREEN}✓ Module loaded${NC}"
        sleep 2
        
        echo -e "\n${CYAN}Recent kernel logs:${NC}"
        dmesg | grep "debug_demo:" | tail -15
        
        echo -e "\n${CYAN}Module information:${NC}"
        modinfo src/debug_demo.ko | head -10
        
        sudo rmmod debug_demo
        echo -e "${GREEN}✓ Module unloaded${NC}"
    else
        echo -e "${YELLOW}Module load failed (expected in WSL/containers)${NC}"
    fi
else
    echo -e "${YELLOW}Skipping - module not built${NC}"
fi

echo -e "\n${BLUE}=== Demo 2: Loading Module with Custom Parameters ===${NC}"
if [ -f "src/debug_demo.ko" ]; then
    echo -e "${CYAN}Loading with simulate_bug=1, loop_count=3...${NC}"
    if sudo insmod src/debug_demo.ko simulate_bug=1 loop_count=3 2>/dev/null; then
        sleep 2
        echo -e "${CYAN}Kernel logs with bug simulation:${NC}"
        dmesg | grep "debug_demo:" | tail -10
        
        if [ -d "/sys/module/debug_demo/parameters" ]; then
            echo -e "\n${CYAN}Current module parameters:${NC}"
            for param in /sys/module/debug_demo/parameters/*; do
                echo "  $(basename $param): $(cat $param)"
            done
        fi
        
        sudo rmmod debug_demo 2>/dev/null || true
    fi
else
    echo -e "${YELLOW}Skipping - module not built${NC}"
fi

echo -e "\n${BLUE}=== Demo 3: Kernel Log Monitoring ===${NC}"
if [ -f "src/klog_monitor" ] && [ -x "src/klog_monitor" ]; then
    echo -e "${CYAN}Kernel log monitor is available${NC}"
    echo -e "${YELLOW}To run: sudo ./src/klog_monitor${NC}"
    echo -e "${YELLOW}(This requires an interactive terminal)${NC}"
else
    echo -e "${YELLOW}Monitor not built${NC}"
fi

echo -e "\n${BLUE}=== Demo 4: Viewing Kernel Logs ===${NC}"
echo -e "${CYAN}All debug_demo messages:${NC}"
dmesg | grep "debug_demo:" | tail -20 || echo "No messages found"

echo -e "\n${CYAN}Log level distribution:${NC}"
dmesg | grep "debug_demo:" | grep -oE "(EMERG|ALERT|CRIT|ERR|WARNING|NOTICE|INFO|DEBUG)" | sort | uniq -c || echo "No logs to analyze"

echo -e "\n${GREEN}✓ Demo complete!${NC}"
echo -e "\n${CYAN}Next steps:${NC}"
echo -e "  • Run ${YELLOW}./start.sh${NC} to start all services"
echo -e "  • Open ${YELLOW}http://localhost:8080${NC} for dashboard"
echo -e "  • Run ${YELLOW}./test.sh${NC} to verify everything"

