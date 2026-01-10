#!/bin/bash
set +e  # Don't exit on non-zero return, we'll handle it manually

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get the absolute path of the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Running Tests${NC}"
echo -e "${CYAN}========================================${NC}\n"

FAILED_TESTS=0
PASSED_TESTS=0

# Test function
test_check() {
    local name="$1"
    local command="$2"
    
    echo -e "${BLUE}Testing: ${name}${NC}"
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}  ✓ PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}  ✗ FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Test 1: Check if build directory exists
test_check "Build directory exists" "[ -d \"$BUILD_DIR\" ]"

# Test 2: Check if binaries exist
test_check "Dynamic binary exists" "[ -f \"${BUILD_DIR}/httpserver-dynamic\" ]"

# Test 3: Check if static binary exists (either musl or glibc)
STATIC_BINARY=""
if [ -f "${BUILD_DIR}/httpserver-static-musl" ]; then
    STATIC_BINARY="${BUILD_DIR}/httpserver-static-musl"
elif [ -f "${BUILD_DIR}/httpserver-static-glibc" ]; then
    STATIC_BINARY="${BUILD_DIR}/httpserver-static-glibc"
fi

if [ -n "$STATIC_BINARY" ]; then
    test_check "Static binary exists" "[ -f \"$STATIC_BINARY\" ]"
else
    echo -e "${RED}  ✗ FAIL: No static binary found${NC}"
    ((FAILED_TESTS++))
fi

# Test 4: Check if tools exist
test_check "memanalyzer exists" "[ -f \"${BUILD_DIR}/memanalyzer\" ]"
test_check "monitor exists" "[ -f \"${BUILD_DIR}/monitor\" ]"

# Test 5: Check if binaries are executable
if [ -n "$STATIC_BINARY" ]; then
    test_check "Static binary is executable" "[ -x \"$STATIC_BINARY\" ]"
fi
test_check "memanalyzer is executable" "[ -x \"${BUILD_DIR}/memanalyzer\" ]"
test_check "monitor is executable" "[ -x \"${BUILD_DIR}/monitor\" ]"

# Test 6: Check dynamic binary dependencies
if [ -f "${BUILD_DIR}/httpserver-dynamic" ]; then
    echo -e "${BLUE}Testing: Dynamic binary has dependencies${NC}"
    if ldd "${BUILD_DIR}/httpserver-dynamic" 2>/dev/null | grep -q "libc.so"; then
        echo -e "${GREEN}  ✓ PASS: Has dynamic dependencies${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}  ✗ FAIL: Should have dynamic dependencies${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
fi

# Test 7: Check static binary has no dependencies
if [ -n "$STATIC_BINARY" ]; then
    echo -e "${BLUE}Testing: Static binary has no dependencies${NC}"
    if ! ldd "$STATIC_BINARY" 2>&1 | grep -q "libc.so"; then
        echo -e "${GREEN}  ✓ PASS: No dynamic dependencies${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}  ✗ FAIL: Should not have dynamic dependencies${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
fi

# Test 8: Check if server can start (if not already running)
if [ ! -f "${SCRIPT_DIR}/.server.pid" ] || ! ps -p "$(cat "${SCRIPT_DIR}/.server.pid" 2>/dev/null)" > /dev/null 2>&1; then
    if [ -n "$STATIC_BINARY" ]; then
        echo -e "${BLUE}Testing: Server can start${NC}"
        cd "$SCRIPT_DIR"
        timeout 2 "$STATIC_BINARY" > /dev/null 2>&1 &
        TEST_PID=$!
        sleep 1
        if ps -p $TEST_PID > /dev/null 2>&1; then
            kill $TEST_PID 2>/dev/null || true
            sleep 1
            kill -9 $TEST_PID 2>/dev/null || true
            echo -e "${GREEN}  ✓ PASS: Server starts successfully${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}  ✗ FAIL: Server failed to start${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
else
    echo -e "${YELLOW}Skipping server start test (server already running)${NC}"
fi

# Test 9: Test HTTP response (if server is running)
if [ -f "${SCRIPT_DIR}/.server.pid" ]; then
    SERVER_PID=$(cat "${SCRIPT_DIR}/.server.pid" 2>/dev/null || echo "")
    if [ -n "$SERVER_PID" ] && ps -p "$SERVER_PID" > /dev/null 2>&1; then
        if command -v curl > /dev/null 2>&1; then
            echo -e "${BLUE}Testing: HTTP server response${NC}"
            if curl -s -f http://localhost:8080 > /dev/null 2>&1; then
                echo -e "${GREEN}  ✓ PASS: Server responds to HTTP requests${NC}"
                PASSED_TESTS=$((PASSED_TESTS + 1))
                
                # Test response content
                RESPONSE=$(curl -s http://localhost:8080)
                if echo "$RESPONSE" | grep -q "Static Binary Response"; then
                    echo -e "${GREEN}  ✓ PASS: Response content is correct${NC}"
                    PASSED_TESTS=$((PASSED_TESTS + 1))
                else
                    echo -e "${YELLOW}  ⚠ WARN: Response content unexpected${NC}"
                fi
            else
                echo -e "${RED}  ✗ FAIL: Server does not respond${NC}"
                FAILED_TESTS=$((FAILED_TESTS + 1))
            fi
        else
            echo -e "${YELLOW}Skipping HTTP test (curl not available)${NC}"
        fi
    fi
fi

# Test 10: Check source files exist
test_check "Source files exist" "[ -f \"${SCRIPT_DIR}/src/httpserver.c\" ] && [ -f \"${SCRIPT_DIR}/src/memanalyzer.c\" ] && [ -f \"${SCRIPT_DIR}/src/monitor.c\" ]"

# Test 11: Check Dockerfiles exist
test_check "Dockerfiles exist" "[ -f \"${SCRIPT_DIR}/containers/Dockerfile.dynamic\" ] && [ -f \"${SCRIPT_DIR}/containers/Dockerfile.static\" ]"

# Summary
echo -e "\n${CYAN}========================================${NC}"
echo -e "${CYAN}Test Summary${NC}"
echo -e "${CYAN}========================================${NC}"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
    exit 1
else
    echo -e "${GREEN}Failed: $FAILED_TESTS${NC}"
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
fi

