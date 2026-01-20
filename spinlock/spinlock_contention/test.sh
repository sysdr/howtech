#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║              Running Tests for Spinlock Demo                ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Test 1: Check if binaries exist
echo -e "${YELLOW}[Test 1] Checking if all binaries exist...${NC}"
BINARIES=("build/spinlock_livelock" "build/rcu_stall" "build/monitor")
for bin in "${BINARIES[@]}"; do
    if [ -f "$bin" ]; then
        echo -e "${GREEN}✓${NC} $bin exists"
    else
        echo -e "${RED}✗${NC} $bin missing!"
        exit 1
    fi
done

# Test 2: Check if binaries are executable
echo -e "${YELLOW}[Test 2] Checking if binaries are executable...${NC}"
for bin in "${BINARIES[@]}"; do
    if [ -x "$bin" ]; then
        echo -e "${GREEN}✓${NC} $bin is executable"
    else
        echo -e "${RED}✗${NC} $bin is not executable!"
        exit 1
    fi
done

# Test 3: Quick functionality test for spinlock_livelock (timeout after 10 seconds)
echo -e "${YELLOW}[Test 3] Testing spinlock_livelock (quick test with timeout)...${NC}"
timeout 10s ./build/spinlock_livelock > /tmp/spinlock_test.out 2>&1 || true
if grep -q "Thread" /tmp/spinlock_test.out; then
    echo -e "${GREEN}✓${NC} spinlock_livelock produces output"
else
    echo -e "${RED}✗${NC} spinlock_livelock test failed"
    cat /tmp/spinlock_test.out
    exit 1
fi

# Test 4: Quick functionality test for monitor (should start and show output)
echo -e "${YELLOW}[Test 4] Testing monitor (quick test)...${NC}"
timeout 6s ./build/monitor > /tmp/monitor_test.out 2>&1 || true
if grep -q "CPU Usage" /tmp/monitor_test.out || grep -q "Live Lock Detection" /tmp/monitor_test.out; then
    echo -e "${GREEN}✓${NC} monitor produces output"
else
    echo -e "${RED}✗${NC} monitor test failed"
    cat /tmp/monitor_test.out
    exit 1
fi

echo ""
echo -e "${GREEN}✓ All tests passed!${NC}"
rm -f /tmp/spinlock_test.out /tmp/monitor_test.out

