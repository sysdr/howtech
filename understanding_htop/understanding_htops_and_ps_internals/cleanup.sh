#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${YELLOW}Cleaning up project...${NC}"

# Kill any running stress processes
echo "Stopping any running stress tests..."
pkill -f "build/stress" 2>/dev/null || true
sleep 1

# Clean build artifacts
echo "Cleaning build artifacts..."
make clean 2>/dev/null || true

# Remove build directory
if [ -d "$SCRIPT_DIR/build" ]; then
    rm -rf "$SCRIPT_DIR/build"
    echo "Removed build directory"
fi

echo -e "${GREEN}Cleanup complete!${NC}"

