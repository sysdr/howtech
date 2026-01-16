#!/bin/bash

# Startup script for OpenOCD RISC-V Demo
# This script starts the demo services

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_header() {
    echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
}

print_step() {
    echo -e "${GREEN}➜${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_header "OpenOCD RISC-V Demo Startup"

# Check if executables exist
if [ ! -f "./openocd_validator" ] || [ ! -f "./jtag_simulator" ] || [ ! -f "./openocd_monitor" ]; then
    print_error "Executables not found. Please run setup.sh first."
    exit 1
fi

# Check for duplicate services
print_step "Checking for duplicate services..."
PROCESS_COUNT=$(ps aux | grep -E "(openocd_validator|jtag_simulator|openocd_monitor)" | grep -v grep | wc -l)
if [ "$PROCESS_COUNT" -gt 0 ]; then
    print_warning "Found $PROCESS_COUNT existing service(s). Stopping them..."
    pkill -f "openocd_validator" || true
    pkill -f "jtag_simulator" || true
    pkill -f "openocd_monitor" || true
    sleep 1
fi

# Run validator demo
print_step "Running configuration validator..."
./openocd_validator configs/riscv_target.cfg | tee output/validator_output.txt

# Run JTAG simulator demo
print_step "Running JTAG simulator..."
timeout 5 ./jtag_simulator | tee output/jtag_simulator_output.txt || true

print_step "Startup complete!"
echo ""
echo -e "${GREEN}Available tools:${NC}"
echo -e "  ./openocd_validator configs/riscv_target.cfg"
echo -e "  ./jtag_simulator"
echo -e "  ./openocd_monitor  (interactive)"

