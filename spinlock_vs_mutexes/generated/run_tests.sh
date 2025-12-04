#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Running tests..."
echo ""

# Test 1: Check if executables exist
echo -n "Test 1: Checking executables... "
if [ -f "build/spinlock_test" ] && [ -f "build/mutex_test" ] && [ -f "build/monitor" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

# Test 2: Run spinlock test
echo -n "Test 2: Running spinlock test... "
mkdir -p logs
if timeout 5 "$SCRIPT_DIR/build/spinlock_test" > logs/test_spinlock.log 2>&1; then
    if grep -q "Final counter" logs/test_spinlock.log; then
        echo -e "${GREEN}PASS${NC}"
    else
        echo -e "${RED}FAIL${NC}"
        exit 1
    fi
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

# Test 3: Run mutex test
echo -n "Test 3: Running mutex test... "
if timeout 5 "$SCRIPT_DIR/build/mutex_test" > logs/test_mutex.log 2>&1; then
    if grep -q "Final counter" logs/test_mutex.log; then
        echo -e "${GREEN}PASS${NC}"
    else
        echo -e "${RED}FAIL${NC}"
        exit 1
    fi
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

# Test 4: Check counter correctness
echo -n "Test 4: Verifying counter correctness... "
SPIN_COUNT=$(grep "Final counter" logs/test_spinlock.log | grep -oE '[0-9]+' | head -1)
MUTEX_COUNT=$(grep "Final counter" logs/test_mutex.log | grep -oE '[0-9]+' | head -1)
EXPECTED=4000000  # 4 threads * 1000000 iterations

if [ "$SPIN_COUNT" = "$EXPECTED" ] && [ "$MUTEX_COUNT" = "$EXPECTED" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${YELLOW}WARN${NC} (Expected: $EXPECTED, Spinlock: $SPIN_COUNT, Mutex: $MUTEX_COUNT)"
fi

echo ""
echo -e "${GREEN}All tests completed!${NC}"
