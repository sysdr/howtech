#!/bin/bash
set +e  # Don't exit on error, we'll handle it manually

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}Running Tests for LD_PRELOAD Demo...${NC}\n"

PASSED=0
FAILED=0

# Test 1: Check if source files exist
echo -e "${YELLOW}[Test 1]${NC} Checking source files..."
if [ -f "src/malloc_hook.c" ] && [ -f "src/test_program.c" ] && [ -f "src/monitor.c" ]; then
    echo -e "${GREEN}✓${NC} All source files exist"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} Missing source files"
    ((FAILED++))
fi

# Test 2: Check if build files exist
echo -e "${YELLOW}[Test 2]${NC} Checking build files..."
if [ -f "build/malloc_hook.so" ] && [ -f "build/test_program" ] && [ -f "build/monitor" ]; then
    echo -e "${GREEN}✓${NC} All build files exist"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} Missing build files"
    ((FAILED++))
fi

# Test 3: Check if test_program runs without LD_PRELOAD
echo -e "${YELLOW}[Test 3]${NC} Testing program execution without LD_PRELOAD..."
if ./build/test_program > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Program runs successfully without LD_PRELOAD"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} Program failed to run"
    ((FAILED++))
fi

# Test 4: Check if test_program runs with LD_PRELOAD
echo -e "${YELLOW}[Test 4]${NC} Testing program execution with LD_PRELOAD..."
if LD_PRELOAD="$SCRIPT_DIR/build/malloc_hook.so" ./build/test_program > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Program runs successfully with LD_PRELOAD"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} Program failed to run with LD_PRELOAD"
    ((FAILED++))
fi

# Test 5: Verify LD_PRELOAD interception is working
echo -e "${YELLOW}[Test 5]${NC} Verifying LD_PRELOAD interception..."
OUTPUT=$(LD_PRELOAD="$SCRIPT_DIR/build/malloc_hook.so" ./build/test_program 2>&1)
if echo "$OUTPUT" | grep -q "\[HOOK\]"; then
    echo -e "${GREEN}✓${NC} LD_PRELOAD interception is working"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} LD_PRELOAD interception not detected"
    ((FAILED++))
fi

# Test 6: Check if monitor program exists and is executable
echo -e "${YELLOW}[Test 6]${NC} Checking monitor program..."
if [ -x "build/monitor" ]; then
    echo -e "${GREEN}✓${NC} Monitor program is executable"
    ((PASSED++))
else
    echo -e "${RED}✗${NC} Monitor program not executable"
    ((FAILED++))
fi

# Summary
echo -e "\n${CYAN}═════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}Test Summary${NC}"
echo -e "${CYAN}═════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed: ${FAILED}${NC}"
    exit 1
else
    echo -e "${GREEN}Failed: ${FAILED}${NC}"
    echo -e "\n${GREEN}All tests passed!${NC}\n"
    exit 0
fi

