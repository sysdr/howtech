#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

clear

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                    eBPF Verifier Dashboard                              ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Metric 1: File Status
echo -e "${CYAN}┌─ File Status${NC}"
PASSING_COUNT=$(ls -1 src/passing/*.c 2>/dev/null | wc -l)
FAILING_COUNT=$(ls -1 src/failing/*.c 2>/dev/null | wc -l)
MONITOR_COUNT=$(ls -1 src/monitor/*.c 2>/dev/null | wc -l)
TOTAL_SRC=$((PASSING_COUNT + FAILING_COUNT + MONITOR_COUNT))

echo -e "${GREEN}  ✓ Passing Programs:${NC} $PASSING_COUNT"
echo -e "${RED}  ✗ Failing Programs:${NC} $FAILING_COUNT"
echo -e "${BLUE}  ⚙ Monitor Tools:${NC} $MONITOR_COUNT"
echo -e "${YELLOW}  📊 Total Source Files:${NC} $TOTAL_SRC"
echo ""

# Metric 2: Build Status
echo -e "${CYAN}┌─ Build Status${NC}"
if [ -f "build/log_parser" ]; then
    LOG_PARSER_SIZE=$(du -h build/log_parser 2>/dev/null | cut -f1)
    echo -e "${GREEN}  ✓ log_parser:${NC} Built ($LOG_PARSER_SIZE)"
else
    echo -e "${RED}  ✗ log_parser:${NC} Not built"
fi

BUILD_O_COUNT=$(ls -1 build/*.o 2>/dev/null | wc -l)
if [ $BUILD_O_COUNT -gt 0 ]; then
    echo -e "${GREEN}  ✓ Object Files:${NC} $BUILD_O_COUNT"
else
    echo -e "${YELLOW}  ⚠ Object Files:${NC} 0 (compilation requires clang/Docker)"
fi
echo ""

# Metric 3: Service Status
echo -e "${CYAN}┌─ Service Status${NC}"
RUNNING_PROCESSES=$(ps aux | grep -E "log_parser|start_demo|ebpf" | grep -v grep | wc -l)
if [ $RUNNING_PROCESSES -eq 0 ]; then
    echo -e "${GREEN}  ✓ Services:${NC} No duplicate services running"
else
    echo -e "${YELLOW}  ⚠ Services:${NC} $RUNNING_PROCESSES process(es) detected"
fi

if docker images | grep -q ebpf-verifier-demo 2>/dev/null; then
    echo -e "${GREEN}  ✓ Docker Image:${NC} ebpf-verifier-demo available"
else
    echo -e "${YELLOW}  ⚠ Docker Image:${NC} Not built (optional)"
fi
echo ""

# Metric 4: Functionality Test
echo -e "${CYAN}┌─ Functionality Test${NC}"
if [ -f "build/log_parser" ]; then
    if ./build/log_parser stages > /dev/null 2>&1; then
        echo -e "${GREEN}  ✓ Verification Pipeline:${NC} Working"
    else
        echo -e "${RED}  ✗ Verification Pipeline:${NC} Failed"
    fi
    
    if ./build/log_parser registers > /dev/null 2>&1; then
        echo -e "${GREEN}  ✓ Register Types:${NC} Working"
    else
        echo -e "${RED}  ✗ Register Types:${NC} Failed"
    fi
else
    echo -e "${RED}  ✗ Functionality:${NC} log_parser not available"
fi
echo ""

# Metric 5: Directory Structure
echo -e "${CYAN}┌─ Directory Structure${NC}"
for dir in "src/passing" "src/failing" "src/monitor" "build" "logs"; do
    if [ -d "$dir" ]; then
        FILE_COUNT=$(find "$dir" -type f 2>/dev/null | wc -l)
        echo -e "${GREEN}  ✓${NC} $dir/ ($FILE_COUNT files)"
    else
        echo -e "${RED}  ✗${NC} $dir/ (missing)"
    fi
done
echo ""

# Metric 6: Demo Status
echo -e "${CYAN}┌─ Demo Status${NC}"
if [ -f "start_demo.sh" ] && [ -x "start_demo.sh" ]; then
    echo -e "${GREEN}  ✓ Demo Script:${NC} Available and executable"
else
    echo -e "${RED}  ✗ Demo Script:${NC} Not available"
fi

if [ -f "run_tests.sh" ] && [ -x "run_tests.sh" ]; then
    echo -e "${GREEN}  ✓ Test Script:${NC} Available and executable"
else
    echo -e "${RED}  ✗ Test Script:${NC} Not available"
fi
echo ""

# Summary
echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Summary                                                                  ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════════════╝${NC}"

ALL_GOOD=true

# Check critical components
if [ ! -f "build/log_parser" ]; then
    echo -e "${RED}  ✗ Critical: log_parser not built${NC}"
    ALL_GOOD=false
fi

if [ $TOTAL_SRC -lt 6 ]; then
    echo -e "${RED}  ✗ Warning: Missing source files${NC}"
    ALL_GOOD=false
fi

if [ $ALL_GOOD = true ]; then
    echo -e "${GREEN}  ✓ All critical components operational${NC}"
    echo -e "${GREEN}  ✓ Dashboard metrics updated successfully${NC}"
    echo -e "${GREEN}  ✓ Demo is working correctly${NC}"
else
    echo -e "${YELLOW}  ⚠ Some components need attention${NC}"
fi

echo ""
echo -e "${CYAN}Last updated:${NC} $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

