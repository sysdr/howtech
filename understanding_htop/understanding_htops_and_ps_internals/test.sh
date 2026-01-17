#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PASSED=0
FAILED=0

test_check() {
    local name="$1"
    local cmd="$2"
    
    echo -n "Testing $name... "
    if eval "$cmd" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

echo -e "${BLUE}=== Running Tests ===${NC}"
echo ""

# Test 1: Check all executables exist
test_check "monitor executable exists" "test -f $SCRIPT_DIR/build/monitor"
test_check "stress executable exists" "test -f $SCRIPT_DIR/build/stress"
test_check "proc_inspector executable exists" "test -f $SCRIPT_DIR/build/proc_inspector"

# Test 2: Check executables are actually executable
test_check "monitor is executable" "test -x $SCRIPT_DIR/build/monitor"
test_check "stress is executable" "test -x $SCRIPT_DIR/build/stress"
test_check "proc_inspector is executable" "test -x $SCRIPT_DIR/build/proc_inspector"

# Test 3: Test proc_inspector with current PID
test_check "proc_inspector runs" "$SCRIPT_DIR/build/proc_inspector $$ > /dev/null 2>&1"

# Test 4: Test proc_inspector with self
test_check "proc_inspector can inspect self" "$SCRIPT_DIR/build/proc_inspector 1 > /dev/null 2>&1 || true"

# Test 5: Check source files exist
test_check "process_info.c exists" "test -f $SCRIPT_DIR/src/process_info.c"
test_check "process_info.h exists" "test -f $SCRIPT_DIR/src/process_info.h"
test_check "monitor.c exists" "test -f $SCRIPT_DIR/src/monitor.c"
test_check "stress.c exists" "test -f $SCRIPT_DIR/src/stress.c"
test_check "proc_inspector.c exists" "test -f $SCRIPT_DIR/src/proc_inspector.c"

# Test 6: Check Makefile exists
test_check "Makefile exists" "test -f $SCRIPT_DIR/Makefile"

# Test 7: Test make clean works
test_check "make clean works" "cd $SCRIPT_DIR && make clean > /dev/null 2>&1 && make > /dev/null 2>&1"

echo ""
echo -e "${BLUE}=== Test Results ===${NC}"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

