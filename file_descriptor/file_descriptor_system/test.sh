#!/bin/bash
# Test script for File Descriptor Tracking Demo
# Tests FD leak detection, monitoring, and cleanup

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED=0
FAILED=0

test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        FAILED=$((FAILED + 1))
    fi
}

echo -e "${BLUE}=== File Descriptor Demo Test Suite ===${NC}\n"

# Test 1: Check if binaries exist
echo -e "${BLUE}Test 1: Binaries exist${NC}"
if [ -f "build/fd_leak" ]; then
    test_result 0 "fd_leak binary found"
else
    test_result 1 "fd_leak binary not found - run 'bash setup.sh' first"
    exit 1
fi

if [ -f "build/fd_monitor" ]; then
    test_result 0 "fd_monitor binary found"
else
    test_result 1 "fd_monitor binary not found - run 'bash setup.sh' first"
    exit 1
fi

# Test 2: Check if binaries are executable
echo -e "\n${BLUE}Test 2: Binaries are executable${NC}"
if [ -x "build/fd_leak" ]; then
    test_result 0 "fd_leak is executable"
else
    test_result 1 "fd_leak is not executable"
fi

if [ -x "build/fd_monitor" ]; then
    test_result 0 "fd_monitor is executable"
else
    test_result 1 "fd_monitor is not executable"
fi

# Test 3: Test fd_leak leak mode
echo -e "\n${BLUE}Test 3: FD leak functionality${NC}"
./build/fd_leak leak 5 > /tmp/test_leak.log 2>&1 &
LEAK_PID=$!
sleep 2

if kill -0 $LEAK_PID 2>/dev/null; then
    test_result 0 "fd_leak process started"
    
    # Check if FDs were created
    FD_COUNT=$(ls /proc/$LEAK_PID/fd 2>/dev/null | wc -l || echo "0")
    if [ "$FD_COUNT" -gt 5 ]; then
        test_result 0 "FDs created (count: $FD_COUNT)"
    else
        test_result 1 "Expected more FDs, got: $FD_COUNT"
    fi
    
    # Check for leaked files
    LEAKED_FILES=$(ls /tmp/leaked_file_*.txt 2>/dev/null | wc -l || echo "0")
    if [ "$LEAKED_FILES" -gt 0 ]; then
        test_result 0 "Leaked files created (count: $LEAKED_FILES)"
    else
        test_result 1 "No leaked files found"
    fi
    
    kill $LEAK_PID 2>/dev/null || true
    wait $LEAK_PID 2>/dev/null || true
else
    test_result 1 "fd_leak process failed to start"
    cat /tmp/test_leak.log
fi

# Test 4: Test fd_monitor summary
echo -e "\n${BLUE}Test 4: FD monitor summary${NC}"
if ./build/fd_monitor summary > /tmp/test_summary.log 2>&1; then
    test_result 0 "fd_monitor summary executed"
    if grep -q "PID" /tmp/test_summary.log; then
        test_result 0 "Summary output contains process information"
    else
        test_result 1 "Summary output missing process information"
    fi
else
    test_result 1 "fd_monitor summary failed"
    cat /tmp/test_summary.log
fi

# Test 5: Test fd_monitor watch mode (short test)
echo -e "\n${BLUE}Test 5: FD monitor watch mode${NC}"
./build/fd_leak leak 10 > /dev/null 2>&1 &
WATCH_PID=$!
sleep 1

if kill -0 $WATCH_PID 2>/dev/null; then
    timeout 3 ./build/fd_monitor watch $WATCH_PID > /tmp/test_watch.log 2>&1 &
    WATCHER_PID=$!
    sleep 2
    
    if grep -q "Total FDs" /tmp/test_watch.log 2>/dev/null; then
        test_result 0 "Monitor watch mode working"
    else
        test_result 1 "Monitor watch mode not producing output"
    fi
    
    kill $WATCHER_PID 2>/dev/null || true
    kill $WATCH_PID 2>/dev/null || true
    wait $WATCH_PID 2>/dev/null || true
else
    test_result 1 "Test process failed to start"
fi

# Test 6: Test deleted file scenario
echo -e "\n${BLUE}Test 6: Deleted file scenario${NC}"
./build/fd_leak deleted > /tmp/test_deleted.log 2>&1 &
DEL_PID=$!
sleep 3

if kill -0 $DEL_PID 2>/dev/null; then
    test_result 0 "Deleted file scenario process started"
    
    # Check if file was deleted but still accessible
    if lsof -p $DEL_PID 2>/dev/null | grep -q "deleted"; then
        test_result 0 "Deleted file detected via lsof"
    elif ls -la /proc/$DEL_PID/fd 2>/dev/null | grep -q "deleted"; then
        test_result 0 "Deleted file detected via /proc"
    else
        test_result 1 "Deleted file not detected"
    fi
    
    kill $DEL_PID 2>/dev/null || true
    wait $DEL_PID 2>/dev/null || true
else
    test_result 1 "Deleted file scenario process failed"
    cat /tmp/test_deleted.log
fi

# Test 7: Test /proc access
echo -e "\n${BLUE}Test 7: /proc filesystem access${NC}"
TEST_PID=$$
if [ -d "/proc/$TEST_PID/fd" ]; then
    test_result 0 "/proc/$TEST_PID/fd directory accessible"
    FD_COUNT=$(ls /proc/$TEST_PID/fd 2>/dev/null | wc -l || echo "0")
    if [ "$FD_COUNT" -gt 0 ]; then
        test_result 0 "Current process has $FD_COUNT FDs"
    else
        test_result 1 "No FDs found for current process"
    fi
else
    test_result 1 "/proc filesystem not accessible"
fi

# Test 8: Test lsof availability (if installed)
echo -e "\n${BLUE}Test 8: lsof availability${NC}"
if command -v lsof >/dev/null 2>&1; then
    test_result 0 "lsof command available"
    if lsof -p $$ >/dev/null 2>&1; then
        test_result 0 "lsof can list process FDs"
    else
        test_result 1 "lsof failed to list FDs"
    fi
else
    echo -e "${YELLOW}  SKIP: lsof not installed${NC}"
fi

# Cleanup
rm -f /tmp/leaked_file_*.txt /tmp/deleted_but_open.txt 2>/dev/null || true

# Summary
echo -e "\n${BLUE}=== Test Summary ===${NC}"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
TOTAL=$((PASSED + FAILED))
if [ $TOTAL -gt 0 ]; then
    PERCENT=$((PASSED * 100 / TOTAL))
    echo "Success rate: $PERCENT%"
fi

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed${NC}"
    exit 1
fi

