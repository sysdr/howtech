#!/bin/bash
# Startup script for Core Dump Monitoring Dashboard

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if core_monitor exists
MONITOR_BIN="$SCRIPT_DIR/build/core_monitor"
if [ ! -f "$MONITOR_BIN" ]; then
    echo -e "${RED}Error: core_monitor not found at $MONITOR_BIN${NC}"
    echo "Please run setup.sh first to build the project."
    exit 1
fi

# Check if it's executable
if [ ! -x "$MONITOR_BIN" ]; then
    echo -e "${YELLOW}Warning: core_monitor is not executable. Making it executable...${NC}"
    chmod +x "$MONITOR_BIN"
fi

# Ensure cores directory exists
mkdir -p "$SCRIPT_DIR/cores"

echo -e "${GREEN}Starting Core Dump Monitoring Dashboard...${NC}"
echo "Dashboard location: $MONITOR_BIN"
echo ""

# Run the monitor
exec "$MONITOR_BIN"

