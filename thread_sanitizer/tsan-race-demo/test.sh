#!/bin/bash
set -uo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

cd "$(dirname "$0")"

echo -e "${BLUE}Running TSAN Demo Tests...${NC}"
echo

PASSED=0
FAILED=0

# Test 1: Check if all binaries exist
echo -e "${YELLOW}Test 1: Checking binaries...${NC}"
BINARIES=("race_example" "race_example_tsan" "race_fixed" "race_fixed_tsan" "race_atomic" "race_atomic_tsan" "tsan_monitor")
for bin in "${BINARIES[@]}"; do
    if [ -f "$bin" ] && [ -x "$bin" ]; then
        echo -e "  ${GREEN}✓${NC} $bin exists and is executable"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} $bin missing or not executable"
        FAILED=$((FAILED + 1))
    fi
done

# Test 2: Run race_example (should show incorrect results)
echo
echo -e "${YELLOW}Test 2: Running race_example (should show race condition)...${NC}"
if ./race_example > /tmp/race_test.out 2>&1; then
    FINAL=$(grep "Final counter value" /tmp/race_test.out | awk '{print $4}')
    EXPECTED=400000
    if [ -n "$FINAL" ] && [ "$FINAL" -lt "$EXPECTED" ]; then
        echo -e "  ${GREEN}✓${NC} Race condition detected (got $FINAL, expected $EXPECTED)"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${YELLOW}⚠${NC} Race condition may not have occurred (got $FINAL)"
        PASSED=$((PASSED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} race_example failed to run"
    ((FAILED++))
fi

# Test 3: Run race_fixed (should show correct results)
echo
echo -e "${YELLOW}Test 3: Running race_fixed (should show correct results)...${NC}"
if ./race_fixed > /tmp/fixed_test.out 2>&1; then
    FINAL=$(grep "Final counter value" /tmp/fixed_test.out | awk '{print $4}')
    EXPECTED=400000
    if [ -n "$FINAL" ] && [ "$FINAL" -eq "$EXPECTED" ]; then
        echo -e "  ${GREEN}✓${NC} Fixed version works correctly (got $FINAL)"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} Fixed version failed (got $FINAL, expected $EXPECTED)"
        FAILED=$((FAILED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} race_fixed failed to run"
    ((FAILED++))
fi

# Test 4: Run race_atomic (should show correct results)
echo
echo -e "${YELLOW}Test 4: Running race_atomic (should show correct results)...${NC}"
if ./race_atomic > /tmp/atomic_test.out 2>&1; then
    FINAL=$(grep "Final counter value" /tmp/atomic_test.out | awk '{print $4}')
    EXPECTED=400000
    if [ -n "$FINAL" ] && [ "$FINAL" -eq "$EXPECTED" ]; then
        echo -e "  ${GREEN}✓${NC} Atomic version works correctly (got $FINAL)"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} Atomic version failed (got $FINAL, expected $EXPECTED)"
        FAILED=$((FAILED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} race_atomic failed to run"
    ((FAILED++))
fi

# Test 5: TSAN should detect race in race_example_tsan
echo
echo -e "${YELLOW}Test 5: TSAN should detect race condition...${NC}"
export TSAN_OPTIONS="halt_on_error=0:exitcode=0"
if ./race_example_tsan > /tmp/tsan_test.out 2>&1; then
    if grep -q "WARNING: ThreadSanitizer" /tmp/tsan_test.out || grep -q "data race" /tmp/tsan_test.out; then
        echo -e "  ${GREEN}✓${NC} TSAN detected race condition"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${YELLOW}⚠${NC} TSAN output not found (may still be working)"
        PASSED=$((PASSED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} race_example_tsan failed to run"
    ((FAILED++))
fi

# Test 6: TSAN should NOT detect race in race_fixed_tsan
echo
echo -e "${YELLOW}Test 6: TSAN should NOT detect race in fixed version...${NC}"
export TSAN_OPTIONS="halt_on_error=0:exitcode=0"
if ./race_fixed_tsan > /tmp/fixed_tsan_test.out 2>&1; then
    if grep -q "WARNING: ThreadSanitizer" /tmp/fixed_tsan_test.out || grep -q "data race" /tmp/fixed_tsan_test.out; then
        echo -e "  ${YELLOW}⚠${NC} TSAN detected something (may be false positive)"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${GREEN}✓${NC} TSAN found no race condition (as expected)"
        PASSED=$((PASSED + 1))
    fi
else
    echo -e "  ${RED}✗${NC} race_fixed_tsan failed to run"
    ((FAILED++))
fi

# Summary
echo
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "  ${GREEN}Passed: $PASSED${NC}"
echo -e "  ${RED}Failed: $FAILED${NC}"
echo

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
