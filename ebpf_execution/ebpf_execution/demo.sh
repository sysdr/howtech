#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${CYAN}================================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}================================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}→ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Check if running with appropriate privileges
check_privileges() {
    if [ "$EUID" -ne 0 ]; then
        print_warning "This script needs sudo for eBPF operations"
        print_info "Re-running with sudo..."
        exec sudo bash "$0" "$@"
    fi
}

# Install dependencies
install_dependencies() {
    print_header "Installing Dependencies"
    
    if command -v apt-get &> /dev/null; then
        apt-get update -qq
        apt-get install -y -qq \
            build-essential \
            clang \
            llvm \
            libelf-dev \
            linux-headers-$(uname -r) \
            libbpf-dev \
            bpftool \
            libncurses5-dev \
            libncursesw5-dev \
            pkg-config \
            linux-tools-common \
            linux-tools-generic \
            2>/dev/null || true
    fi
    
    print_success "Dependencies installed"
}

# Check for required tools
check_dependencies() {
    local missing=()
    
    if ! command -v clang &> /dev/null; then
        missing+=("clang")
    fi
    if ! command -v gcc &> /dev/null; then
        missing+=("gcc")
    fi
    if ! command -v make &> /dev/null; then
        missing+=("make")
    fi
    
    if [ ${#missing[@]} -gt 0 ]; then
        print_warning "Missing dependencies: ${missing[*]}"
        print_info "Installing dependencies..."
        install_dependencies
    fi
}

# Build everything
build_all() {
    print_header "Building All Components"
    
    make clean 2>/dev/null || true
    make all
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Run demonstrations
run_demonstrations() {
    print_header "Running Demonstrations"
    
    echo -e "\n${YELLOW}Demo 1: Benchmark JIT vs Interpreter${NC}"
    echo "---------------------------------------"
    if [ -f ./build/loader ]; then
        sleep 2
        ./build/loader
    else
        print_error "Loader not found. Build may have failed."
        return 1
    fi
    
    echo -e "\n\n${YELLOW}Demo 2: Check Current JIT Status${NC}"
    echo "---------------------------------------"
    current_jit=$(cat /proc/sys/net/core/bpf_jit_enable)
    echo "Current JIT setting: $current_jit"
    echo "  0 = Interpreter only"
    echo "  1 = JIT enabled"
    echo "  2 = JIT with debug output"
    
    echo -e "\n\n${YELLOW}Demo 3: Inspect Loaded eBPF Programs${NC}"
    echo "---------------------------------------"
    if command -v bpftool &> /dev/null; then
        bpftool prog list | head -20
    else
        print_warning "bpftool not available"
    fi
    
    echo -e "\n\n${YELLOW}Demo 4: Launch Real-time Monitor${NC}"
    echo "---------------------------------------"
    if [ -f ./build/monitor ]; then
        print_info "Press 'q' to exit the monitor"
        sleep 2
        ./build/monitor
    else
        print_error "Monitor not found. Build may have failed."
    fi
}

# Main execution
main() {
    clear
    echo -e "${GREEN}"
    echo "╔════════════════════════════════════════════════════╗"
    echo "║   eBPF JIT Compilation Deep Dive - Demo Script    ║"
    echo "╚════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    
    check_privileges
    check_dependencies
    build_all
    
    print_header "Setup Complete!"
    print_success "All components built successfully"
    
    echo -e "\n${CYAN}Available components:${NC}"
    echo "  • build/counter.bpf.o  - eBPF bytecode"
    echo "  • build/loader         - Benchmark tool"
    echo "  • build/monitor        - Real-time monitor"
    
    echo -e "\n${YELLOW}Starting demonstrations...${NC}\n"
    sleep 2
    
    run_demonstrations
    
    print_header "Demo Complete!"
    echo -e "\n${GREEN}To run individual components:${NC}"
    echo "  sudo ./build/loader    - Run benchmark"
    echo "  sudo ./build/monitor   - Launch monitor"
    echo -e "\n${GREEN}To cleanup:${NC}"
    echo "  sudo ./cleanup.sh"
    echo ""
}

main "$@"
