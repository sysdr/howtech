#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  eBPF Verifier Demo - Starting...                         ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if log_parser exists
if [ ! -f "build/log_parser" ]; then
    echo -e "${YELLOW}Building log_parser...${NC}"
    gcc -O2 -Wall -Wextra -o build/log_parser src/monitor/log_parser.c || {
        echo -e "${RED}Failed to build log_parser${NC}"
        exit 1
    }
fi

echo -e "${GREEN}[1/3] Showing Verification Pipeline...${NC}"
./build/log_parser stages

echo ""
echo -e "${GREEN}[2/3] Showing Register State Types...${NC}"
./build/log_parser registers

echo ""
echo -e "${GREEN}[3/3] Demo complete!${NC}"
echo ""
echo -e "${BLUE}Available source files:${NC}"
echo "  - Passing programs: $(ls -1 src/passing/*.c 2>/dev/null | wc -l) files"
echo "  - Failing programs: $(ls -1 src/failing/*.c 2>/dev/null | wc -l) files"
echo "  - Monitor tools: $(ls -1 src/monitor/*.c 2>/dev/null | wc -l) files"
echo ""
echo -e "${GREEN}Demo is working correctly!${NC}"

