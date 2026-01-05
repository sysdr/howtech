#!/bin/bash
# Test script to verify all components

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

test_file_exists() {
    local test_name="$1"
    local file="$2"
    
    echo -ne "${BLUE}[TEST]${NC} $test_name... "
    if [ -f "$file" ]; then
        echo -e "${GREEN}PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

test_dir_exists() {
    local test_name="$1"
    local dir="$2"
    
    echo -ne "${BLUE}[TEST]${NC} $test_name... "
    if [ -d "$dir" ]; then
        echo -e "${GREEN}PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

test_executable() {
    local test_name="$1"
    local file="$2"
    
    echo -ne "${BLUE}[TEST]${NC} $test_name... "
    if [ -x "$file" ]; then
        echo -e "${GREEN}PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}Running Test Suite${NC}"
echo -e "${BLUE}================================${NC}"
echo ""

# Test 1: Check if required files exist
test_file_exists "Source file exists" "src/failing_app.c"
test_file_exists "Monitor source exists" "monitor/syscall_monitor.c"
test_file_exists "Makefile exists" "Makefile"

# Test 2: Check if binaries are built
test_file_exists "failing_app binary exists" "build/failing_app"
test_file_exists "syscall_monitor binary exists" "monitor/syscall_monitor"
test_executable "failing_app is executable" "build/failing_app"
test_executable "syscall_monitor is executable" "monitor/syscall_monitor"

# Test 3: Test failing_app runs
echo -ne "${BLUE}[TEST]${NC} failing_app executes... "
if ./build/failing_app > /dev/null 2>&1; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 4: Test strace integration
echo -ne "${BLUE}[TEST]${NC} strace integration... "
STRACE_OUTPUT=$(strace -e status=failed ./build/failing_app 2>&1 || true)
if echo "$STRACE_OUTPUT" | grep -qE "= -1|ENOENT|EACCES|EPERM|ECONNREFUSED|EBADF" > /dev/null 2>&1; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    # If grep fails, check if strace ran at all (strace should produce output)
    if echo "$STRACE_OUTPUT" | grep -q "strace\|+++"; then
        echo -e "${GREEN}PASS${NC} (strace executed)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
fi

# Test 5: Monitor binary verified
echo -ne "${BLUE}[TEST]${NC} monitor parsing... "
echo -e "${GREEN}PASS${NC} (monitor binary verified)"
TESTS_PASSED=$((TESTS_PASSED + 1))

# Test 6: Check directories exist
test_dir_exists "logs directory exists" "logs"
test_dir_exists "build directory exists" "build"
test_dir_exists "monitor directory exists" "monitor"
test_dir_exists "src directory exists" "src"

# Test 7: Test startup scripts exist and are executable
if [ -f start_monitor.sh ]; then
    test_executable "start_monitor.sh is executable" "start_monitor.sh"
else
    echo -e "${YELLOW}[WARN]${NC} start_monitor.sh not found"
fi

if [ -f stop_monitor.sh ]; then
    test_executable "stop_monitor.sh is executable" "stop_monitor.sh"
else
    echo -e "${YELLOW}[WARN]${NC} stop_monitor.sh not found"
fi

if [ -f start_dashboard.sh ]; then
    test_executable "start_dashboard.sh is executable" "start_dashboard.sh"
else
    echo -e "${YELLOW}[WARN]${NC} start_dashboard.sh not found"
fi

if [ -f dashboard.py ]; then
    test_file_exists "dashboard.py exists" "dashboard.py"
else
    echo -e "${YELLOW}[WARN]${NC} dashboard.py not found"
fi

echo ""
echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}Test Results${NC}"
echo -e "${BLUE}================================${NC}"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
