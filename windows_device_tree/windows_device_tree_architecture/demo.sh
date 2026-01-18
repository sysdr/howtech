#!/bin/bash

# Demo script for Windows Device Tree Analyzer
# Runs setup and then executes the monitor demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=================================="
echo "Windows Device Tree Analyzer Demo"
echo "=================================="
echo ""

# Run setup if files don't exist
if [ ! -f "src/driver/device_enum.c" ] || [ ! -f "src/usermode/enum_devices.cpp" ]; then
    echo "Running setup script..."
    bash setup.sh
else
    echo "Source files already exist, skipping setup..."
    echo ""
fi

# Run the monitor demo
echo "Running device enumeration monitor..."
echo ""
bash src/usermode/monitor.sh

