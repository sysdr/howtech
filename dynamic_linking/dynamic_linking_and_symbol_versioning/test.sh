#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

print_test() {
    echo -e "${CYAN}Testing: $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check if build directory exists
if [ ! -d "build" ] || [ ! -f "build/libmylib.so.3.0.0" ]; then
    print_error "Build directory or binaries not found. Run ./setup.sh first."
    exit 1
fi

echo -e "\n${YELLOW}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}Running Tests${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════════${NC}\n"

# Test 1: Check if library exists and has correct version info
print_test "Library version definitions"
if readelf -V build/libmylib.so.3 | grep -q "MYLIB_1.0\|MYLIB_2.0\|MYLIB_3.0"; then
    print_success "Library has version definitions"
else
    print_error "Library missing version definitions"
    exit 1
fi

# Test 2: Check if apps exist
print_test "Application binaries"
if [ -f "build/app_v1" ] && [ -f "build/app_v2" ]; then
    print_success "Application binaries exist"
else
    print_error "Application binaries missing"
    exit 1
fi

# Test 3: Run app_v1
print_test "Running app_v1 (v1.0 API)"
if LD_LIBRARY_PATH=./build ./build/app_v1 > /tmp/test_app_v1.out 2>&1; then
    if grep -q "v1.0" /tmp/test_app_v1.out; then
        print_success "app_v1 runs correctly"
    else
        print_error "app_v1 output incorrect"
        exit 1
    fi
else
    print_error "app_v1 failed to run"
    exit 1
fi

# Test 4: Run app_v2
print_test "Running app_v2 (v2.0 API)"
if LD_LIBRARY_PATH=./build ./build/app_v2 > /tmp/test_app_v2.out 2>&1; then
    if grep -q "v2.0" /tmp/test_app_v2.out; then
        print_success "app_v2 runs correctly"
    else
        print_error "app_v2 output incorrect"
        exit 1
    fi
else
    print_error "app_v2 failed to run"
    exit 1
fi

# Test 5: Check symbol versions
print_test "Symbol versioning"
if objdump --dynamic-syms build/libmylib.so.3 | grep -q "api_init\|api_process\|api_init_v2"; then
    print_success "Symbols are versioned correctly"
else
    print_error "Symbol versioning issue"
    exit 1
fi

# Test 6: Check monitor exists
print_test "Monitor binary"
if [ -f "build/monitor" ]; then
    print_success "Monitor binary exists"
else
    print_error "Monitor binary missing"
    exit 1
fi

echo -e "\n${GREEN}All tests passed!${NC}\n"
rm -f /tmp/test_app_v1.out /tmp/test_app_v2.out

