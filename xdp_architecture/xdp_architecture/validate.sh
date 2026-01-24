#!/bin/bash
# Validation script for XDP demo setup
# Checks all files, scripts, and dependencies

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}XDP Demo Validation${NC}"
echo -e "${BLUE}========================================${NC}"
echo

ERRORS=0
WARNINGS=0

check_file() {
    local file="$1"
    local desc="${2:-$file}"
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $desc exists"
        return 0
    else
        echo -e "${RED}✗${NC} $desc missing"
        ((ERRORS++))
        return 1
    fi
}

check_binary() {
    local binary="$1"
    if command -v "$binary" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $binary installed"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} $binary not found (may need: sudo ./install_deps.sh)"
        ((WARNINGS++))
        return 1
    fi
}

echo -e "${BLUE}Checking source files...${NC}"
check_file "src/xdp_drop.c" "XDP program source"
check_file "src/packet_gen.c" "Packet generator source"
check_file "src/xdp_monitor.c" "Monitor source"
check_file "Makefile" "Makefile"
check_file "Dockerfile" "Dockerfile"
echo

echo -e "${BLUE}Checking scripts...${NC}"
check_file "setup.sh" "Setup script"
check_file "startup.sh" "Startup script"
check_file "cleanup.sh" "Cleanup script"
check_file "run_tests.sh" "Test script"
check_file "run_dashboard.sh" "Dashboard script"
check_file "run_demo.sh" "Demo script"
check_file "install_deps.sh" "Dependencies installer"
echo

echo -e "${BLUE}Checking build dependencies...${NC}"
check_binary "clang"
check_binary "make"
check_binary "bpftool"
echo

echo -e "${BLUE}Checking build outputs...${NC}"
if [ -f "build/xdp_drop.o" ]; then
    echo -e "${GREEN}✓${NC} XDP program built"
else
    echo -e "${YELLOW}⚠${NC} XDP program not built (run: make)"
    ((WARNINGS++))
fi

if [ -f "build/packet_gen" ]; then
    echo -e "${GREEN}✓${NC} Packet generator built"
else
    echo -e "${YELLOW}⚠${NC} Packet generator not built (run: make)"
    ((WARNINGS++))
fi

if [ -f "build/xdp_monitor" ]; then
    echo -e "${GREEN}✓${NC} Monitor built"
else
    echo -e "${YELLOW}⚠${NC} Monitor not built (run: make)"
    ((WARNINGS++))
fi
echo

echo -e "${BLUE}Checking for running services...${NC}"
if pgrep -f "packet_gen" >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠${NC} Packet generator process found"
    ((WARNINGS++))
else
    echo -e "${GREEN}✓${NC} No packet generator running"
fi

if pgrep -f "xdp_monitor" >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠${NC} Monitor process found"
    ((WARNINGS++))
else
    echo -e "${GREEN}✓${NC} No monitor running"
fi

if ip link show dev lo 2>/dev/null | grep -q xdp; then
    echo -e "${GREEN}✓${NC} XDP program loaded on lo"
else
    echo -e "${YELLOW}⚠${NC} XDP program not loaded (run: sudo ./startup.sh)"
    ((WARNINGS++))
fi
echo

echo -e "${BLUE}Summary:${NC}"
if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}All checks passed!${NC}"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}$WARNINGS warning(s) - setup may need dependencies or build${NC}"
    exit 0
else
    echo -e "${RED}$ERRORS error(s), $WARNINGS warning(s)${NC}"
    exit 1
fi

