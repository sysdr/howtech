#!/bin/bash
# Don't use set -e, we want to continue checking all files

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Running eBPF LSM Demo Tests...${NC}\n"

PASSED=0
FAILED=0

test_file_exists() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓ $1 exists${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗ $1 missing${NC}"
        ((FAILED++))
        return 1
    fi
}

echo -e "${YELLOW}Checking generated files...${NC}"
test_file_exists "src/file_policy.bpf.c"
test_file_exists "src/loader.c"
test_file_exists "src/test_access.c"
test_file_exists "src/monitor.c"
test_file_exists "Makefile"
test_file_exists "Dockerfile"
test_file_exists "../cleanup.sh"

echo -e "\n${YELLOW}Checking build outputs...${NC}"
test_file_exists "build/file_policy.bpf.o"
test_file_exists "build/loader"
test_file_exists "build/test_access"
test_file_exists "build/monitor"

echo -e "\n${YELLOW}Checking startup scripts...${NC}"
test_file_exists "start_demo.sh"
test_file_exists "stop_demo.sh"
test_file_exists "dashboard.py"

echo -e "\n${BLUE}Test Summary:${NC}"
echo -e "  Passed: ${GREEN}${PASSED}${NC}"
echo -e "  Failed: ${RED}${FAILED}${NC}"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed!${NC}"
    exit 1
fi
