#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Multi-Level FIFO Scheduler Demo                        ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if built
if [ ! -f "mlq_loader" ] || [ ! -f "workload_gen" ]; then
    echo -e "${YELLOW}[!] Binaries not found. Building...${NC}"
    make all || {
        echo -e "${RED}[!] Build failed${NC}"
        exit 1
    }
fi

# Check if BPF object exists (required for loader)
if [ ! -f "multi_level_sched.bpf.o" ]; then
    echo -e "${YELLOW}[!] BPF object file not found${NC}"
    echo -e "${YELLOW}[i] BPF build may have failed (expected on systems without kernel BTF)${NC}"
    echo -e "${YELLOW}[i] Running workload generator test only...${NC}"
    ./workload_gen
    exit 0
fi

# Check kernel version
KERNEL_VERSION=$(uname -r | cut -d. -f1,2)
REQUIRED_VERSION="6.6"

if ! awk -v ver="$KERNEL_VERSION" -v req="$REQUIRED_VERSION" 'BEGIN {exit !(ver >= req)}'; then
    echo -e "${YELLOW}[!] Kernel $REQUIRED_VERSION+ required for sched_ext${NC}"
    echo -e "${YELLOW}[i] Running workload generator only...${NC}"
    ./workload_gen
    exit 0
fi

# Check if scheduler is already loaded
if [ -d "/sys/fs/bpf/mlq_sched" ] 2>/dev/null; then
    echo -e "${YELLOW}[!] Scheduler may already be loaded${NC}"
fi

echo -e "${GREEN}[+] Starting scheduler loader...${NC}"
echo -e "${YELLOW}[i] Press Ctrl+C to stop${NC}"
echo ""

# Run loader in background (requires sudo)
if ! sudo -n true 2>/dev/null; then
    echo -e "${YELLOW}[!] Sudo access required for mlq_loader${NC}"
    echo -e "${YELLOW}[i] Running workload generator test only...${NC}"
    ./workload_gen
    exit 0
fi

sudo ./mlq_loader &
LOADER_PID=$!

# Wait a bit for it to start
sleep 2

# Run workload
echo -e "${GREEN}[+] Running workload generator...${NC}"
./workload_gen &
WORKLOAD_PID=$!

# Wait for workload or user interrupt
trap "kill $LOADER_PID $WORKLOAD_PID 2>/dev/null; exit" INT TERM
wait $WORKLOAD_PID 2>/dev/null || true

# Stop loader
kill $LOADER_PID 2>/dev/null || true
wait $LOADER_PID 2>/dev/null || true

echo -e "${GREEN}[✓] Demo complete!${NC}"
