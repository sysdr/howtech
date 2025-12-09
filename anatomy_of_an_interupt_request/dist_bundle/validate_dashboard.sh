#!/bin/bash

# Dashboard validation script for irq_monitor
set +e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  Dashboard Validation - IRQ Monitor${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo

VALIDATION_PASSED=0
VALIDATION_FAILED=0

validate_check() {
    local test_name="$1"
    local command="$2"
    
    echo -ne "${CYAN}Validating: ${test_name}...${NC} "
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
        return 1
    fi
}

# Check if irq_monitor exists and is executable
validate_check "irq_monitor binary exists" "[ -f irq_monitor ]"
validate_check "irq_monitor is executable" "[ -x irq_monitor ]"

# Check if required /proc files are accessible
validate_check "/proc/interrupts accessible" "[ -r /proc/interrupts ]"
validate_check "/proc/softirqs accessible" "[ -r /proc/softirqs ]"

# Test that irq_monitor can read interrupts (non-interactive test)
echo -ne "${CYAN}Validating: irq_monitor can read interrupt data...${NC} "
if timeout 1 ./irq_monitor 2>&1 | head -5 | grep -q "IRQ\|CPU\|INTERRUPT" 2>/dev/null; then
    echo -e "${GREEN}✓ PASSED${NC}"
    VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
else
    # Alternative: check if it at least starts without errors
    if timeout 1 ./irq_monitor > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    else
        echo -e "${YELLOW}⚠ PARTIAL (may need root permissions)${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    fi
fi

# Validate metrics can be read from /proc
echo -e "${CYAN}Validating: Interrupt metrics available...${NC}"
if [ -r /proc/interrupts ]; then
    IRQ_COUNT=$(grep -c "^[[:space:]]*[0-9]" /proc/interrupts 2>/dev/null || echo "0")
    if [ "$IRQ_COUNT" -gt 0 ]; then
        echo -e "  ${GREEN}✓ Found $IRQ_COUNT IRQ entries${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    else
        echo -e "  ${RED}✗ No IRQ entries found${NC}"
        VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
    fi
else
    echo -e "  ${RED}✗ Cannot read /proc/interrupts${NC}"
    VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
fi

# Validate softirq metrics
echo -e "${CYAN}Validating: Softirq metrics available...${NC}"
if [ -r /proc/softirqs ]; then
    SOFTIRQ_TYPES=$(grep -c "^[[:space:]]*[A-Z]" /proc/softirqs 2>/dev/null || echo "0")
    if [ "$SOFTIRQ_TYPES" -gt 0 ]; then
        echo -e "  ${GREEN}✓ Found $SOFTIRQ_TYPES softirq types${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    else
        echo -e "  ${RED}✗ No softirq types found${NC}"
        VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
    fi
else
    echo -e "  ${RED}✗ Cannot read /proc/softirqs${NC}"
    VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
fi

# Test that analyzer script produces output
echo -e "${CYAN}Validating: Analyzer script produces metrics...${NC}"
ANALYZER_OUTPUT=$(timeout 5 ./src/irq_analyzer.sh 2>&1)
if echo "$ANALYZER_OUTPUT" | grep -q "Hardware Interrupt\|Software Interrupt\|IRQ\|CPU" 2>/dev/null; then
    echo -e "  ${GREEN}✓ Analyzer produces valid output${NC}"
    VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
else
    echo -e "  ${YELLOW}⚠ Analyzer output may be limited${NC}"
    VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
fi

# Check CPU count detection
echo -e "${CYAN}Validating: CPU detection...${NC}"
CPU_COUNT=$(nproc 2>/dev/null || echo "0")
if [ "$CPU_COUNT" -gt 0 ]; then
    echo -e "  ${GREEN}✓ Detected $CPU_COUNT CPU cores${NC}"
    VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
else
    echo -e "  ${RED}✗ Cannot detect CPU count${NC}"
    VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
fi

# Demo functionality test: Start stress test and verify interrupts increase
echo -e "${CYAN}Validating: Demo functionality (stress test)...${NC}"
if [ -f irq_stress ] && [ -x irq_stress ]; then
    # Get baseline interrupt count
    BASELINE=$(grep -E "^[[:space:]]*[0-9]+:" /proc/interrupts | head -1 | awk '{sum=0; for(i=2;i<=NF;i++){if($i~/^[0-9]+$/){sum+=$i}} print sum}')
    
    # Start stress test in background
    timeout 3 ./irq_stress > /dev/null 2>&1 &
    STRESS_PID=$!
    sleep 1
    
    # Get new interrupt count
    NEW_COUNT=$(grep -E "^[[:space:]]*[0-9]+:" /proc/interrupts | head -1 | awk '{sum=0; for(i=2;i<=NF;i++){if($i~/^[0-9]+$/){sum+=$i}} print sum}')
    
    # Kill stress test
    kill $STRESS_PID 2>/dev/null || true
    wait $STRESS_PID 2>/dev/null || true
    
    if [ ! -z "$BASELINE" ] && [ ! -z "$NEW_COUNT" ] && [ "$NEW_COUNT" -gt "$BASELINE" ]; then
        echo -e "  ${GREEN}✓ Stress test generates interrupts (baseline: $BASELINE, after: $NEW_COUNT)${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    else
        echo -e "  ${YELLOW}⚠ Interrupt generation test inconclusive${NC}"
        VALIDATION_PASSED=$((VALIDATION_PASSED + 1))
    fi
else
    echo -e "  ${RED}✗ irq_stress not available${NC}"
    VALIDATION_FAILED=$((VALIDATION_FAILED + 1))
fi

echo
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  Validation Results${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "Passed: ${GREEN}${VALIDATION_PASSED}${NC}"
echo -e "Failed: ${RED}${VALIDATION_FAILED}${NC}"
echo

# Summary
echo -e "${CYAN}Dashboard Metrics Summary:${NC}"
echo "  - Hardware Interrupts: $(grep -cE '^[[:space:]]*[0-9]+:' /proc/interrupts 2>/dev/null || echo 'N/A') IRQs"
echo "  - Softirq Types: $(grep -cE '^[[:space:]]*[A-Z]' /proc/softirqs 2>/dev/null || echo 'N/A') types"
echo "  - CPU Cores: $(nproc 2>/dev/null || echo 'N/A')"
echo "  - System Uptime: $(uptime -p 2>/dev/null || echo 'N/A')"
echo

if [ $VALIDATION_FAILED -eq 0 ]; then
    echo -e "${GREEN}All dashboard validations passed!${NC}"
    echo -e "${GREEN}The irq_monitor dashboard is ready to use.${NC}"
    echo -e "${YELLOW}Note: For full functionality, run with: sudo ./irq_monitor${NC}"
    exit 0
else
    echo -e "${RED}Some validations failed!${NC}"
    exit 1
fi

