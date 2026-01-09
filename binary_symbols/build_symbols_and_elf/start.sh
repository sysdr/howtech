#!/bin/bash
set -euo pipefail

# Colors for output
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}Starting Binary Symbol Analysis Demo...${NC}"
echo ""

# Check if binaries exist
if [ ! -f "build/demo" ]; then
    echo -e "${YELLOW}Binaries not found. Running setup...${NC}"
    bash setup.sh
fi

# Check for duplicate processes
if pgrep -f "build/demo" > /dev/null; then
    echo -e "${YELLOW}Warning: Demo process already running${NC}"
    pgrep -f "build/demo"
fi

if pgrep -f "build/monitor" > /dev/null; then
    echo -e "${YELLOW}Warning: Monitor process already running${NC}"
    pgrep -f "build/monitor"
fi

echo -e "${GREEN}Running demo program...${NC}"
./build/demo

echo ""
echo -e "${CYAN}Demo completed successfully!${NC}"

