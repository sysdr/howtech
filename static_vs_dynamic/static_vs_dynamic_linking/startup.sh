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

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Starting Static Binary Demo${NC}"
echo -e "${CYAN}========================================${NC}\n"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Detect which static binary exists
STATIC_BINARY=""
if [ -f "${BUILD_DIR}/httpserver-static-musl" ]; then
    STATIC_BINARY="${BUILD_DIR}/httpserver-static-musl"
elif [ -f "${BUILD_DIR}/httpserver-static-glibc" ]; then
    STATIC_BINARY="${BUILD_DIR}/httpserver-static-glibc"
else
    echo -e "${RED}Error: No static binary found in $BUILD_DIR${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check if server is already running
if [ -f "${SCRIPT_DIR}/.server.pid" ]; then
    OLD_PID=$(cat "${SCRIPT_DIR}/.server.pid" 2>/dev/null || echo "")
    if [ -n "$OLD_PID" ] && ps -p "$OLD_PID" > /dev/null 2>&1; then
        echo -e "${YELLOW}Server is already running (PID: $OLD_PID)${NC}"
        echo -e "${YELLOW}Use cleanup.sh to stop it first${NC}"
        exit 1
    else
        rm -f "${SCRIPT_DIR}/.server.pid"
    fi
fi

# Check for any running httpserver processes
if pgrep -f "httpserver-static" > /dev/null 2>&1; then
    echo -e "${YELLOW}Warning: Found existing httpserver processes${NC}"
    pgrep -f "httpserver-static"
    echo -e "${YELLOW}Use cleanup.sh to stop them first${NC}"
    exit 1
fi

# Verify binary is executable
if [ ! -x "$STATIC_BINARY" ]; then
    echo -e "${RED}Error: Binary is not executable: $STATIC_BINARY${NC}"
    exit 1
fi

echo -e "${GREEN}Starting static binary HTTP server...${NC}"
echo -e "${BLUE}Binary: $STATIC_BINARY${NC}"

# Change to script directory to run relative to project root
cd "$SCRIPT_DIR"

# Start the server in background
"$STATIC_BINARY" > "${SCRIPT_DIR}/server.log" 2>&1 &
SERVER_PID=$!
sleep 2

# Verify server started successfully
if ! ps -p $SERVER_PID > /dev/null 2>&1; then
    echo -e "${RED}Error: Server failed to start${NC}"
    cat "${SCRIPT_DIR}/server.log" 2>/dev/null || true
    exit 1
fi

# Save PID for cleanup
echo $SERVER_PID > "${SCRIPT_DIR}/.server.pid"

echo -e "${GREEN}Server running (PID: $SERVER_PID)${NC}"
echo -e "${GREEN}Listening on http://localhost:8080${NC}\n"

# Test if server is responding
if command -v curl > /dev/null 2>&1; then
    echo -e "${CYAN}Testing server response...${NC}"
    if curl -s -f http://localhost:8080 > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Server is responding correctly${NC}\n"
    else
        echo -e "${YELLOW}Warning: Server may not be responding yet${NC}\n"
    fi
fi

# Show memory information if memanalyzer is available
if [ -f "${BUILD_DIR}/memanalyzer" ] && [ -x "${BUILD_DIR}/memanalyzer" ]; then
    echo -e "${CYAN}=== Process Memory Analysis ===${NC}"
    "${BUILD_DIR}/memanalyzer" $SERVER_PID 2>/dev/null || echo "Memory analysis tool unavailable"
    echo ""
fi

echo -e "${CYAN}Server Status:${NC}"
echo -e "  ${BLUE}•${NC} PID: $SERVER_PID"
echo -e "  ${BLUE}•${NC} Binary: $STATIC_BINARY"
echo -e "  ${BLUE}•${NC} Port: 8080"
echo -e "  ${BLUE}•${NC} Log file: ${SCRIPT_DIR}/server.log"
echo -e "  ${BLUE}•${NC} PID file: ${SCRIPT_DIR}/.server.pid"

echo -e "\n${GREEN}To monitor metrics, run: ${BLUE}./monitor.sh $SERVER_PID${NC}"
echo -e "${GREEN}To stop server, run: ${BLUE}./cleanup.sh${NC}\n"

