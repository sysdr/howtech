#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ -f /tmp/ebpf_loader.pid ]; then
    PID=$(cat /tmp/ebpf_loader.pid)
    if kill $PID 2>/dev/null; then
        echo -e "${GREEN}✓ Stopped loader (PID: $PID)${NC}"
        rm -f /tmp/ebpf_loader.pid
    else
        echo -e "${YELLOW}Loader process not found${NC}"
        rm -f /tmp/ebpf_loader.pid
    fi
else
    echo -e "${YELLOW}No loader process found${NC}"
    pkill -f "build/loader" 2>/dev/null && echo -e "${GREEN}✓ Stopped any running loaders${NC}" || true
fi
