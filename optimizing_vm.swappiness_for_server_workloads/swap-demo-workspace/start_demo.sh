#!/usr/bin/env bash
# Startup script for vm.swappiness demo

set -euo pipefail

WORKDIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$WORKDIR/build"
PROBE="$BUILD_DIR/swappiness_probe"
MONITOR="$BUILD_DIR/lru_monitor"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RESET='\033[0m'

echo -e "${BLUE}========================================${RESET}"
echo -e "${BLUE}vm.swappiness Demo - Startup Script${RESET}"
echo -e "${BLUE}========================================${RESET}"
echo ""

# Check if binaries exist
if [ ! -f "$PROBE" ] || [ ! -f "$MONITOR" ]; then
    echo -e "${YELLOW}Error: Binaries not found. Run setup.sh first.${RESET}"
    exit 1
fi

# Check for duplicate processes
echo "Checking for running demo processes..."
PROBE_PIDS=$(pgrep -f "swappiness_probe" || true)
MONITOR_PIDS=$(pgrep -f "lru_monitor" || true)

if [ -n "$PROBE_PIDS" ]; then
    echo -e "${YELLOW}Warning: swappiness_probe is already running (PIDs: $PROBE_PIDS)${RESET}"
fi

if [ -n "$MONITOR_PIDS" ]; then
    echo -e "${YELLOW}Warning: lru_monitor is already running (PIDs: $MONITOR_PIDS)${RESET}"
    echo -e "${YELLOW}Killing existing lru_monitor processes...${RESET}"
    pkill -f "lru_monitor" || true
    sleep 1
fi

echo ""
echo -e "${GREEN}Starting vm.swappiness demo...${RESET}"
echo ""

# Display current system state
echo "=== Current System State ==="
"$PROBE"
echo ""

# Ask user what to run
echo "Options:"
echo "1. Run swappiness_probe (one-time snapshot)"
echo "2. Run lru_monitor (real-time monitoring)"
echo "3. Run both (probe first, then monitor)"
echo ""
read -p "Select option [1-3] (default: 1): " choice
choice=${choice:-1}

case "$choice" in
    1)
        echo ""
        echo "=== Running swappiness_probe ==="
        "$PROBE"
        ;;
    2)
        echo ""
        echo "=== Starting lru_monitor ==="
        echo "Press 'q' to quit the monitor"
        sleep 2
        "$MONITOR"
        ;;
    3)
        echo ""
        echo "=== Running swappiness_probe ==="
        "$PROBE"
        echo ""
        echo "=== Starting lru_monitor ==="
        echo "Press 'q' to quit the monitor"
        sleep 2
        "$MONITOR"
        ;;
    *)
        echo "Invalid option"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Demo completed.${RESET}"

