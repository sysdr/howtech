#!/bin/bash

# Demo script for custom syscall demonstration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}Custom Syscall Demo${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# Check if setup has been run
if [ ! -f "src/custom_syscall.c" ] || [ ! -f "build/test_syscall" ]; then
    echo -e "${YELLOW}Setup not complete. Running setup.sh first...${NC}"
    ./setup.sh
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}This script requires root privileges${NC}"
    echo "Please run with: sudo $0"
    exit 1
fi

# Check if module is loaded
if ! lsmod | grep -q custom_syscall; then
    echo -e "${BLUE}Loading kernel module...${NC}"
    if [ ! -f "src/custom_syscall.ko" ]; then
        echo -e "${RED}Kernel module not found. Please run setup.sh first.${NC}"
        exit 1
    fi
    
    # Remove if already loaded (cleanup)
    rmmod custom_syscall 2>/dev/null || true
    
    # Load module
    insmod src/custom_syscall.ko
    
    # Get major number from dmesg
    sleep 1
    MAJOR=$(dmesg | grep "custom_syscall: Registered with major number" | tail -1 | awk '{print $NF}')
    
    if [ -n "$MAJOR" ]; then
        # Create device node
        rm -f /dev/custom_syscall
        mknod /dev/custom_syscall c $MAJOR 0
        chmod 666 /dev/custom_syscall
        echo -e "${GREEN}✓ Module loaded, device created${NC}"
    else
        echo -e "${RED}Failed to get major number${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ Kernel module already loaded${NC}"
fi

echo ""
echo -e "${BLUE}Running test program...${NC}"
echo ""
"$SCRIPT_DIR/build/test_syscall"

echo ""
echo -e "${BLUE}Kernel module output (dmesg):${NC}"
dmesg | grep custom_syscall | tail -20

echo ""
echo -e "${GREEN}Demo complete!${NC}"
echo ""
echo "To monitor in real-time, run:"
echo "  $SCRIPT_DIR/build/monitor"
echo ""
