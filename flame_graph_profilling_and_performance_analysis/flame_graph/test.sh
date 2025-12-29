#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=== Running Tests ===${NC}\n"

TESTS_PASSED=0
TESTS_FAILED=0

# Test 1: Check if source files exist
echo -e "${BLUE}Test 1: Source files${NC}"
if [ -f "src/hotspot_demo.c" ] && [ -f "src/monitor.c" ]; then
    echo -e "${GREEN}✓ Source files exist${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Source files missing${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 2: Check if binaries are built
echo -e "${BLUE}Test 2: Compiled binaries${NC}"
if [ -f "build/hotspot_demo" ] && [ -f "build/monitor" ]; then
    echo -e "${GREEN}✓ Binaries exist${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Binaries missing${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 3: Check if binaries are executable
echo -e "${BLUE}Test 3: Binary executability${NC}"
if [ -x "build/hotspot_demo" ] && [ -x "build/monitor" ]; then
    echo -e "${GREEN}✓ Binaries are executable${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Binaries are not executable${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 4: Test hotspot_demo runs (short test)
echo -e "${BLUE}Test 4: hotspot_demo execution${NC}"
timeout 2 ./build/hotspot_demo > /dev/null 2>&1 && EXIT_CODE=$? || EXIT_CODE=$?
if [ $EXIT_CODE -eq 124 ] || [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ hotspot_demo runs${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ hotspot_demo failed to run (exit code: $EXIT_CODE)${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 5: Test monitor with invalid PID (should handle gracefully)
echo -e "${BLUE}Test 5: monitor error handling${NC}"
timeout 1 ./build/monitor 99999 > /dev/null 2>&1 && EXIT_CODE=$? || EXIT_CODE=$?
if [ $EXIT_CODE -eq 124 ] || [ $EXIT_CODE -eq 0 ] || [ $EXIT_CODE -eq 1 ]; then
    echo -e "${GREEN}✓ monitor handles errors${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ monitor error handling failed${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 6: Check FlameGraph tools
echo -e "${BLUE}Test 6: FlameGraph tools${NC}"
if [ -d "FlameGraph" ] && [ -f "FlameGraph/flamegraph.pl" ] && [ -f "FlameGraph/stackcollapse-perf.pl" ]; then
    echo -e "${GREEN}✓ FlameGraph tools available${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ FlameGraph tools missing${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 7: Check Makefiles
echo -e "${BLUE}Test 7: Makefiles${NC}"
if [ -f "Makefile" ] && [ -f "Makefile.monitor" ]; then
    echo -e "${GREEN}✓ Makefiles exist${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Makefiles missing${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Test 8: Check Dockerfile
echo -e "${BLUE}Test 8: Dockerfile${NC}"
if [ -f "Dockerfile" ]; then
    echo -e "${GREEN}✓ Dockerfile exists${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Dockerfile missing${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

# Summary
echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}Failed: $TESTS_FAILED${NC}"
    exit 0
fi

