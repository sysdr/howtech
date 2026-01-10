#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Cleaning up...${NC}"

# Stop server if running
if [ -f .server.pid ]; then
    PID=$(cat .server.pid)
    if ps -p $PID > /dev/null 2>&1; then
        echo -e "${GREEN}Stopping server (PID: $PID)...${NC}"
        kill $PID 2>/dev/null || true
        sleep 1
        if ps -p $PID > /dev/null 2>&1; then
            kill -9 $PID 2>/dev/null || true
        fi
    fi
    rm -f .server.pid
fi

# Stop any other instances
pkill -f "httpserver" 2>/dev/null || true
pkill -f "monitor" 2>/dev/null || true
sleep 1
# Force kill any remaining
pkill -9 -f "httpserver" 2>/dev/null || true
pkill -9 -f "monitor" 2>/dev/null || true

echo -e "${GREEN}Cleanup complete!${NC}"
