#!/bin/bash
# Test script for Core Dump Mechanics

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PASSED=0
FAILED=0

test_check() {
    local test_name="$1"
    local command="$2"
    
    echo -e "${BLUE}Testing: $test_name${NC}"
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS: $test_name${NC}"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}✗ FAIL: $test_name${NC}"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
echo -e "${BLUE}Core Dump Mechanics Test Suite${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
echo ""

# Test 1: Check if required directories exist
test_check "src directory exists" "[ -d '$SCRIPT_DIR/src' ]"
test_check "build directory exists" "[ -d '$SCRIPT_DIR/build' ]"
test_check "cores directory exists" "[ -d '$SCRIPT_DIR/cores' ]"

# Test 2: Check if source files exist
test_check "crash_test.c exists" "[ -f '$SCRIPT_DIR/src/crash_test.c' ]"
test_check "core_monitor.c exists" "[ -f '$SCRIPT_DIR/src/core_monitor.c' ]"
test_check "gdb_analyze.py exists" "[ -f '$SCRIPT_DIR/src/gdb_analyze.py' ]"

# Test 3: Check if binaries are built
test_check "crash_test binary exists" "[ -f '$SCRIPT_DIR/build/crash_test' ]"
test_check "core_monitor binary exists" "[ -f '$SCRIPT_DIR/build/core_monitor' ]"
test_check "crash_test is executable" "[ -x '$SCRIPT_DIR/build/crash_test' ]"
test_check "core_monitor is executable" "[ -x '$SCRIPT_DIR/build/core_monitor' ]"

# Test 4: Check if Makefile and Dockerfile exist
test_check "Makefile exists" "[ -f '$SCRIPT_DIR/Makefile' ]"
test_check "Dockerfile exists" "[ -f '$SCRIPT_DIR/Dockerfile' ]"

# Test 5: Test crash_test can run (it will crash, but that's expected)
echo -e "${BLUE}Testing: crash_test program runs (will segfault)${NC}"
if timeout 5 "$SCRIPT_DIR/build/crash_test" 1 > /dev/null 2>&1; then
    echo -e "${YELLOW}⚠ WARN: crash_test did not crash as expected${NC}"
    FAILED=$((FAILED + 1))
else
    echo -e "${GREEN}✓ PASS: crash_test crashes as expected${NC}"
    PASSED=$((PASSED + 1))
fi

# Test 6: Test core_monitor can run
echo -e "${BLUE}Testing: core_monitor program runs${NC}"
if "$SCRIPT_DIR/build/core_monitor" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: core_monitor runs successfully${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}✗ FAIL: core_monitor failed to run${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 7: Check if binaries are linked correctly
test_check "crash_test is valid ELF" "file '$SCRIPT_DIR/build/crash_test' | grep -q 'ELF'"
test_check "core_monitor is valid ELF" "file '$SCRIPT_DIR/build/core_monitor' | grep -q 'ELF'"

# Test 8: Check if gdb_analyze.py is valid Python
echo -e "${BLUE}Testing: gdb_analyze.py is valid Python${NC}"
if python3 -m py_compile "$SCRIPT_DIR/src/gdb_analyze.py" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: gdb_analyze.py is valid Python${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}✗ FAIL: gdb_analyze.py has syntax errors${NC}"
    FAILED=$((FAILED + 1))
fi

# Summary
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
echo -e "${GREEN}Passed: $PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}Failed: $FAILED${NC}"
    exit 0
fi

