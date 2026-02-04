#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "build/priority_inversion" ]; then
    echo "Error: build/priority_inversion not found. Run ./setup.sh first."
    exit 1
fi

if [ "$EUID" -ne 0 ]; then
    echo "WARNING: Not running as root. Real-time priorities require CAP_SYS_NICE capability."
    echo "For full demonstration, run: sudo ./demo.sh"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Function to run demonstration
run_demo() {
    local use_pi=$1
    local mode_name=$2
    
    echo "============================================"
    echo "Running: $mode_name"
    echo "============================================"
    
    # Run the main program
    "$SCRIPT_DIR/build/priority_inversion" $use_pi
    
    echo ""
}

# Run demonstrations
echo "=== Demonstration 1: Regular Mutex (Priority Inversion) ==="
run_demo 0 "WITHOUT Priority Inheritance"

echo ""
echo "Press Enter to continue to demonstration 2..."
read

echo "=== Demonstration 2: PI Mutex (Priority Inheritance) ==="
run_demo 1 "WITH Priority Inheritance"

echo ""
echo "============================================"
echo "Demonstration Complete!"
echo "============================================"
echo ""
echo "Key Observations:"
echo "1. Without PI: High priority task experiences longer wait times"
echo "   - Medium priority task preempts low priority"
echo "   - High priority blocked despite having highest priority"
echo ""
echo "2. With PI: High priority wait times are reduced"
echo "   - Low priority task temporarily boosted to high priority"
echo "   - Medium priority cannot preempt boosted low priority"
echo ""
echo "This demonstrates bounded vs unbounded priority inversion."
echo ""

