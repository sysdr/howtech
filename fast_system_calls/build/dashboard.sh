#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# If script is in build folder, use current dir; otherwise use build subdirectory
if [ "$(basename "${SCRIPT_DIR}")" = "build" ]; then
    BUILD_DIR="${SCRIPT_DIR}"
else
    BUILD_DIR="${SCRIPT_DIR}/build"
fi

# Function to clear screen and show header
clear_screen() {
    printf "\033[2J\033[H"
}

# Function to get vDSO status
get_vdso_status() {
    if ./vdso_inspector 2>&1 | grep -q "vDSO found in memory"; then
        echo "✓ Active"
    else
        echo "✗ Not Found"
    fi
}

# Function to get benchmark results
get_benchmark_results() {
    cd "${BUILD_DIR}"
    # Run quick benchmark and extract results
    OUTPUT=$(timeout 3 ./getpid_bench 2>&1 || true)
    if echo "$OUTPUT" | grep -q "Per call:"; then
        NS_PER_CALL=$(echo "$OUTPUT" | grep "Per call:" | awk '{print $3}')
        THROUGHPUT=$(echo "$OUTPUT" | grep "Throughput:" | awk '{print $3}')
        echo "$NS_PER_CALL|$THROUGHPUT"
    else
        echo "N/A|N/A"
    fi
}

# Function to check if binaries exist
check_binaries() {
    local missing=0
    for bin in "getpid_bench" "vdso_inspector" "monitor"; do
        if [ ! -f "${BUILD_DIR}/${bin}" ]; then
            missing=$((missing + 1))
        fi
    done
    echo $missing
}

# Main dashboard loop
main() {
    cd "${BUILD_DIR}"
    
    if [ ! -d "${BUILD_DIR}" ]; then
        echo -e "${RED}Error: Build directory not found!${NC}"
        echo -e "${YELLOW}Please run setup.sh first${NC}"
        exit 1
    fi
    
    local missing=$(check_binaries)
    if [ "$missing" -gt 0 ]; then
        echo -e "${RED}Error: $missing binary(ies) missing!${NC}"
        echo -e "${YELLOW}Please run setup.sh first${NC}"
        exit 1
    fi
    
    echo -e "${BOLD}${CYAN}Starting Dashboard...${NC}"
    echo -e "${YELLOW}Press Ctrl+C to stop${NC}\n"
    sleep 2
    
    while true; do
        clear_screen
        
        echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${BOLD}${CYAN}║         Fast Syscalls Dashboard                              ║${NC}"
        echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}\n"
        
        # System Info
        echo -e "${BOLD}System Information:${NC}"
        echo -e "  • Hostname:     $(hostname)"
        echo -e "  • Kernel:       $(uname -r)"
        echo -e "  • Architecture: $(uname -m)"
        echo -e "  • Uptime:       $(uptime -p 2>/dev/null || uptime | awk '{print $3,$4}' | sed 's/,//')"
        echo ""
        
        # vDSO Status
        echo -e "${BOLD}vDSO Status:${NC}"
        VDSO_STATUS=$(get_vdso_status)
        echo -e "  • vDSO:         ${VDSO_STATUS}"
        echo ""
        
        # Binary Status
        echo -e "${BOLD}Binary Status:${NC}"
        for bin in "getpid_bench" "vdso_inspector" "monitor"; do
            if [ -f "${BUILD_DIR}/${bin}" ] && [ -x "${BUILD_DIR}/${bin}" ]; then
                SIZE=$(ls -lh "${BUILD_DIR}/${bin}" | awk '{print $5}')
                echo -e "  • ${bin}:${NC}     ${GREEN}✓${NC} Ready (${SIZE})"
            else
                echo -e "  • ${bin}:${NC}     ${RED}✗${NC} Missing"
            fi
        done
        echo ""
        
        # Performance Metrics
        echo -e "${BOLD}Performance Metrics:${NC}"
        BENCH_RESULTS=$(get_benchmark_results)
        NS_PER_CALL=$(echo "$BENCH_RESULTS" | cut -d'|' -f1)
        THROUGHPUT=$(echo "$BENCH_RESULTS" | cut -d'|' -f2)
        
        if [ "$NS_PER_CALL" != "N/A" ]; then
            echo -e "  • Latency:      ${NS_PER_CALL} ns/call"
            echo -e "  • Throughput:   ${THROUGHPUT} M calls/sec"
        else
            echo -e "  • Latency:      ${YELLOW}Calculating...${NC}"
            echo -e "  • Throughput:   ${YELLOW}Calculating...${NC}"
        fi
        echo ""
        
        # Running Processes
        echo -e "${BOLD}Running Processes:${NC}"
        RUNNING=$(ps aux | grep -E "(getpid_bench|vdso_inspector|monitor)" | grep -v grep | wc -l)
        if [ "$RUNNING" -gt 0 ]; then
            echo -e "  • Active:       ${YELLOW}$RUNNING process(es)${NC}"
            ps aux | grep -E "(getpid_bench|vdso_inspector|monitor)" | grep -v grep | awk '{print "    - " $11 " (PID: " $2 ")"}'
        else
            echo -e "  • Active:       ${GREEN}None${NC}"
        fi
        echo ""
        
        # Last Update
        echo -e "${BOLD}Last Update:${NC} $(date '+%Y-%m-%d %H:%M:%S')"
        echo ""
        echo -e "${YELLOW}Press Ctrl+C to stop dashboard...${NC}"
        
        sleep 5
    done
}

# Handle Ctrl+C
trap 'echo -e "\n${YELLOW}Dashboard stopped.${NC}"; exit 0' INT TERM

main

