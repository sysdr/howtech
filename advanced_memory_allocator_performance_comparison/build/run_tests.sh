#!/usr/bin/env bash
# run_tests.sh — Validate setup and run tests
set -euo pipefail
BOLD='\033[1m'; GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'; RESET='\033[0m'
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$WORKDIR/build"

PASSED=0
FAILED=0

test_check() {
    if [ "$1" -eq 0 ]; then
        echo -e "${GREEN}✓${RESET} $2"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗${RESET} $2"
        FAILED=$((FAILED + 1))
    fi
}

echo -e "${BOLD}Running tests...${RESET}\n"

# Test 1: Check required files exist
echo "Test 1: Checking required files..."
[ -f "$BUILD_DIR/alloc_bench.c" ] && test_check 0 "alloc_bench.c exists" || test_check 1 "alloc_bench.c missing"
[ -f "$BUILD_DIR/Makefile" ] && test_check 0 "Makefile exists" || test_check 1 "Makefile missing"
[ -f "$BUILD_DIR/Dockerfile" ] && test_check 0 "Dockerfile exists" || test_check 1 "Dockerfile missing"
[ -f "$BUILD_DIR/syscall_trace.sh" ] && test_check 0 "syscall_trace.sh exists" || test_check 1 "syscall_trace.sh missing"
[ -f "$BUILD_DIR/monitor.sh" ] && test_check 0 "monitor.sh exists" || test_check 1 "monitor.sh missing"

# Test 2: Check Docker image exists
echo -e "\nTest 2: Checking Docker image..."
docker images alloc-bench:latest --format "{{.Repository}}:{{.Tag}}" | grep -q "alloc-bench:latest" && \
    test_check 0 "Docker image alloc-bench:latest exists" || \
    test_check 1 "Docker image alloc-bench:latest missing"

# Test 3: Check binaries in container
echo -e "\nTest 3: Checking binaries in container..."
docker run --rm alloc-bench:latest test -f /bench/bench_system && \
    test_check 0 "bench_system binary exists" || \
    test_check 1 "bench_system binary missing"
docker run --rm alloc-bench:latest test -f /bench/bench_tcmalloc && \
    test_check 0 "bench_tcmalloc binary exists" || \
    test_check 1 "bench_tcmalloc binary missing"
docker run --rm alloc-bench:latest test -f /bench/bench_jemalloc && \
    test_check 0 "bench_jemalloc binary exists" || \
    test_check 1 "bench_jemalloc binary missing"

# Test 4: Quick functionality test
echo -e "\nTest 4: Quick functionality test..."
docker run --rm alloc-bench:latest /bench/bench_system "test" > /dev/null 2>&1 && \
    test_check 0 "bench_system runs successfully" || \
    test_check 1 "bench_system failed to run"

echo -e "\n${BOLD}Test Summary:${RESET}"
echo -e "  ${GREEN}Passed: $PASSED${RESET}"
echo -e "  ${RED}Failed: $FAILED${RESET}"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✓ All tests passed!${RESET}\n"
    exit 0
else
    echo -e "\n${RED}✗ Some tests failed!${RESET}\n"
    exit 1
fi
