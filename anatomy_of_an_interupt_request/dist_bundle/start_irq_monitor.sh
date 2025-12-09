#!/bin/bash

# Startup script for IRQ Monitor with full path checking
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IRQ_MONITOR="${SCRIPT_DIR}/irq_monitor"

echo -e "${CYAN}Starting IRQ Monitor...${NC}"

# Check if irq_monitor exists
if [ ! -f "$IRQ_MONITOR" ]; then
    echo -e "${RED}Error: irq_monitor not found at ${IRQ_MONITOR}${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check if it's executable
if [ ! -x "$IRQ_MONITOR" ]; then
    echo -e "${RED}Error: irq_monitor is not executable${NC}"
    chmod +x "$IRQ_MONITOR"
    echo -e "${GREEN}Made irq_monitor executable${NC}"
fi

# Check if already running
if pgrep -f "irq_monitor" > /dev/null; then
    echo -e "${YELLOW}Warning: irq_monitor appears to be already running${NC}"
    echo -e "${YELLOW}PIDs: $(pgrep -f 'irq_monitor')${NC}"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check permissions
if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}Warning: Not running as root. Some /proc files may not be accessible.${NC}"
    echo -e "${YELLOW}For full functionality, run with: sudo $0${NC}"
fi

echo -e "${GREEN}Starting irq_monitor from: ${IRQ_MONITOR}${NC}"
exec "$IRQ_MONITOR"

