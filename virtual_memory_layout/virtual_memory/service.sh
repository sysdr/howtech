#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
LOG_DIR="${SCRIPT_DIR}/logs"

stop_service() {
    local pid_file="$1"
    local service_name="$2"
    
    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file" 2>/dev/null || echo "")
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            echo -e "${GREEN}✓ Stopped $service_name (PID: $pid)${NC}"
        else
            echo -e "${YELLOW}⚠ $service_name was not running${NC}"
        fi
        rm -f "$pid_file"
    else
        echo -e "${YELLOW}⚠ $service_name PID file not found${NC}"
    fi
}

start_services() {
    echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${CYAN}    Starting Virtual Memory Layout Services${NC}"
    echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}\n"

    # Check if binaries exist
    if [ ! -f "${BUILD_DIR}/memexplore" ]; then
        echo -e "${RED}✗ memexplore binary not found. Run 'make' to build.${NC}"
        exit 1
    fi

    if [ ! -f "${BUILD_DIR}/memmonitor" ]; then
        echo -e "${RED}✗ memmonitor binary not found. Run 'make' to build.${NC}"
        exit 1
    fi

    # Create logs directory
    mkdir -p "${LOG_DIR}"

    # Check for duplicate services
    check_duplicates() {
        local pid_file="$1"
        if [ -f "$pid_file" ]; then
            local old_pid=$(cat "$pid_file" 2>/dev/null || echo "")
            if [ -n "$old_pid" ] && kill -0 "$old_pid" 2>/dev/null; then
                echo -e "${YELLOW}⚠ Service already running with PID $old_pid${NC}"
                return 1
            else
                rm -f "$pid_file"
            fi
        fi
        return 0
    }

    # Start demo process
    echo -e "${BLUE}Starting demo process...${NC}"
    DEMO_PID_FILE="${LOG_DIR}/demo.pid"
    if check_duplicates "$DEMO_PID_FILE"; then
        # Start a background process for monitoring
        (
            cd "$SCRIPT_DIR"
            while true; do
                sleep 1
            done
        ) > "${LOG_DIR}/demo.log" 2>&1 &
        DEMO_PID=$!
        echo $DEMO_PID > "$DEMO_PID_FILE"
        echo -e "${GREEN}✓ Demo process started with PID: $DEMO_PID${NC}"
    else
        DEMO_PID=$(cat "$DEMO_PID_FILE")
        echo -e "${YELLOW}Using existing demo process PID: $DEMO_PID${NC}"
    fi

    # Start metrics collector (if dashboard script exists)
    if [ -f "${SCRIPT_DIR}/dashboard_server.py" ]; then
        echo -e "${BLUE}Starting dashboard server...${NC}"
        DASHBOARD_PID_FILE="${LOG_DIR}/dashboard.pid"
        if check_duplicates "$DASHBOARD_PID_FILE"; then
            python3 "${SCRIPT_DIR}/dashboard_server.py" > "${LOG_DIR}/dashboard.log" 2>&1 &
            DASHBOARD_PID=$!
            echo $DASHBOARD_PID > "$DASHBOARD_PID_FILE"
            echo -e "${GREEN}✓ Dashboard server started with PID: $DASHBOARD_PID${NC}"
            sleep 2  # Give server time to start
        else
            DASHBOARD_PID=$(cat "$DASHBOARD_PID_FILE")
            echo -e "${YELLOW}Dashboard server already running with PID: $DASHBOARD_PID${NC}"
        fi
    fi

    echo -e "\n${GREEN}${BOLD}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}${BOLD}     Services Started Successfully${NC}"
    echo -e "${GREEN}${BOLD}═══════════════════════════════════════════════════════════════${NC}\n"

    echo -e "Demo PID: ${CYAN}$DEMO_PID${NC}"
    if [ -n "${DASHBOARD_PID:-}" ]; then
        echo -e "Dashboard PID: ${CYAN}${DASHBOARD_PID}${NC}"
        echo -e "Dashboard URL: ${CYAN}http://localhost:8080${NC}"
    fi
    echo -e "\nTo stop services, run: ${CYAN}./service.sh stop${NC}"
    echo -e "To view logs: ${CYAN}tail -f logs/*.log${NC}\n"
}

stop_services() {
    echo "Stopping services..."
    stop_service "${LOG_DIR}/demo.pid" "Demo process"
    stop_service "${LOG_DIR}/dashboard.pid" "Dashboard server"
    echo -e "\n${GREEN}All services stopped.${NC}\n"
}

# Main logic
case "${1:-start}" in
    start)
        start_services
        ;;
    stop)
        stop_services
        ;;
    *)
        echo "Usage: $0 [start|stop]"
        echo "  start - Start all services (default)"
        echo "  stop  - Stop all services"
        exit 1
        ;;
esac

