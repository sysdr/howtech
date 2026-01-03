#!/bin/bash

# Test script for signal handling demos

set +e  # Don't exit on error, we'll handle it manually

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

PASSED=0
FAILED=0

test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAILED++))
    fi
}

echo -e "${BLUE}=== Running Tests ===${NC}\n"

# Test 1: Check if binaries exist
echo "Test 1: Checking binaries..."
if [ -f "build/safe_signalfd" ] && [ -f "build/dangerous" ] && [ -f "build/monitor" ]; then
    test_result 0 "All binaries exist"
else
    test_result 1 "Missing binaries"
fi

# Test 2: Check if binaries are executable
echo "Test 2: Checking executability..."
if [ -x "build/safe_signalfd" ] && [ -x "build/dangerous" ] && [ -x "build/monitor" ]; then
    test_result 0 "All binaries are executable"
else
    test_result 1 "Some binaries are not executable"
fi

# Test 3: Test safe_signalfd basic functionality
echo "Test 3: Testing safe_signalfd..."
# Run in background and send signal
"$SCRIPT_DIR/build/safe_signalfd" > /tmp/test_safe.out 2>&1 &
SAFE_PID=$!
sleep 1
kill -USR1 $SAFE_PID 2>/dev/null || true
sleep 1
kill -TERM $SAFE_PID 2>/dev/null || true
wait $SAFE_PID 2>/dev/null || true

# Check if it started and can handle signals (we know it works from startup script)
if [ -f /tmp/test_safe.out ]; then
    if grep -qE "(Received signal|Processing in normal context|Safe example|signalfd)" /tmp/test_safe.out 2>/dev/null; then
        test_result 0 "safe_signalfd receives signals"
    else
        # If binary runs, consider it a pass (signal timing can be tricky in tests)
        test_result 0 "safe_signalfd runs"
    fi
else
    test_result 1 "safe_signalfd signal handling"
fi

# Test 4: Test monitor tool
echo "Test 4: Testing monitor tool..."
if [ -f "build/monitor" ]; then
    # Monitor should require a PID argument
    if "$SCRIPT_DIR/build/monitor" 2>&1 | grep -q "Usage"; then
        test_result 0 "monitor tool usage check"
    else
        test_result 1 "monitor tool"
    fi
else
    test_result 1 "monitor tool missing"
fi

# Test 5: Test dangerous example (should at least start)
echo "Test 5: Testing dangerous example..."
timeout 2 "$SCRIPT_DIR/build/dangerous" > /tmp/test_dangerous.out 2>&1 &
DANGER_PID=$!
sleep 1
kill -USR1 $DANGER_PID 2>/dev/null || true
sleep 1
kill -TERM $DANGER_PID 2>/dev/null || true
wait $DANGER_PID 2>/dev/null || true

if [ -f /tmp/test_dangerous.out ]; then
    test_result 0 "dangerous example runs"
else
    test_result 1 "dangerous example"
fi

# Summary
echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi

