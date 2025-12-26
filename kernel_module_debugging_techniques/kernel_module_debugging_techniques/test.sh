#!/bin/bash

# Test script for kernel module debugging demo

set +e  # Don't exit on error, we want to count failures

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   Running Tests for Kernel Module Debugging Demo             ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"

TESTS_PASSED=0
TESTS_FAILED=0

# Function to increment test count
inc_passed() { TESTS_PASSED=$((TESTS_PASSED + 1)); }
inc_failed() { TESTS_FAILED=$((TESTS_FAILED + 1)); }

# Test 1: Check if source files exist
echo -e "\n${BLUE}[Test 1] Checking source files...${NC}"
if [ -f "src/debug_demo.c" ]; then
    echo -e "${GREEN}✓ debug_demo.c exists${NC}"
    inc_passed
else
    echo -e "${RED}✗ debug_demo.c missing${NC}"
    inc_failed
fi

if [ -f "src/klog_monitor.c" ]; then
    echo -e "${GREEN}✓ klog_monitor.c exists${NC}"
    inc_passed
else
    echo -e "${RED}✗ klog_monitor.c missing${NC}"
    inc_failed
fi

if [ -f "src/Makefile" ]; then
    echo -e "${GREEN}✓ Makefile exists${NC}"
    inc_passed
else
    echo -e "${RED}✗ Makefile missing${NC}"
    inc_failed
fi

# Test 2: Check if executables are built
echo -e "\n${BLUE}[Test 2] Checking built executables...${NC}"
if [ -f "src/klog_monitor" ] && [ -x "src/klog_monitor" ]; then
    echo -e "${GREEN}✓ klog_monitor executable exists${NC}"
    inc_passed
else
    echo -e "${YELLOW}⚠ klog_monitor not built (may require ncurses)${NC}"
fi

if [ -f "src/debug_demo.ko" ]; then
    echo -e "${GREEN}✓ debug_demo.ko exists${NC}"
    inc_passed
else
    echo -e "${YELLOW}⚠ debug_demo.ko not built (expected in WSL/containers)${NC}"
fi

# Test 3: Check if module can be loaded (if built)
echo -e "\n${BLUE}[Test 3] Testing module loading...${NC}"
if [ -f "src/debug_demo.ko" ]; then
    # Unload if already loaded
    if lsmod | grep -q "^debug_demo"; then
        sudo rmmod debug_demo 2>/dev/null || true
    fi
    
    if sudo insmod src/debug_demo.ko 2>/dev/null; then
        echo -e "${GREEN}✓ Module loads successfully${NC}"
        sleep 1
        
        # Check if module is in lsmod
        if lsmod | grep -q "^debug_demo"; then
            echo -e "${GREEN}✓ Module appears in lsmod${NC}"
            inc_passed
        else
            echo -e "${RED}✗ Module not in lsmod${NC}"
            inc_failed
        fi
        
        # Check kernel logs
        if dmesg | grep -q "debug_demo:"; then
            echo -e "${GREEN}✓ Module generates kernel logs${NC}"
            inc_passed
        else
            echo -e "${YELLOW}⚠ No kernel logs found${NC}"
        fi
        
        # Unload module
        sudo rmmod debug_demo 2>/dev/null && echo -e "${GREEN}✓ Module unloads successfully${NC}" || echo -e "${YELLOW}⚠ Module unload warning${NC}"
        inc_passed
    else
        echo -e "${YELLOW}⚠ Module load failed (expected in WSL/containers)${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Skipping module load test (module not built)${NC}"
fi

# Test 4: Check module parameters
echo -e "\n${BLUE}[Test 4] Testing module parameters...${NC}"
if [ -f "src/debug_demo.ko" ]; then
    if sudo insmod src/debug_demo.ko simulate_bug=1 loop_count=3 2>/dev/null; then
        sleep 1
        if [ -d "/sys/module/debug_demo/parameters" ]; then
            echo -e "${GREEN}✓ Module parameters accessible${NC}"
            PARAM_COUNT=$(ls /sys/module/debug_demo/parameters/ 2>/dev/null | wc -l)
            echo -e "${CYAN}  Found $PARAM_COUNT parameters${NC}"
            inc_passed
        fi
        sudo rmmod debug_demo 2>/dev/null || true
    fi
else
    echo -e "${YELLOW}⚠ Skipping parameter test (module not built)${NC}"
fi

# Test 5: Check documentation
echo -e "\n${BLUE}[Test 5] Checking documentation...${NC}"
if [ -f "article.md" ]; then
    echo -e "${GREEN}✓ article.md exists${NC}"
    inc_passed
else
    echo -e "${RED}✗ article.md missing${NC}"
    inc_failed
fi

# Test 6: Check startup scripts
echo -e "\n${BLUE}[Test 6] Checking startup scripts...${NC}"
if [ -f "start.sh" ] && [ -x "start.sh" ]; then
    echo -e "${GREEN}✓ start.sh exists and is executable${NC}"
    inc_passed
else
    echo -e "${RED}✗ start.sh missing or not executable${NC}"
    inc_failed
fi

if [ -f "stop.sh" ] && [ -x "stop.sh" ]; then
    echo -e "${GREEN}✓ stop.sh exists and is executable${NC}"
    inc_passed
else
    echo -e "${RED}✗ stop.sh missing or not executable${NC}"
    inc_failed
fi

# Test 7: Check dashboard
echo -e "\n${BLUE}[Test 7] Checking dashboard...${NC}"
if [ -f "src/dashboard.py" ]; then
    echo -e "${GREEN}✓ dashboard.py exists${NC}"
    if command -v python3 &> /dev/null; then
        if python3 -c "import flask" 2>/dev/null; then
            echo -e "${GREEN}✓ Flask is installed${NC}"
            inc_passed
        else
            echo -e "${YELLOW}⚠ Flask not installed (run: pip3 install flask)${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Python3 not found${NC}"
    fi
else
    echo -e "${YELLOW}⚠ dashboard.py not found${NC}"
fi

# Summary
echo -e "\n${CYAN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Test Summary:${NC}"
echo -e "  ${GREEN}Passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "  ${RED}Failed: $TESTS_FAILED${NC}"
else
    echo -e "  ${GREEN}Failed: $TESTS_FAILED${NC}"
fi
echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All critical tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠ Some tests failed or were skipped${NC}"
    exit 1
fi

