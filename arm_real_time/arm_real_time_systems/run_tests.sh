#!/bin/bash
# Test script to validate all functionality

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0

test_check() {
    local name="$1"
    local command="$2"
    
    echo -n "Testing $name... "
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
        return 1
    fi
}

echo "=========================================="
echo "Running ARM Real-Time Demo Tests"
echo "=========================================="
echo ""

# Test 1: Check if all source files exist
test_check "Source files exist" "test -f src/latency_test.c && test -f src/irq_monitor.c && test -f src/cache_polluter.c"

# Test 2: Check if all binaries exist
test_check "Binary files exist" "test -f build/latency_test && test -f build/irq_monitor && test -f build/cache_polluter"

# Test 3: Check if binaries are executable
test_check "Binaries are executable" "test -x build/latency_test && test -x build/irq_monitor && test -x build/cache_polluter"

# Test 4: Check if Makefile exists
test_check "Makefile exists" "test -f Makefile"

# Test 5: Check if Dockerfile exists
test_check "Dockerfile exists" "test -f Dockerfile"

# Test 6: Test latency_test binary (quick test)
echo -n "Testing latency_test binary (5 second test)... "
timeout 5 "$SCRIPT_DIR/build/latency_test" 0 > /dev/null 2>&1
if [ $? -eq 124 ] || [ $? -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test 7: Test irq_monitor binary (quick test)
echo -n "Testing irq_monitor binary (2 second test)... "
timeout 2 "$SCRIPT_DIR/build/irq_monitor" > /dev/null 2>&1
if [ $? -eq 124 ] || [ $? -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test 8: Test cache_polluter binary (quick test)
echo -n "Testing cache_polluter binary (1 second test)... "
timeout 1 "$SCRIPT_DIR/build/cache_polluter" 1 > /dev/null 2>&1
if [ $? -eq 124 ] || [ $? -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test 9: Check if results directory exists and is writable
test_check "Results directory writable" "test -w results"

# Test 10: Check if startup scripts exist
test_check "Startup scripts exist" "test -f start_latency_test.sh && test -f start_irq_monitor.sh && test -f start_cache_polluter.sh && test -f start_all.sh"

# Test 11: Check if startup scripts are executable
test_check "Startup scripts executable" "test -x start_latency_test.sh && test -x start_irq_monitor.sh && test -x start_cache_polluter.sh && test -x start_all.sh"

# Test 12: Verify latency_stats.txt was created
if [ -f "results/latency_stats.txt" ]; then
    test_check "Latency stats file readable" "test -r results/latency_stats.txt"
    if [ -s "results/latency_stats.txt" ]; then
        echo -e "${GREEN}Latency stats file contains data${NC}"
        ((PASSED++))
    else
        echo -e "${YELLOW}Latency stats file is empty${NC}"
    fi
else
    echo -e "${YELLOW}Latency stats file not found (may need to run latency test)${NC}"
fi

echo ""
echo "=========================================="
echo "Test Results: $PASSED passed, $FAILED failed"
echo "=========================================="

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi

