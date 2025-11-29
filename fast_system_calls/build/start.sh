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
echo -e "${BOLD}${CYAN}   Starting Fast Syscalls Demo        ${NC}"
echo -e "${BOLD}${CYAN}======================================${NC}\n"

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${RED}Error: Build directory not found!${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

cd "${BUILD_DIR}"

# Check if binaries exist
if [ ! -f "getpid_bench" ] || [ ! -f "vdso_inspector" ] || [ ! -f "monitor" ]; then
    echo -e "${RED}Error: Binaries not found!${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check for duplicate processes
echo -e "${YELLOW}Checking for duplicate services...${NC}"
if pgrep -f "getpid_bench|vdso_inspector|monitor" > /dev/null 2>&1; then
    RUNNING_COUNT=$(pgrep -f "getpid_bench|vdso_inspector|monitor" | wc -l)
    echo -e "${YELLOW}Warning: Found $RUNNING_COUNT running process(es)${NC}"
    ps aux | grep -E "(getpid_bench|vdso_inspector|monitor)" | grep -v grep || true
    echo -e "${YELLOW}Killing existing processes...${NC}"
    pkill -f "getpid_bench|vdso_inspector|monitor" || true
    sleep 1
else
    echo -e "${GREEN}✓${NC} No duplicate services running"
fi

echo -e "\n${GREEN}✓${NC} All checks passed. Ready to run demos."
echo -e "\n${CYAN}Available programs:${NC}"
echo -e "  • ${GREEN}./vdso_inspector${NC} - Inspect vDSO availability"
echo -e "  • ${GREEN}./getpid_bench${NC} - Benchmark getpid() performance"
echo -e "  • ${GREEN}./monitor${NC} - Real-time performance monitor"
echo -e "\n${YELLOW}Run './test.sh' to run all tests${NC}"

