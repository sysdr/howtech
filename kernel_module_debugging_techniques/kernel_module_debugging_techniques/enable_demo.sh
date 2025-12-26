#!/bin/bash
# Enable demo mode on the dashboard

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${CYAN}Enabling demo mode on dashboard...${NC}"

# Check if dashboard is running
if ! curl -s http://127.0.0.1:8080/api/metrics > /dev/null 2>&1; then
    echo -e "${YELLOW}⚠ Dashboard is not running. Starting it...${NC}"
    ./start_dashboard.sh
    sleep 3
fi

# Enable demo mode
response=$(curl -s -X POST http://127.0.0.1:8080/api/demo \
    -H "Content-Type: application/json" \
    -d '{"enable": true}')

if echo "$response" | grep -q "success"; then
    echo -e "${GREEN}✓ Demo mode enabled successfully${NC}"
    echo -e "${CYAN}Dashboard available at: http://localhost:8080${NC}"
    echo -e "${CYAN}Demo data is now being displayed${NC}"
else
    echo -e "${YELLOW}⚠ Failed to enable demo mode. Response: $response${NC}"
    exit 1
fi

