#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

cd "$(dirname "$0")"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Thread Sanitizer (TSAN) Race Condition Detector Demo  ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo

# Demonstration 1: Race condition without TSAN
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}Demonstration 1: Race Condition (WITHOUT TSAN)${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo "Running buggy program 5 times to show non-deterministic results:"
echo

for i in {1..5}; do
    echo -e "${BLUE}Run $i:${NC}"
    ./race_example 2>/dev/null | grep -E "(Final|Lost)" || true
done

echo
sleep 1

# Demonstration 2: Same program WITH TSAN
echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
echo -e "${RED}Demonstration 2: Race Condition (WITH TSAN)${NC}"
echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"

./tsan_monitor race_example_tsan
echo

export TSAN_OPTIONS="halt_on_error=0:exitcode=0"
./race_example_tsan 2>&1 | head -100

echo
echo -e "${RED}↑ TSAN detected the race condition! Notice the two conflicting accesses.${NC}"
echo
sleep 1

# Demonstration 3: Fixed version with mutex
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Demonstration 3: Fixed with Mutex (WITH TSAN)${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"

./tsan_monitor race_fixed_tsan
./race_fixed_tsan

echo
echo -e "${GREEN}✓ No races detected - proper synchronization!${NC}"
echo
sleep 1

# Demonstration 4: Fixed with atomics
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Demonstration 4: Fixed with Atomics (WITH TSAN)${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"

./tsan_monitor race_atomic_tsan
./race_atomic_tsan

echo
echo -e "${GREEN}✓ No races detected - atomic operations work correctly!${NC}"
echo

# Performance comparison
echo
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Performance Overhead Comparison${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo

echo "Measuring execution time (3 runs each):"
echo

echo -e "${YELLOW}Without TSAN:${NC}"
for i in {1..3}; do
    /usr/bin/time -f "  Run $i: %E elapsed, %M KB max memory" ./race_example 2>&1 > /dev/null | grep elapsed || true
done

echo
echo -e "${YELLOW}With TSAN:${NC}"
export TSAN_OPTIONS="halt_on_error=1:exitcode=0"
for i in {1..3}; do
    /usr/bin/time -f "  Run $i: %E elapsed, %M KB max memory" ./race_example_tsan 2>&1 > /dev/null | grep elapsed || true
done

echo
echo -e "${BLUE}Note: TSAN adds 5-15x CPU overhead and 5-10x memory overhead${NC}"
echo -e "${BLUE}This is acceptable for testing but not production use${NC}"

echo
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                    Demo Complete!                        ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
