#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get the absolute path of the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Check if PID is provided
if [ $# -eq 0 ]; then
    if [ -f "${SCRIPT_DIR}/.server.pid" ]; then
        SERVER_PID=$(cat "${SCRIPT_DIR}/.server.pid" 2>/dev/null || echo "")
    else
        echo -e "${RED}Error: No PID provided and .server.pid not found${NC}"
        echo -e "${YELLOW}Usage: $0 <pid> [duration_seconds]${NC}"
        echo -e "${YELLOW}Or start the server with startup.sh first${NC}"
        exit 1
    fi
else
    SERVER_PID="$1"
fi

DURATION="${2:-0}"

if [ -z "$SERVER_PID" ]; then
    echo -e "${RED}Error: Server PID not found${NC}"
    exit 1
fi

# Check if process exists
if ! ps -p "$SERVER_PID" > /dev/null 2>&1; then
    echo -e "${RED}Error: Process $SERVER_PID not found${NC}"
    exit 1
fi

# Check if monitor binary exists
if [ ! -f "${BUILD_DIR}/monitor" ] || [ ! -x "${BUILD_DIR}/monitor" ]; then
    echo -e "${RED}Error: Monitor binary not found at ${BUILD_DIR}/monitor${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

echo -e "${CYAN}Starting performance monitor for PID: $SERVER_PID${NC}"
if [ "$DURATION" -gt 0 ]; then
    echo -e "${CYAN}Duration: $DURATION seconds${NC}"
fi
echo ""

# Run monitor
cd "$SCRIPT_DIR"
"${BUILD_DIR}/monitor" "$SERVER_PID" "$DURATION"

