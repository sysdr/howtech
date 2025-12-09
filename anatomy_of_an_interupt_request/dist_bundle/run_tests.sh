#!/bin/bash

# Test script for IRQ demo components
set +e  # Don't exit on errors, we'll handle them

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PASSED=0
FAILED=0

test_check() {
    local test_name="$1"
    local command="$2"
    
    echo -ne "${CYAN}Testing: ${test_name}...${NC} "
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  Running IRQ Demo Tests${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo

# Test 1: Check if source files exist
test_check "Source files exist" "[ -f src/irq_monitor.c ] && [ -f src/irq_stress.c ] && [ -f src/irq_analyzer.sh ]"

# Test 2: Check if binaries are built
test_check "Binaries built" "[ -f irq_monitor ] && [ -f irq_stress ]"

# Test 3: Check if binaries are executable
test_check "Binaries executable" "[ -x irq_monitor ] && [ -x irq_stress ]"

# Test 4: Check if analyzer script is executable
test_check "Analyzer script executable" "[ -x src/irq_analyzer.sh ]"

# Test 5: Test irq_stress compilation check (quick version check)
test_check "irq_stress binary valid" "file irq_stress | grep -q ELF"

# Test 6: Test irq_monitor compilation check
test_check "irq_monitor binary valid" "file irq_monitor | grep -q ELF"

# Test 7: Check if /proc/interrupts is readable
test_check "/proc/interrupts readable" "[ -r /proc/interrupts ]"

# Test 8: Check if /proc/softirqs is readable
test_check "/proc/softirqs readable" "[ -r /proc/softirqs ]"

# Test 9: Test analyzer script runs (non-interactive)
test_check "Analyzer script runs" "timeout 5 ./src/irq_analyzer.sh > /dev/null 2>&1"

# Test 10: Check Makefile exists
test_check "Makefile exists" "[ -f Makefile ]"

# Test 11: Check Dockerfile exists
test_check "Dockerfile exists" "[ -f Dockerfile ]"

# Test 12: Check analysis directory exists
test_check "Analysis directory exists" "[ -d analysis ]"

# Test 13: Verify irq_stress can start and stop (quick test)
echo -ne "${CYAN}Testing: irq_stress start/stop...${NC} "
timeout 2 ./irq_stress > /dev/null 2>&1 &
STRESS_PID=$!
sleep 1
kill $STRESS_PID 2>/dev/null || true
wait $STRESS_PID 2>/dev/null || true
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ] || [ $EXIT_CODE -eq 143 ]; then  # 143 = SIGTERM
    echo -e "${GREEN}✓ PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}✗ FAILED${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 14: Check for duplicate services
echo -ne "${CYAN}Testing: No duplicate services running...${NC} "
IRQ_MONITOR_COUNT=$(pgrep -f "irq_monitor" | wc -l)
IRQ_STRESS_COUNT=$(pgrep -f "irq_stress" | wc -l)
if [ "$IRQ_MONITOR_COUNT" -le 1 ] && [ "$IRQ_STRESS_COUNT" -le 1 ]; then
    echo -e "${GREEN}✓ PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${YELLOW}⚠ WARNING: Multiple instances detected${NC}"
    echo -e "  irq_monitor: $IRQ_MONITOR_COUNT instances"
    echo -e "  irq_stress: $IRQ_STRESS_COUNT instances"
    FAILED=$((FAILED + 1))
fi

echo
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  Test Results${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
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

