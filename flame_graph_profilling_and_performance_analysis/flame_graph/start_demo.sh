#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}=== Starting Flame Graph Profiling Demo ===${NC}"

# Check if binaries exist
if [ ! -f "build/hotspot_demo" ]; then
    echo -e "${RED}Error: build/hotspot_demo not found. Run setup.sh first.${NC}"
    exit 1
fi

if [ ! -f "build/monitor" ]; then
    echo -e "${RED}Error: build/monitor not found. Run setup.sh first.${NC}"
    exit 1
fi

# Check for FlameGraph tools
if [ ! -d "FlameGraph" ]; then
    echo -e "${YELLOW}FlameGraph directory not found. Cloning...${NC}"
    git clone https://github.com/brendangregg/FlameGraph
fi

# Check for perf
if ! command -v perf &> /dev/null; then
    echo -e "${YELLOW}Warning: perf command not found${NC}"
    echo -e "${YELLOW}To install perf, run:${NC}"
    echo -e "  sudo apt-get install -y linux-tools-$(uname -r) || sudo apt-get install -y linux-tools-generic"
    echo -e "${YELLOW}Note: Profiling requires perf. Continuing with demo mode (no profiling)...${NC}"
    PERF_AVAILABLE=0
else
    PERF_AVAILABLE=1
fi

# Create output directory
mkdir -p output

# Clean up any previous runs
pkill -f "build/hotspot_demo" 2>/dev/null || true
pkill -f "build/monitor" 2>/dev/null || true
rm -f output/perf.data output/perf.out output/folded.out

echo -e "${GREEN}Starting target program...${NC}"
./build/hotspot_demo &
TARGET_PID=$!
echo -e "${GREEN}Started target program (PID: $TARGET_PID)${NC}"

# Wait a moment for it to start
sleep 1

# Start monitor in background
echo -e "${GREEN}Starting monitor...${NC}"
./build/monitor $TARGET_PID > /dev/null 2>&1 &
MONITOR_PID=$!

# Start perf recording
if [ $PERF_AVAILABLE -eq 1 ]; then
    echo -e "${YELLOW}Starting perf profiler...${NC}"
    sudo perf record -F 99 -g --call-graph dwarf -p $TARGET_PID -o output/perf.data &
    PERF_PID=$!
else
    PERF_PID=""
    echo -e "${YELLOW}Skipping perf profiling (perf not available)${NC}"
fi

# Wait for program to complete
echo -e "${BLUE}Profiling in progress... (this will take ~30 seconds)${NC}"
wait $TARGET_PID 2>/dev/null || true

# Stop monitor and perf
kill $MONITOR_PID 2>/dev/null || true
if [ -n "$PERF_PID" ]; then
    sudo kill -INT $PERF_PID 2>/dev/null || true
fi
sleep 2

echo -e "\n${GREEN}✓ Profiling complete${NC}"

# Generate flame graph
if [ $PERF_AVAILABLE -eq 1 ]; then
    echo -e "\n${BLUE}Generating flame graph...${NC}"
    cd output

    # Process perf data
    if [ -f perf.data ]; then
        sudo perf script -i perf.data > perf.out 2>/dev/null || true
        if [ -f perf.out ] && [ -s perf.out ]; then
            ../FlameGraph/stackcollapse-perf.pl perf.out > folded.out 2>/dev/null || true
            if [ -f folded.out ] && [ -s folded.out ]; then
                ../FlameGraph/flamegraph.pl folded.out > flamegraph.svg 2>/dev/null || true
            fi
        fi
        
        # Generate stats
        sudo perf report -i perf.data --stdio > perf-report.txt 2>&1 || true
    fi

    cd ..
else
    echo -e "\n${YELLOW}Skipping flame graph generation (perf not available)${NC}"
fi

echo -e "${GREEN}✓ Flame graph generated${NC}"

# Display results
echo -e "\n${BLUE}=== Results ===${NC}"
echo ""
if [ -f output/flamegraph.svg ]; then
    echo -e "${GREEN}✓ output/flamegraph.svg${NC}     - Interactive flame graph"
else
    echo -e "${YELLOW}⚠ output/flamegraph.svg${NC}     - Not generated"
fi

if [ -f output/perf.data ]; then
    echo -e "${GREEN}✓ output/perf.data${NC}           - Raw perf data"
else
    echo -e "${YELLOW}⚠ output/perf.data${NC}           - Not generated"
fi

if [ -f output/perf-report.txt ]; then
    echo -e "${GREEN}✓ output/perf-report.txt${NC}    - Text report"
else
    echo -e "${YELLOW}⚠ output/perf-report.txt${NC}    - Not generated"
fi

echo ""
echo -e "${GREEN}Demo complete!${NC}"

