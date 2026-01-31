#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Cleaning up eBPF LSM demo...${NC}"

# Kill any running loader processes
pkill -f "build/loader" 2>/dev/null && echo -e "${GREEN}✓ Stopped loader processes${NC}" || true

# Remove test file
rm -f /tmp/protected_file.txt && echo -e "${GREEN}✓ Removed test file${NC}" || true

# Clean build artifacts
if [ -d "lsm-bpf-demo" ]; then
    cd lsm-bpf-demo
    make clean 2>/dev/null || true
    cd ..
    rm -rf lsm-bpf-demo && echo -e "${GREEN}✓ Removed lsm-bpf-demo directory${NC}"
fi

echo -e "${GREEN}Cleanup complete!${NC}"
