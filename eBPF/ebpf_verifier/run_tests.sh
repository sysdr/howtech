#!/bin/bash

set +e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  eBPF Verifier - Running Tests                            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

TESTS_PASSED=0
TESTS_FAILED=0

increment_passed() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

increment_failed() {
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

# Test 1: Check if all source files exist
echo -e "${YELLOW}[Test 1] Checking source files...${NC}"
EXPECTED_FILES=(
    "src/passing/simple_pass.c"
    "src/passing/bounded_loop.c"
    "src/failing/unbounded_pointer.c"
    "src/failing/no_bounds_check.c"
    "src/monitor/log_parser.c"
    "src/monitor/bpf_loader.c"
)

for file in "${EXPECTED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo -e "  ${GREEN}✓${NC} $file"
        increment_passed
    else
        echo -e "  ${RED}✗${NC} $file (missing)"
        increment_failed
    fi
done

# Test 2: Check if build files exist
echo ""
echo -e "${YELLOW}[Test 2] Checking build files...${NC}"
if [ -f "Makefile" ]; then
    echo -e "  ${GREEN}✓${NC} Makefile"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} Makefile (missing)"
    ((TESTS_FAILED++))
fi

if [ -f "Dockerfile" ]; then
    echo -e "  ${GREEN}✓${NC} Dockerfile"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} Dockerfile (missing)"
    ((TESTS_FAILED++))
fi

# Test 3: Check if log_parser exists and works
echo ""
echo -e "${YELLOW}[Test 3] Testing log_parser...${NC}"
if [ -f "build/log_parser" ]; then
    echo -e "  ${GREEN}✓${NC} log_parser exists"
    ((TESTS_PASSED++))
    
    if ./build/log_parser stages > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} log_parser stages command works"
        increment_passed
    else
        echo -e "  ${RED}✗${NC} log_parser stages command failed"
        increment_failed
    fi
    
    if ./build/log_parser registers > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} log_parser registers command works"
        increment_passed
    else
        echo -e "  ${RED}✗${NC} log_parser registers command failed"
        increment_failed
    fi
else
    echo -e "  ${YELLOW}⚠${NC} log_parser not built (attempting to build...)"
    if gcc -O2 -Wall -Wextra -o build/log_parser src/monitor/log_parser.c 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} log_parser built successfully"
        increment_passed
    else
        echo -e "  ${RED}✗${NC} Failed to build log_parser"
        increment_failed
    fi
fi

# Test 4: Check directory structure
echo ""
echo -e "${YELLOW}[Test 4] Checking directory structure...${NC}"
for dir in "src/passing" "src/failing" "src/monitor" "build" "logs"; do
    if [ -d "$dir" ]; then
        echo -e "  ${GREEN}✓${NC} $dir/"
        increment_passed
    else
        echo -e "  ${RED}✗${NC} $dir/ (missing)"
        increment_failed
    fi
done

# Summary
echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Test Summary                                               ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo -e "  ${GREEN}Passed:${NC} $TESTS_PASSED"
echo -e "  ${RED}Failed:${NC} $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

