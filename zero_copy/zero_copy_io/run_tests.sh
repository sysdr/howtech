#!/bin/bash
# Test script for zero-copy demonstration

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DATA_DIR="${SCRIPT_DIR}/data"
CLIENT="${BUILD_DIR}/client"
TESTFILE="${DATA_DIR}/testfile.bin"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Check if binaries exist
if [ ! -f "${CLIENT}" ]; then
    echo -e "${RED}Error: Client binary not found. Please run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/traditional_server" ] || \
   [ ! -f "${BUILD_DIR}/sendfile_server" ] || \
   [ ! -f "${BUILD_DIR}/splice_server" ]; then
    echo -e "${RED}Error: Server binaries not found. Please run setup.sh first.${NC}"
    exit 1
fi

# Kill any existing servers
pkill -f "traditional_server.*testfile" 2>/dev/null || true
pkill -f "sendfile_server.*testfile" 2>/dev/null || true
pkill -f "splice_server.*testfile" 2>/dev/null || true
sleep 1

echo -e "${YELLOW}Running zero-copy demonstration tests...${NC}\n"

# Test 1: Traditional server
echo -e "${YELLOW}Test 1: Traditional read/write server (port 8080)${NC}"
"${BUILD_DIR}/traditional_server" "${TESTFILE}" > /tmp/test_traditional_server.log 2>&1 &
TRAD_PID=$!
sleep 2

if "${CLIENT}" 8080 > /tmp/test_traditional.log 2>&1; then
    wait $TRAD_PID 2>/dev/null || true
    RECEIVED=$(grep -oP 'received: \K\d+' /tmp/test_traditional.log 2>/dev/null || echo "0")
    if [ "$RECEIVED" -gt 0 ]; then
        echo -e "${GREEN}✓ Traditional server test passed (received ${RECEIVED} bytes)${NC}"
    else
        echo -e "${RED}✗ Traditional server test failed (no data received)${NC}"
        cat /tmp/test_traditional.log
        exit 1
    fi
else
    wait $TRAD_PID 2>/dev/null || true
    echo -e "${RED}✗ Traditional server test failed (client error)${NC}"
    cat /tmp/test_traditional.log
    exit 1
fi

sleep 1

# Test 2: Sendfile server
echo -e "${YELLOW}Test 2: sendfile() zero-copy server (port 8081)${NC}"
"${BUILD_DIR}/sendfile_server" "${TESTFILE}" > /tmp/test_sendfile_server.log 2>&1 &
SENDFILE_PID=$!
sleep 2

if "${CLIENT}" 8081 > /tmp/test_sendfile.log 2>&1; then
    wait $SENDFILE_PID 2>/dev/null || true
    RECEIVED=$(grep -oP 'received: \K\d+' /tmp/test_sendfile.log 2>/dev/null || echo "0")
    if [ "$RECEIVED" -gt 0 ]; then
        echo -e "${GREEN}✓ Sendfile server test passed (received ${RECEIVED} bytes)${NC}"
    else
        echo -e "${RED}✗ Sendfile server test failed (no data received)${NC}"
        cat /tmp/test_sendfile.log
        exit 1
    fi
else
    wait $SENDFILE_PID 2>/dev/null || true
    echo -e "${RED}✗ Sendfile server test failed (client error)${NC}"
    cat /tmp/test_sendfile.log
    exit 1
fi

sleep 1

# Test 3: Splice server
echo -e "${YELLOW}Test 3: splice() zero-copy server (port 8082)${NC}"
"${BUILD_DIR}/splice_server" "${TESTFILE}" > /tmp/test_splice_server.log 2>&1 &
SPLICE_PID=$!
sleep 2

if "${CLIENT}" 8082 > /tmp/test_splice.log 2>&1; then
    wait $SPLICE_PID 2>/dev/null || true
    RECEIVED=$(grep -oP 'received: \K\d+' /tmp/test_splice.log 2>/dev/null || echo "0")
    if [ "$RECEIVED" -gt 0 ]; then
        echo -e "${GREEN}✓ Splice server test passed (received ${RECEIVED} bytes)${NC}"
    else
        echo -e "${RED}✗ Splice server test failed (no data received)${NC}"
        cat /tmp/test_splice.log
        exit 1
    fi
else
    wait $SPLICE_PID 2>/dev/null || true
    echo -e "${RED}✗ Splice server test failed (client error)${NC}"
    cat /tmp/test_splice.log
    exit 1
fi

echo ""
echo -e "${GREEN}All tests passed!${NC}"

