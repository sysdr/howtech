#!/bin/bash
set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}Starting LD_PRELOAD Demo...${NC}\n"

# Check if build files exist
if [ ! -f "./build/malloc_hook.so" ]; then
    echo -e "${YELLOW}Error: build/malloc_hook.so not found. Run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "./build/test_program" ]; then
    echo -e "${YELLOW}Error: build/test_program not found. Run setup.sh first.${NC}"
    exit 1
fi

# Run the demo with LD_PRELOAD
echo -e "${GREEN}Running test program with LD_PRELOAD interposition...${NC}\n"
LD_PRELOAD="$SCRIPT_DIR/build/malloc_hook.so" "$SCRIPT_DIR/build/test_program"

echo -e "\n${CYAN}Demo completed successfully!${NC}\n"

