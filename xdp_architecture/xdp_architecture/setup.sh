#!/bin/bash
# Setup script for XDP demo
# Installs dependencies and builds the project

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}XDP Demo Setup${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check if running as root for dependency installation
if [ "$EUID" -eq 0 ]; then
    echo -e "${BLUE}Installing dependencies...${NC}"
    ./install_deps.sh
else
    echo -e "${YELLOW}Note: Not running as root. Skipping dependency installation.${NC}"
    echo -e "${YELLOW}If dependencies are missing, run: sudo ./install_deps.sh${NC}"
    echo
fi

# Create output directory
mkdir -p output

# Build the project
echo -e "${BLUE}Building XDP demo...${NC}"
make

echo
echo -e "${GREEN}Setup complete!${NC}"
echo
echo "Next steps:"
echo "  1. Load XDP program: sudo ./startup.sh"
echo "  2. Run demo: sudo ./run_demo.sh [duration] [target_ip]"
echo "  3. Run dashboard: sudo ./run_dashboard.sh"
echo "  4. Run tests: ./run_tests.sh"
echo "  5. Cleanup: sudo ./cleanup.sh"

