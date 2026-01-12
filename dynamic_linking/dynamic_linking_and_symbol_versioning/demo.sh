#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}

print_step() {
    echo -e "${GREEN}➜${NC} $1"
}

# Check if build directory exists and has binaries
if [ ! -d "build" ] || [ ! -f "build/libmylib.so.3.0.0" ]; then
    echo -e "${RED}Error: Build directory or binaries not found. Run ./setup.sh first.${NC}"
    exit 1
fi

print_header "Symbol Versioning Demo"

echo -e "${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "1. Inspecting Library Version Definitions"
echo ""
readelf -V build/libmylib.so.3 | grep -A 20 "Version definition section" || true

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "2. Viewing Versioned Symbols"
echo ""
objdump --dynamic-syms build/libmylib.so.3 | grep "MYLIB\|api_" || true

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "3. Checking v1.0 App Requirements"
echo ""
readelf -V build/app_v1 | grep -A 10 "Version needs section" || true

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "4. Running v1.0 Application"
echo ""
LD_LIBRARY_PATH=./build ./build/app_v1

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "5. Running v2.0 Application"
echo ""
LD_LIBRARY_PATH=./build ./build/app_v2

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

print_step "6. Symbol Version Information"
echo ""
readelf -V build/libmylib.so.3 2>&1 | head -50

echo -e "\n${CYAN}Demo complete! Run './build/monitor' for real-time monitoring${NC}\n"

