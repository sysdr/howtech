#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}Cleaning up eBPF programs and processes...${NC}"

# Kill any running netcorr_user processes
PIDS=$(pgrep -f netcorr_user || true)
if [ -n "$PIDS" ]; then
    echo -e "${YELLOW}Stopping netcorr_user processes...${NC}"
    echo "$PIDS" | xargs -r sudo kill -INT 2>/dev/null || true
    sleep 1
    echo "$PIDS" | xargs -r sudo kill -9 2>/dev/null || true
fi

# Kill any running monitor processes
PIDS=$(pgrep -f "monitor " || true)
if [ -n "$PIDS" ]; then
    echo -e "${YELLOW}Stopping monitor processes...${NC}"
    echo "$PIDS" | xargs -r kill -INT 2>/dev/null || true
    sleep 1
    echo "$PIDS" | xargs -r kill -9 2>/dev/null || true
fi

# Unload BPF programs if they exist
if command -v bpftool &> /dev/null; then
    PROGS=$(sudo bpftool prog list 2>/dev/null | grep -E "trace_tcp_connect|trace_tcp_sendmsg|trace_tcp_recvmsg|trace_tcp_close" || true)
    if [ -n "$PROGS" ]; then
        echo -e "${YELLOW}Unloading BPF programs...${NC}"
        sudo bpftool prog list | grep -E "id [0-9]+" | awk '{print $2}' | while read id; do
            sudo bpftool prog detach id $id 2>/dev/null || true
        done
    fi
fi

# Clean up build artifacts (optional)
if [ "$1" = "--all" ]; then
    echo -e "${YELLOW}Cleaning build artifacts...${NC}"
    make clean 2>/dev/null || true
    rm -f *.o netcorr_user test_client monitor netcorr.skel.h vmlinux.h 2>/dev/null || true
fi

echo -e "${GREEN}Cleanup complete!${NC}"
