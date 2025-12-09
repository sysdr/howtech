#!/bin/bash

# Startup script for IRQ Stress Test with full path checking
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IRQ_STRESS="${SCRIPT_DIR}/irq_stress"

echo -e "${CYAN}Starting IRQ Stress Test...${NC}"

# Check if irq_stress exists
if [ ! -f "$IRQ_STRESS" ]; then
    echo -e "${RED}Error: irq_stress not found at ${IRQ_STRESS}${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check if it's executable
if [ ! -x "$IRQ_STRESS" ]; then
    echo -e "${RED}Error: irq_stress is not executable${NC}"
    chmod +x "$IRQ_STRESS"
    echo -e "${GREEN}Made irq_stress executable${NC}"
fi

# Check if already running
if pgrep -f "irq_stress" > /dev/null; then
    echo -e "${YELLOW}Warning: irq_stress appears to be already running${NC}"
    echo -e "${YELLOW}PIDs: $(pgrep -f 'irq_stress')${NC}"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo -e "${GREEN}Starting irq_stress from: ${IRQ_STRESS}${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop${NC}"
exec "$IRQ_STRESS"

