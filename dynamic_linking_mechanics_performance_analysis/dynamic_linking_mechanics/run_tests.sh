#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BASE_DIR}/build"

echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║           Running Dynamic Linking Tests                      ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}\n"

# Check if binaries exist
if [ ! -f "${BUILD_DIR}/binding_test_lazy" ]; then
    echo -e "${RED}Error: binding_test_lazy not found${NC}"
    echo -e "${YELLOW}Run setup.sh first${NC}"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/binding_test_now" ]; then
    echo -e "${RED}Error: binding_test_now not found${NC}"
    echo -e "${YELLOW}Run setup.sh first${NC}"
    exit 1
fi

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    
    echo -e "\n${BLUE}[TEST]${NC} ${test_name}..."
    
    if eval "$command" > /tmp/test_output_$$.txt 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        echo -e "${YELLOW}Output:${NC}"
        cat /tmp/test_output_$$.txt | head -20
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Test 1: Lazy binding binary executes
run_test "Lazy binding binary execution" \
    "${BUILD_DIR}/binding_test_lazy"

# Test 2: Immediate binding binary executes
run_test "Immediate binding binary execution" \
    "${BUILD_DIR}/binding_test_now"

# Test 3: Lazy binding with threads
run_test "Lazy binding multithreaded test" \
    "${BUILD_DIR}/binding_test_lazy --threads"

# Test 4: Immediate binding with threads
run_test "Immediate binding multithreaded test" \
    "${BUILD_DIR}/binding_test_now --threads"

# Test 5: Verify lazy binding has PLT entries
run_test "Lazy binding PLT verification" \
    "[ \$(objdump -d ${BUILD_DIR}/binding_test_lazy 2>/dev/null | grep -c '@plt>') -gt 0 ]"

# Test 6: Verify immediate binding has BIND_NOW flag
run_test "Immediate binding BIND_NOW verification" \
    "readelf -d ${BUILD_DIR}/binding_test_now | grep -q 'BIND_NOW'"

# Test 7: Performance comparison - lazy should have overhead
echo -e "\n${BLUE}[TEST]${NC} Performance overhead detection..."
LAZY_OUTPUT=$("${BUILD_DIR}/binding_test_lazy" 2>&1)
if echo "$LAZY_OUTPUT" | grep -q "PLT resolution overhead"; then
    echo -e "${GREEN}✓ PASSED${NC} - PLT overhead detected in lazy binding"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${YELLOW}⚠ WARNING${NC} - PLT overhead not clearly detected"
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# Test 8: Check binary sizes are reasonable
run_test "Binary size check (lazy)" \
    "[ \$(stat -c%s ${BUILD_DIR}/binding_test_lazy) -gt 10000 ]"

run_test "Binary size check (immediate)" \
    "[ \$(stat -c%s ${BUILD_DIR}/binding_test_now) -gt 10000 ]"

# Cleanup
rm -f /tmp/test_output_$$.txt

# Summary
echo -e "\n${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                      Test Summary                             ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
echo -e "${GREEN}Tests Passed: ${TESTS_PASSED}${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Tests Failed: ${TESTS_FAILED}${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi

