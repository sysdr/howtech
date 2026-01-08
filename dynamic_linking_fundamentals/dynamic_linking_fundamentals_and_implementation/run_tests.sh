#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
PLUGINS_DIR="${SCRIPT_DIR}/plugins"

PASSED=0
FAILED=0

test_passed() {
    echo -e "${GREEN}✓ PASS: $1${NC}"
    PASSED=$((PASSED + 1))
}

test_failed() {
    echo -e "${RED}✗ FAIL: $1${NC}"
    FAILED=$((FAILED + 1))
}

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Dynamic Linking Fundamentals - Tests${NC}"
echo -e "${CYAN}========================================${NC}"
echo

# Test 1: Check if executable exists
echo -e "${BLUE}Test 1: Executable exists${NC}"
if [ -f "${BUILD_DIR}/plugin_demo" ] && [ -x "${BUILD_DIR}/plugin_demo" ]; then
    test_passed "plugin_demo executable exists and is executable"
else
    test_failed "plugin_demo executable missing or not executable"
    exit 1
fi

# Test 2: Check if all plugins exist
echo -e "${BLUE}Test 2: All plugins exist${NC}"
PLUGINS=("plugin_reverse.so" "plugin_rot13.so" "plugin_upper.so")
for plugin in "${PLUGINS[@]}"; do
    if [ -f "${PLUGINS_DIR}/${plugin}" ]; then
        test_passed "Plugin ${plugin} exists"
    else
        test_failed "Plugin ${plugin} missing"
    fi
done

# Test 3: Run demo with RTLD_NOW
echo -e "${BLUE}Test 3: Demo runs with RTLD_NOW${NC}"
if "${BUILD_DIR}/plugin_demo" > /tmp/demo_output_now.txt 2>&1; then
    if grep -q "Loaded 'reverse'" /tmp/demo_output_now.txt && \
       grep -q "Loaded 'rot13'" /tmp/demo_output_now.txt && \
       grep -q "Loaded 'upper'" /tmp/demo_output_now.txt; then
        test_passed "Demo runs successfully with RTLD_NOW and loads all plugins"
    else
        test_failed "Demo runs but plugins not loaded correctly"
    fi
else
    test_failed "Demo failed to run with RTLD_NOW"
fi

# Test 4: Run demo with RTLD_LAZY
echo -e "${BLUE}Test 4: Demo runs with RTLD_LAZY${NC}"
if "${BUILD_DIR}/plugin_demo" --lazy > /tmp/demo_output_lazy.txt 2>&1; then
    if grep -q "Loaded 'reverse'" /tmp/demo_output_lazy.txt && \
       grep -q "Loaded 'rot13'" /tmp/demo_output_lazy.txt && \
       grep -q "Loaded 'upper'" /tmp/demo_output_lazy.txt; then
        test_passed "Demo runs successfully with RTLD_LAZY and loads all plugins"
    else
        test_failed "Demo runs but plugins not loaded correctly with lazy binding"
    fi
else
    test_failed "Demo failed to run with RTLD_LAZY"
fi

# Test 5: Verify plugin outputs
echo -e "${BLUE}Test 5: Plugin outputs are correct${NC}"
if grep -q 'Output: "!gnidaoL cimanyD olleH"' /tmp/demo_output_now.txt && \
   grep -q 'Output: "Uryyb Qlanzvp Ybnqvat!"' /tmp/demo_output_now.txt && \
   grep -q 'Output: "HELLO DYNAMIC LOADING!"' /tmp/demo_output_now.txt; then
    test_passed "All plugin outputs are correct (reverse, rot13, upper)"
else
    test_failed "Plugin outputs are incorrect"
fi

# Test 6: Check for memory mappings
echo -e "${BLUE}Test 6: Memory mappings shown${NC}"
if grep -q "Memory Mappings" /tmp/demo_output_now.txt; then
    test_passed "Memory mappings are displayed"
else
    test_failed "Memory mappings not shown"
fi

# Test 7: Check for cleanup
echo -e "${BLUE}Test 7: Cleanup executed${NC}"
if grep -q "Cleanup" /tmp/demo_output_now.txt && \
   grep -q "cleaned up" /tmp/demo_output_now.txt; then
    test_passed "Plugin cleanup executed"
else
    test_failed "Plugin cleanup not executed"
fi

# Summary
echo
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Test Summary${NC}"
echo -e "${CYAN}========================================${NC}"
echo -e "Passed: ${GREEN}${PASSED}${NC}"
echo -e "Failed: ${RED}${FAILED}${NC}"
echo

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

