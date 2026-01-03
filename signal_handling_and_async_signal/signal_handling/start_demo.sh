#!/bin/bash

# Startup script for signal handling demos

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== Starting Signal Handling Demo ===${NC}"

# Check if binaries exist
if [ ! -f "build/safe_signalfd" ]; then
    echo -e "${RED}Error: build/safe_signalfd not found. Run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "build/dangerous" ]; then
    echo -e "${RED}Error: build/dangerous not found. Run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "build/monitor" ]; then
    echo -e "${RED}Error: build/monitor not found. Run setup.sh first.${NC}"
    exit 1
fi

echo -e "${GREEN}All binaries found.${NC}"
echo ""

# Function to send signals
send_signals() {
    local pid=$1
    local count=$2
    sleep 1
    for i in $(seq 1 $count); do
        kill -USR1 $pid 2>/dev/null || break
        sleep 0.2
    done
}

# Start safe signalfd demo in background
echo -e "${YELLOW}Starting safe signalfd demo...${NC}"
"$SCRIPT_DIR/build/safe_signalfd" &
SAFE_PID=$!
echo "Safe demo PID: $SAFE_PID"

# Send some signals
sleep 2
echo "Sending signals to safe demo..."
send_signals $SAFE_PID 3 &
SIGNAL_PID=$!

# Wait a bit then cleanup
sleep 5
kill -TERM $SAFE_PID 2>/dev/null || true
wait $SAFE_PID 2>/dev/null || true
wait $SIGNAL_PID 2>/dev/null || true

echo -e "${GREEN}Demo completed successfully${NC}"

