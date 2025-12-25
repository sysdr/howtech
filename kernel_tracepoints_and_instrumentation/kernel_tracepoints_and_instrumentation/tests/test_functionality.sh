#!/bin/bash

set -eo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

TESTS_PASSED=0
TESTS_FAILED=0

test_check() {
    local name="$1"
    local condition="$2"
    
    if eval "$condition"; then
        echo -e "${GREEN}✓${NC} $name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}✗${NC} $name"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

echo "=== Running Functionality Tests ==="
echo ""

# Clean up any existing processes
pkill -f "build/target_program" 2>/dev/null || true
pkill -f "build/perf_monitor" 2>/dev/null || true
sleep 1

# Test 1: Target program starts and produces output
echo -e "${BLUE}Test 1: Target program functionality${NC}"
mkdir -p logs
"$PROJECT_DIR/build/target_program" > logs/test_target.log 2>&1 &
TARGET_PID=$!
sleep 3

if kill -0 $TARGET_PID 2>/dev/null; then
    test_check "Target program runs successfully" "true"
    
    # Check if output contains expected content
    if grep -q "Target program starting" logs/test_target.log && \
       grep -q "my_function called" logs/test_target.log; then
        test_check "Target program produces expected output" "true"
    else
        test_check "Target program produces expected output" "false"
    fi
    
    # Clean up
    kill $TARGET_PID 2>/dev/null || true
    wait $TARGET_PID 2>/dev/null || true
else
    test_check "Target program runs successfully" "false"
    cat logs/test_target.log
fi

# Test 2: Perf monitor (if we have permissions)
echo -e "${BLUE}Test 2: Perf monitor functionality${NC}"
if [ "$EUID" -eq 0 ] || capsh --print 2>/dev/null | grep -q "cap_perfmon"; then
    # Start target program again
    "$PROJECT_DIR/build/target_program" > /dev/null 2>&1 &
    TARGET_PID=$!
    sleep 1
    
    if kill -0 $TARGET_PID 2>/dev/null; then
        # Try to start perf monitor
        timeout 3 "$PROJECT_DIR/build/perf_monitor" $TARGET_PID > logs/test_monitor.log 2>&1 &
        MONITOR_PID=$!
        sleep 2
        
        if [ -f logs/test_monitor.log ] && ! grep -q "Error opening perf events" logs/test_monitor.log; then
            test_check "Perf monitor runs successfully" "true"
        else
            test_check "Perf monitor runs successfully" "false"
        fi
        
        kill $MONITOR_PID 2>/dev/null || true
        wait $MONITOR_PID 2>/dev/null || true
        kill $TARGET_PID 2>/dev/null || true
        wait $TARGET_PID 2>/dev/null || true
    fi
else
    echo -e "${YELLOW}Skipping perf monitor test (requires root or CAP_PERFMON)${NC}"
fi

# Test 3: Startup script
echo -e "${BLUE}Test 3: Startup script functionality${NC}"
if [ -x "$PROJECT_DIR/startup.sh" ]; then
    # Run startup script
    timeout 5 "$PROJECT_DIR/startup.sh" > logs/test_startup.log 2>&1 || true
    sleep 1
    
    # Check if processes are running
    if pgrep -f "build/target_program" > /dev/null; then
        test_check "Startup script launches target program" "true"
        
        # Clean up
        "$PROJECT_DIR/stop.sh" > /dev/null 2>&1 || true
        sleep 1
    else
        test_check "Startup script launches target program" "false"
    fi
else
    test_check "Startup script is executable" "false"
fi

# Test 4: Stop script
echo -e "${BLUE}Test 4: Stop script functionality${NC}"
if [ -x "$PROJECT_DIR/stop.sh" ]; then
    # Start processes
    "$PROJECT_DIR/build/target_program" > /dev/null 2>&1 &
    sleep 1
    
    # Run stop script
    "$PROJECT_DIR/stop.sh" > /dev/null 2>&1
    sleep 1
    
    # Check if processes are stopped
    if ! pgrep -f "build/target_program" > /dev/null; then
        test_check "Stop script terminates processes" "true"
    else
        test_check "Stop script terminates processes" "false"
    fi
else
    test_check "Stop script is executable" "false"
fi

echo ""
echo "=== Test Results ==="
echo "Passed: $TESTS_PASSED"
echo "Failed: $TESTS_FAILED"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

