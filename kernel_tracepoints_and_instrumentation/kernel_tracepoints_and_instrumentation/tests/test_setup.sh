#!/bin/bash

set -eo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

TESTS_PASSED=0
TESTS_FAILED=0

test_check() {
    local name="$1"
    local condition="$2"
    
    if eval "$condition"; then
        echo -e "${GREEN}✓${NC} $name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}✗${NC} $name"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

echo "=== Running Setup Tests ==="
echo ""

# Test 1: Check directories exist
test_check "src/ directory exists" "[ -d src ]"
test_check "build/ directory exists" "[ -d build ]"
test_check "logs/ directory exists" "[ -d logs ]"

# Test 2: Check source files exist
test_check "src/target_program.c exists" "[ -f src/target_program.c ]"
test_check "src/perf_monitor.c exists" "[ -f src/perf_monitor.c ]"
test_check "src/trace_demo.sh exists" "[ -f src/trace_demo.sh ]"
test_check "src/trace_demo.sh is executable" "[ -x src/trace_demo.sh ]"

# Test 3: Check Makefile exists
test_check "Makefile exists" "[ -f Makefile ]"

# Test 4: Check executables exist and are executable
test_check "build/target_program exists" "[ -f build/target_program ]"
test_check "build/target_program is executable" "[ -x build/target_program ]"
test_check "build/perf_monitor exists" "[ -f build/perf_monitor ]"
test_check "build/perf_monitor is executable" "[ -x build/perf_monitor ]"

# Test 5: Check executables can run (basic test)
if [ -x build/target_program ]; then
    timeout 2 build/target_program > /dev/null 2>&1 &
    TARGET_PID=$!
    sleep 0.5
    if kill -0 $TARGET_PID 2>/dev/null; then
        kill $TARGET_PID 2>/dev/null || true
        test_check "build/target_program can execute" "true"
    else
        test_check "build/target_program can execute" "false"
    fi
    wait $TARGET_PID 2>/dev/null || true
fi

# Test 6: Check startup script exists
test_check "startup.sh exists" "[ -f startup.sh ]"
test_check "startup.sh is executable" "[ -x startup.sh ]"

# Test 7: Check stop script exists
test_check "stop.sh exists" "[ -f stop.sh ]"
test_check "stop.sh is executable" "[ -x stop.sh ]"

echo ""
echo "=== Test Results ==="
echo "Passed: $TESTS_PASSED"
echo "Failed: $TESTS_FAILED"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

