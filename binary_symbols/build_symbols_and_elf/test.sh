#!/bin/bash
set -eo pipefail

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Running tests for Binary Symbol Analysis...${NC}"
echo ""

PASSED=0
FAILED=0

# Test 1: Check if binaries exist
echo -n "Test 1: Checking if binaries exist... "
if [ -f "build/demo" ] && [ -f "build/libmylib.so" ] && [ -f "build/monitor" ]; then
    echo -e "${GREEN}PASS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 2: Check if demo runs successfully
echo -n "Test 2: Running demo program... "
if ./build/demo > /tmp/demo_test.out 2>&1; then
    if grep -q "Symbol Resolution Demo" /tmp/demo_test.out; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
    fi
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 3: Check if library symbols are exported
echo -n "Test 3: Checking library symbols... "
if nm -D build/libmylib.so | grep -q "increment_global"; then
    echo -e "${GREEN}PASS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 4: Check if hidden symbol is not exported
echo -n "Test 4: Checking hidden symbol is not exported... "
if ! nm -D build/libmylib.so | grep -q "hidden_function"; then
    echo -e "${GREEN}PASS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 5: Check if log files exist
echo -n "Test 5: Checking log files... "
if [ -f "logs/library_symbols.txt" ] && [ -f "logs/detailed_symbols.txt" ] && \
   [ -f "logs/disassembly.txt" ] && [ -f "logs/headers.txt" ]; then
    echo -e "${GREEN}PASS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 6: Check if demo uses library correctly
echo -n "Test 6: Checking demo uses library... "
if ./build/demo 2>&1 | grep -q "After increment: 101"; then
    echo -e "${GREEN}PASS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=========================================="
echo -e "Tests passed: ${GREEN}${PASSED}${NC}"
echo -e "Tests failed: ${RED}${FAILED}${NC}"
echo "=========================================="

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi

