#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo ""
echo "╔════════════════════════════════════════════════╗"
echo "║  Running Validation Tests                      ║"
echo "╚════════════════════════════════════════════════╝"
echo ""

# Check if we're in the right directory
if [ ! -f "setup.sh" ]; then
    echo -e "${RED}✗ Error: Must run from windows_debugging directory${NC}"
    exit 1
fi

cd "$(dirname "$(readlink -f "$0")")" || exit 1

PASSED=0
FAILED=0

# Test 1: Verify all source files exist
echo -e "${BLUE}[Test 1] Checking source files...${NC}"
for file in src/stack_demo.c src/breakpoint_demo.c src/gdb_script.txt src/gdb_breakpoint.txt; do
    if [ -f "$file" ]; then
        echo -e "  ${GREEN}✓${NC} $file"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} $file (missing)"
        FAILED=$((FAILED + 1))
    fi
done

# Test 2: Verify compiled binaries exist
echo -e "${BLUE}[Test 2] Checking compiled binaries...${NC}"
for file in build/stack_demo build/breakpoint_demo; do
    if [ -f "$file" ] && [ -x "$file" ]; then
        echo -e "  ${GREEN}✓${NC} $file (executable)"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} $file (missing or not executable)"
        FAILED=$((FAILED + 1))
    fi
done

# Test 3: Run stack_demo and verify output
echo -e "${BLUE}[Test 3] Running stack_demo...${NC}"
if OUTPUT=$(./build/stack_demo 2>&1); then
    if echo "$OUTPUT" | grep -q "Level 4"; then
        echo -e "  ${GREEN}✓${NC} stack_demo executed successfully"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} stack_demo output invalid"
        FAILED=$((FAILED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} stack_demo failed to execute"
    FAILED=$((FAILED + 1))
fi

# Test 4: Run breakpoint_demo and verify output
echo -e "${BLUE}[Test 4] Running breakpoint_demo...${NC}"
if OUTPUT=$(./build/breakpoint_demo 2>&1); then
    if echo "$OUTPUT" | grep -q "critical_function"; then
        echo -e "  ${GREEN}✓${NC} breakpoint_demo executed successfully"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} breakpoint_demo output invalid"
        FAILED=$((FAILED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} breakpoint_demo failed to execute"
    FAILED=$((FAILED + 1))
fi

# Test 5: Check for duplicate running processes
echo -e "${BLUE}[Test 5] Checking for duplicate services...${NC}"
if ps aux | grep -E "(stack_demo|breakpoint_demo)" | grep -v grep > /dev/null 2>&1; then
    RUNNING_COUNT=$(ps aux | grep -E "(stack_demo|breakpoint_demo)" | grep -v grep | wc -l)
    echo -e "  ${RED}✗${NC} Found $RUNNING_COUNT running process(es)"
    FAILED=$((FAILED + 1))
else
    echo -e "  ${GREEN}✓${NC} No duplicate services running"
    PASSED=$((PASSED + 1))
fi

# Summary
echo ""
echo "╔════════════════════════════════════════════════╗"
echo "║  Test Summary                                  ║"
echo "╚════════════════════════════════════════════════╝"
echo ""
echo -e "  Passed: ${GREEN}$PASSED${NC}"
echo -e "  Failed: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi

