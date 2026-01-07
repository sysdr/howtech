#!/bin/bash
# Startup script for kernel module demo
# Loads the module and starts the monitor

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Kernel Module Demo Startup ===${NC}\n"

# Check if module is already loaded
if lsmod | grep -q "^hello_module"; then
    echo -e "${YELLOW}Warning: hello_module is already loaded${NC}"
    echo "Unloading existing module..."
    sudo rmmod hello_module 2>/dev/null || true
    sleep 1
fi

# Check if module file exists
if [ ! -f "src/hello_module.ko" ]; then
    echo -e "${RED}Error: Module file not found: src/hello_module.ko${NC}"
    echo "Please build the module first: make"
    exit 1
fi

# Load the module
echo -e "${BLUE}Loading kernel module...${NC}"
if sudo insmod src/hello_module.ko; then
    echo -e "${GREEN}Module loaded successfully${NC}"
else
    echo -e "${RED}Failed to load module${NC}"
    echo "Check dmesg for errors: dmesg | tail -20"
    exit 1
fi

# Show module info
echo -e "\n${BLUE}Module Information:${NC}"
lsmod | grep hello_module || true
echo ""
dmesg | grep hello_module | tail -5

# Start monitor in background if it exists
if [ -f "module_monitor" ] && [ -x "module_monitor" ]; then
    echo -e "\n${BLUE}Starting module monitor...${NC}"
    echo "Monitor will run in the background. Check with: ps aux | grep module_monitor"
    ./module_monitor &
    MONITOR_PID=$!
    echo "Monitor PID: $MONITOR_PID"
    echo "To stop monitor: kill $MONITOR_PID"
fi

echo -e "\n${GREEN}Startup complete!${NC}"
echo "To unload module: sudo rmmod hello_module"

