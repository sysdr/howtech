#!/bin/bash

# Cleanup script for custom syscall demonstration

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
echo -e "${BLUE}Cleanup Custom Syscall Demo${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}Note: Some cleanup operations require root privileges${NC}"
    echo "Running without root (limited cleanup)..."
    SUDO=""
else
    SUDO=""
fi

# Unload kernel module if loaded
if lsmod | grep -q custom_syscall; then
    echo -e "${BLUE}Unloading kernel module...${NC}"
    rmmod custom_syscall 2>/dev/null && echo -e "${GREEN}✓ Module unloaded${NC}" || echo -e "${YELLOW}⚠ Failed to unload module${NC}"
else
    echo -e "${GREEN}✓ Kernel module not loaded${NC}"
fi

# Remove device node
if [ -e /dev/custom_syscall ]; then
    echo -e "${BLUE}Removing device node...${NC}"
    if [ "$EUID" -eq 0 ]; then
        rm -f /dev/custom_syscall && echo -e "${GREEN}✓ Device node removed${NC}"
    else
        echo -e "${YELLOW}⚠ Skipping device node removal (requires root)${NC}"
    fi
else
    echo -e "${GREEN}✓ Device node does not exist${NC}"
fi

# Optional: Clean build artifacts
read -p "Remove build artifacts? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${BLUE}Cleaning build artifacts...${NC}"
    rm -rf build/*
    if [ -d "src" ]; then
        cd src
        make clean 2>/dev/null || true
        cd ..
    fi
    echo -e "${GREEN}✓ Build artifacts cleaned${NC}"
fi

echo ""
echo -e "${GREEN}Cleanup complete!${NC}"
echo ""
