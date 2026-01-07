#!/bin/bash
# Test script for kernel module
# Tests module loading, parameters, and functionality

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
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAILED++))
    fi
}

echo -e "${BLUE}=== Kernel Module Test Suite ===${NC}\n"

# Test 1: Check if module file exists
echo -e "${BLUE}Test 1: Module file exists${NC}"
if [ -f "src/hello_module.ko" ]; then
    test_result 0 "Module file found"
else
    test_result 1 "Module file not found - run 'make' first"
    exit 1
fi

# Test 2: Check module information
echo -e "\n${BLUE}Test 2: Module information${NC}"
if modinfo src/hello_module.ko >/dev/null 2>&1; then
    test_result 0 "modinfo works"
    MODULE_VERSION=$(modinfo src/hello_module.ko | grep "^version:" | awk '{print $2}' || echo "")
    if [ -n "$MODULE_VERSION" ]; then
        test_result 0 "Module version: $MODULE_VERSION"
    fi
else
    test_result 1 "modinfo failed"
fi

# Test 3: Load module (if not already loaded)
echo -e "\n${BLUE}Test 3: Load module${NC}"
if lsmod | grep -q "^hello_module"; then
    echo -e "${YELLOW}Module already loaded, unloading first...${NC}"
    sudo rmmod hello_module 2>/dev/null || true
    sleep 1
fi

if sudo insmod src/hello_module.ko 2>&1; then
    test_result 0 "Module loaded successfully"
    sleep 1
    if lsmod | grep -q "^hello_module"; then
        test_result 0 "Module appears in lsmod"
    else
        test_result 1 "Module not in lsmod"
    fi
else
    test_result 1 "Failed to load module"
    echo "Check dmesg: dmesg | tail -20"
fi

# Test 4: Check kernel logs
echo -e "\n${BLUE}Test 4: Kernel logs${NC}"
LOG_COUNT=$(dmesg | grep -c "hello_module" || echo "0")
if [ "$LOG_COUNT" -gt 0 ]; then
    test_result 0 "Found $LOG_COUNT kernel log entries"
    dmesg | grep "hello_module" | tail -3
else
    test_result 1 "No kernel log entries found"
fi

# Test 5: Check sysfs parameters
echo -e "\n${BLUE}Test 5: Sysfs parameters${NC}"
if [ -d "/sys/module/hello_module/parameters" ]; then
    test_result 0 "Parameters directory exists"
    if [ -f "/sys/module/hello_module/parameters/name" ]; then
        NAME_PARAM=$(cat /sys/module/hello_module/parameters/name)
        test_result 0 "Parameter 'name' = $NAME_PARAM"
    fi
    if [ -f "/sys/module/hello_module/parameters/count" ]; then
        COUNT_PARAM=$(cat /sys/module/hello_module/parameters/count)
        test_result 0 "Parameter 'count' = $COUNT_PARAM"
    fi
else
    test_result 1 "Parameters directory not found"
fi

# Test 6: Load with parameters
echo -e "\n${BLUE}Test 6: Load with parameters${NC}"
sudo rmmod hello_module 2>/dev/null || true
sleep 1
if sudo insmod src/hello_module.ko name="TestUser" count=3 2>&1; then
    test_result 0 "Module loaded with parameters"
    sleep 1
    NAME_CHECK=$(cat /sys/module/hello_module/parameters/name)
    COUNT_CHECK=$(cat /sys/module/hello_module/parameters/count)
    if [ "$NAME_CHECK" = "TestUser" ]; then
        test_result 0 "Parameter 'name' set correctly: $NAME_CHECK"
    else
        test_result 1 "Parameter 'name' incorrect: expected 'TestUser', got '$NAME_CHECK'"
    fi
    if [ "$COUNT_CHECK" = "3" ]; then
        test_result 0 "Parameter 'count' set correctly: $COUNT_CHECK"
    else
        test_result 1 "Parameter 'count' incorrect: expected '3', got '$COUNT_CHECK'"
    fi
else
    test_result 1 "Failed to load module with parameters"
fi

# Test 7: Unload module
echo -e "\n${BLUE}Test 7: Unload module${NC}"
if sudo rmmod hello_module 2>&1; then
    test_result 0 "Module unloaded successfully"
    sleep 1
    if ! lsmod | grep -q "^hello_module"; then
        test_result 0 "Module removed from lsmod"
    else
        test_result 1 "Module still in lsmod"
    fi
else
    test_result 1 "Failed to unload module"
fi

# Test 8: Check unload logs
echo -e "\n${BLUE}Test 8: Unload kernel logs${NC}"
UNLOAD_LOGS=$(dmesg | grep -c "hello_module.*Goodbye" || echo "0")
if [ "$UNLOAD_LOGS" -gt 0 ]; then
    test_result 0 "Found unload log entries"
else
    test_result 1 "No unload log entries found"
fi

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

