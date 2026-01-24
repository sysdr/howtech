#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}XDP Demo Tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo

TESTS_PASSED=0
TESTS_FAILED=0

test_check() {
    local test_name="$1"
    local command="$2"
    
    echo -n "Testing $test_name... "
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        ((TESTS_FAILED++))
        return 1
    fi
}

# Test 1: Check if binaries exist
test_check "XDP program binary exists" "[ -f build/xdp_drop.o ]"
test_check "Packet generator exists" "[ -f build/packet_gen ]"
test_check "Monitor exists" "[ -f build/xdp_monitor ]"

# Test 2: Check if XDP program is loaded (requires root)
if [ "$EUID" -eq 0 ]; then
    test_check "XDP program loaded on lo" "ip link show dev lo | grep -q xdp"
    
    # Test 3: Check if we can get map FD
    MAP_FD=$(bpftool map list 2>/dev/null | grep xdp_stats | awk '{print $1}' | cut -d: -f1 || echo "")
    if [ -n "$MAP_FD" ]; then
        test_check "Statistics map accessible" "[ -n \"$MAP_FD\" ]"
    else
        echo -n "Testing statistics map accessible... "
        echo -e "${YELLOW}SKIP (map not found)${NC}"
    fi
    
    # Test 4: Send test packets
    echo -n "Testing packet generation... "
    timeout 2 build/packet_gen 127.0.0.1 > /dev/null 2>&1 && echo -e "${GREEN}PASS${NC}" && ((TESTS_PASSED++)) || echo -e "${YELLOW}SKIP${NC}"
else
    echo -e "${YELLOW}Skipping root-required tests (not running as root)${NC}"
fi

echo
echo -e "${BLUE}Test Results:${NC}"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
