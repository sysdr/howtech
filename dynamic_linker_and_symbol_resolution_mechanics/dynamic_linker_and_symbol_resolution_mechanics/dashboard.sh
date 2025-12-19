#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

clear_screen() {
    printf "\033[2J\033[H"
}

print_header() {
    echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║     LD_PRELOAD Memory Interposition Metrics Dashboard      ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}\n"
}

collect_metrics() {
    # Run the test program with LD_PRELOAD and capture statistics
    OUTPUT=$(LD_PRELOAD="$SCRIPT_DIR/build/malloc_hook.so" "$SCRIPT_DIR/build/test_program" 2>&1)
    
    # Extract metrics from hook output (from stderr, look for [HOOK] Final Statistics)
    TOTAL_ALLOC=$(echo "$OUTPUT" | grep "Total allocations:" | sed 's/.*Total allocations: //' | awk '{print $1}')
    TOTAL_FREES=$(echo "$OUTPUT" | grep "Total frees:" | sed 's/.*Total frees: //' | awk '{print $1}')
    BYTES_ALLOC=$(echo "$OUTPUT" | grep "Bytes allocated:" | sed 's/.*Bytes allocated: //' | awk '{print $1}')
    BYTES_FREED=$(echo "$OUTPUT" | grep "Bytes freed:" | sed 's/.*Bytes freed: //' | awk '{print $1}')
    PEAK_USAGE=$(echo "$OUTPUT" | grep "Peak usage:" | sed 's/.*Peak usage: //' | awk '{print $1}')
    CURRENT_LEAKS=$(echo "$OUTPUT" | grep "Current leaks:" | sed 's/.*Current leaks: //' | awk '{print $1}')
    
    # Count malloc/free/calloc/realloc calls
    MALLOC_COUNT=$(echo "$OUTPUT" | grep -c "\[MALLOC\]" || echo "0")
    FREE_COUNT=$(echo "$OUTPUT" | grep -c "\[FREE\]" || echo "0")
    CALLOC_COUNT=$(echo "$OUTPUT" | grep -c "\[CALLOC\]" || echo "0")
    REALLOC_COUNT=$(echo "$OUTPUT" | grep -c "\[REALLOC\]" || echo "0")
}

display_metrics() {
    clear_screen
    print_header
    
    echo -e "${YELLOW}Memory Allocation Statistics:${NC}\n"
    
    if [ -n "$TOTAL_ALLOC" ]; then
        echo -e "${GREEN}Total Allocations:${NC}     ${BLUE}${TOTAL_ALLOC:-0}${NC}"
        echo -e "${GREEN}Total Frees:${NC}           ${BLUE}${TOTAL_FREES:-0}${NC}"
        echo -e "${GREEN}Bytes Allocated:${NC}       ${BLUE}${BYTES_ALLOC:-0} bytes${NC}"
        echo -e "${GREEN}Bytes Freed:${NC}           ${BLUE}${BYTES_FREED:-0} bytes${NC}"
        echo -e "${GREEN}Peak Usage:${NC}            ${BLUE}${PEAK_USAGE:-0} bytes${NC}"
        echo -e "${GREEN}Current Leaks:${NC}        ${BLUE}${CURRENT_LEAKS:-0} bytes${NC}"
    else
        echo -e "${RED}Error: Could not collect metrics${NC}"
        return 1
    fi
    
    echo -e "\n${YELLOW}Function Call Breakdown:${NC}\n"
    echo -e "${GREEN}malloc() calls:${NC}   ${BLUE}${MALLOC_COUNT:-0}${NC}"
    echo -e "${GREEN}free() calls:${NC}     ${BLUE}${FREE_COUNT:-0}${NC}"
    echo -e "${GREEN}calloc() calls:${NC}   ${BLUE}${CALLOC_COUNT:-0}${NC}"
    echo -e "${GREEN}realloc() calls:${NC}  ${BLUE}${REALLOC_COUNT:-0}${NC}"
    
    echo -e "\n${YELLOW}Build Status:${NC}\n"
    if [ -f "build/malloc_hook.so" ]; then
        echo -e "${GREEN}✓${NC} malloc_hook.so      ${BLUE}$(ls -lh build/malloc_hook.so | awk '{print $5}')${NC}"
    else
        echo -e "${RED}✗${NC} malloc_hook.so not found"
    fi
    
    if [ -f "build/test_program" ]; then
        echo -e "${GREEN}✓${NC} test_program        ${BLUE}$(ls -lh build/test_program | awk '{print $5}')${NC}"
    else
        echo -e "${RED}✗${NC} test_program not found"
    fi
    
    if [ -f "build/monitor" ]; then
        echo -e "${GREEN}✓${NC} monitor             ${BLUE}$(ls -lh build/monitor | awk '{print $5}')${NC}"
    else
        echo -e "${RED}✗${NC} monitor not found"
    fi
    
    echo -e "\n${CYAN}═════════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}Dashboard updated successfully!${NC}\n"
}

# Check if build files exist
if [ ! -f "./build/malloc_hook.so" ] || [ ! -f "./build/test_program" ]; then
    echo -e "${RED}Error: Build files not found. Run setup.sh first.${NC}"
    exit 1
fi

# Collect and display metrics
collect_metrics
display_metrics

