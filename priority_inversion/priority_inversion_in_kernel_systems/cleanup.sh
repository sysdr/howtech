#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Cleaning up generated files..."

# Remove build directory
if [ -d "build" ]; then
    rm -rf build
    echo "Removed build/ directory"
fi

# Remove src directory
if [ -d "src" ]; then
    rm -rf src
    echo "Removed src/ directory"
fi

# Remove Makefile
if [ -f "Makefile" ]; then
    rm -f Makefile
    echo "Removed Makefile"
fi

# Remove any log files
if [ -f "demo.log" ]; then
    rm -f demo.log
    echo "Removed demo.log"
fi

if [ -f "metrics.json" ]; then
    rm -f metrics.json
    echo "Removed metrics.json"
fi

echo "Cleanup complete!"

