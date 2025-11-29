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

echo -e "${BOLD}${CYAN}======================================${NC}"
echo -e "${BOLD}${CYAN}   Starting Web Dashboard             ${NC}"
echo -e "${BOLD}${CYAN}======================================${NC}\n"

cd "${BUILD_DIR}"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: Python 3 is not installed!${NC}"
    exit 1
fi

# Check if dashboard server exists
if [ ! -f "dashboard_server.py" ]; then
    echo -e "${RED}Error: dashboard_server.py not found!${NC}"
    exit 1
fi

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${RED}Error: Build directory not found!${NC}"
    echo -e "${YELLOW}Please run setup.sh first${NC}"
    exit 1
fi

# Check for existing dashboard server
if pgrep -f "dashboard_server.py" > /dev/null 2>&1; then
    echo -e "${YELLOW}Warning: Dashboard server is already running${NC}"
    echo -e "${YELLOW}Killing existing server...${NC}"
    pkill -f "dashboard_server.py" || true
    sleep 1
fi

# Make server executable
chmod +x dashboard_server.py

echo -e "${GREEN}✓${NC} Starting web dashboard server..."
echo -e "\n${CYAN}The dashboard will be available at:${NC}"
echo -e "  ${BOLD}http://localhost:8080${NC}\n"

# Run the server
python3 dashboard_server.py

