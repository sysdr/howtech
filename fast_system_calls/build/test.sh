#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# If script is in build folder, use current dir; otherwise use build subdirectory
if [ "$(basename "${SCRIPT_DIR}")" = "build" ]; then
    BUILD_DIR="${SCRIPT_DIR}"
else
    BUILD_DIR="${SCRIPT_DIR}/build"
fi

echo -e "${BOLD}${CYAN}======================================${NC}"
echo -e "${BOLD}${CYAN}   Running Tests                      ${NC}"
echo -e "${BOLD}${CYAN}======================================${NC}\n"

cd "${BUILD_DIR}"

# Test 1: Check if binaries exist
echo -e "${YELLOW}Test 1: Checking if binaries exist...${NC}"
BINARIES=("getpid_bench" "vdso_inspector" "monitor")
for bin in "${BINARIES[@]}"; do
    if [ -f "${bin}" ] && [ -x "${bin}" ]; then
        echo -e "${GREEN}✓${NC} ${bin} exists and is executable"
    else
        echo -e "${RED}✗${NC} ${bin} missing or not executable"
        exit 1
    fi
done

# Test 2: Run vdso_inspector
echo -e "\n${YELLOW}Test 2: Running vdso_inspector...${NC}"
if ./vdso_inspector > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} vdso_inspector runs successfully"
else
    echo -e "${RED}✗${NC} vdso_inspector failed"
    exit 1
fi

# Test 3: Run getpid_bench (quick test)
echo -e "\n${YELLOW}Test 3: Running getpid_bench (quick test)...${NC}"
if timeout 5 ./getpid_bench 2>&1 | grep -q "Results:"; then
    echo -e "${GREEN}✓${NC} getpid_bench runs successfully"
else
    echo -e "${RED}✗${NC} getpid_bench failed or timed out"
    exit 1
fi

# Test 4: Run test_getpid
echo -e "\n${YELLOW}Test 4: Running test_getpid...${NC}"
if ./test_getpid > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} test_getpid runs successfully"
else
    echo -e "${RED}✗${NC} test_getpid failed"
    exit 1
fi

# Test 5: Check if binaries produce expected output
echo -e "\n${YELLOW}Test 5: Verifying output format...${NC}"
if ./vdso_inspector 2>&1 | grep -q "vDSO"; then
    echo -e "${GREEN}✓${NC} vdso_inspector produces expected output"
else
    echo -e "${RED}✗${NC} vdso_inspector output format incorrect"
    exit 1
fi

echo -e "\n${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${GREEN}All tests passed!${NC}"
echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"

