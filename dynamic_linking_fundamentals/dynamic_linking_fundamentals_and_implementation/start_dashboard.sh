#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DASHBOARD_SCRIPT="${SCRIPT_DIR}/dashboard.py"
PORT="${PORT:-8080}"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Starting Dynamic Linking Dashboard${NC}"
echo -e "${CYAN}========================================${NC}"
echo

# Check if dashboard script exists
if [ ! -f "${DASHBOARD_SCRIPT}" ]; then
    echo -e "${RED}Error: Dashboard script not found at ${DASHBOARD_SCRIPT}${NC}"
    exit 1
fi

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 is not installed${NC}"
    exit 1
fi

# Check if executable and plugins exist
if [ ! -f "${SCRIPT_DIR}/build/plugin_demo" ]; then
    echo -e "${YELLOW}Warning: plugin_demo not found. Run setup.sh first.${NC}"
fi

# Check for existing dashboard process
EXISTING_PID=$(pgrep -f "dashboard.py" | head -1)
if [ -n "${EXISTING_PID}" ]; then
    echo -e "${YELLOW}Warning: Dashboard already running (PID: ${EXISTING_PID})${NC}"
    echo -e "${YELLOW}Killing existing process...${NC}"
    kill "${EXISTING_PID}" 2>/dev/null || true
    sleep 1
fi

# Make dashboard executable
chmod +x "${DASHBOARD_SCRIPT}"

# Start dashboard
echo -e "${GREEN}Starting dashboard on port ${PORT}...${NC}"
echo -e "${BLUE}Open http://localhost:${PORT} in your browser${NC}"
echo

cd "${SCRIPT_DIR}"
python3 "${DASHBOARD_SCRIPT}" &
DASHBOARD_PID=$!

# Wait a moment for server to start
sleep 2

# Check if process is still running
if ps -p ${DASHBOARD_PID} > /dev/null 2>&1; then
    echo -e "${GREEN}Dashboard started successfully (PID: ${DASHBOARD_PID})${NC}"
    echo -e "${CYAN}Press Ctrl+C to stop${NC}"
    echo
    
    # Wait for user interrupt
    trap "echo -e '\n${YELLOW}Stopping dashboard...${NC}'; kill ${DASHBOARD_PID} 2>/dev/null; exit 0" INT TERM
    wait ${DASHBOARD_PID}
else
    echo -e "${RED}Error: Dashboard failed to start${NC}"
    exit 1
fi

