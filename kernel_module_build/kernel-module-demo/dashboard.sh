#!/bin/bash
# Dashboard script - displays comprehensive module metrics and status

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

clear 2>/dev/null || true
echo -e "${BOLD}${BLUE}"
cat << 'EOF'
╔══════════════════════════════════════════════════════════════════════╗
║                  KERNEL MODULE DASHBOARD                             ║
║                  Real-Time Metrics & Status                           ║
╚══════════════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}\n"

# Function to print metric
print_metric() {
    local label=$1
    local value=$2
    local status=$3  # "ok", "warning", "error"
    local color=$GREEN
    case $status in
        warning) color=$YELLOW ;;
        error) color=$RED ;;
    esac
    printf "  %-25s: ${color}%s${NC}\n" "$label" "$value"
}

# Module Status
echo -e "${BOLD}${CYAN}┌─ MODULE STATUS ─────────────────────────────────────────────┐${NC}"
MODULE_LOADED=0
if lsmod | grep -q "^hello_module"; then
    MODULE_LOADED=1
    MODULE_INFO=$(lsmod | grep "^hello_module")
    SIZE=$(echo $MODULE_INFO | awk '{print $2}')
    REFCNT=$(echo $MODULE_INFO | awk '{print $3}')
    print_metric "Status" "LOADED" "ok"
    print_metric "Size" "${SIZE} bytes" "ok"
    print_metric "Reference Count" "$REFCNT" "$([ "$REFCNT" -gt 1 ] && echo "warning" || echo "ok")"
else
    print_metric "Status" "NOT LOADED" "error"
fi
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# Module File Status
echo -e "${BOLD}${CYAN}┌─ MODULE FILES ──────────────────────────────────────────────┐${NC}"
if [ -f "src/hello_module.ko" ]; then
    FILE_SIZE=$(ls -lh src/hello_module.ko | awk '{print $5}')
    FILE_DATE=$(stat -c %y src/hello_module.ko | cut -d'.' -f1)
    print_metric "Module File" "EXISTS" "ok"
    print_metric "File Size" "$FILE_SIZE" "ok"
    print_metric "Last Modified" "$FILE_DATE" "ok"
else
    print_metric "Module File" "NOT FOUND" "error"
fi

if [ -f "module_monitor" ]; then
    print_metric "Monitor Binary" "EXISTS" "ok"
else
    print_metric "Monitor Binary" "NOT FOUND" "warning"
fi
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# Module Parameters (if loaded)
if [ $MODULE_LOADED -eq 1 ]; then
    echo -e "${BOLD}${CYAN}┌─ MODULE PARAMETERS ─────────────────────────────────────────┐${NC}"
    if [ -f "/sys/module/hello_module/parameters/name" ]; then
        NAME_PARAM=$(cat /sys/module/hello_module/parameters/name)
        print_metric "name" "$NAME_PARAM" "ok"
    fi
    if [ -f "/sys/module/hello_module/parameters/count" ]; then
        COUNT_PARAM=$(cat /sys/module/hello_module/parameters/count)
        print_metric "count" "$COUNT_PARAM" "ok"
    fi
    echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"
fi

# Kernel Logs Statistics
echo -e "${BOLD}${CYAN}┌─ KERNEL LOGS STATISTICS ─────────────────────────────────────┐${NC}"
TOTAL_LOGS=$(dmesg 2>/dev/null | grep -c "hello_module" 2>/dev/null || true)
TOTAL_LOGS=${TOTAL_LOGS:-0}
INIT_LOGS=$(dmesg 2>/dev/null | grep -c "hello_module.*Initializing" 2>/dev/null || true)
INIT_LOGS=${INIT_LOGS:-0}
EXIT_LOGS=$(dmesg 2>/dev/null | grep -c "hello_module.*Shutting down" 2>/dev/null || true)
EXIT_LOGS=${EXIT_LOGS:-0}
HELLO_LOGS=$(dmesg 2>/dev/null | grep -c "hello_module.*Hello" 2>/dev/null || true)
HELLO_LOGS=${HELLO_LOGS:-0}
GOODBYE_LOGS=$(dmesg 2>/dev/null | grep -c "hello_module.*Goodbye" 2>/dev/null || true)
GOODBYE_LOGS=${GOODBYE_LOGS:-0}

print_metric "Total Log Entries" "$TOTAL_LOGS" "ok"
print_metric "Initialization" "$INIT_LOGS" "ok"
print_metric "Shutdown" "$EXIT_LOGS" "ok"
print_metric "Hello Messages" "$HELLO_LOGS" "ok"
print_metric "Goodbye Messages" "$GOODBYE_LOGS" "ok"
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# Recent Kernel Logs
echo -e "${BOLD}${CYAN}┌─ RECENT KERNEL LOGS (last 5) ────────────────────────────────┐${NC}"
RECENT=$(dmesg | grep "hello_module" | tail -5)
if [ -n "$RECENT" ]; then
    echo "$RECENT" | while IFS= read -r line; do
        echo -e "  ${YELLOW}$line${NC}"
    done
else
    echo -e "  ${RED}No kernel logs found${NC}"
fi
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# System Information
echo -e "${BOLD}${CYAN}┌─ SYSTEM INFORMATION ────────────────────────────────────────┐${NC}"
KERNEL_VERSION=$(uname -r)
print_metric "Kernel Version" "$KERNEL_VERSION" "ok"

if [ -d "/lib/modules/${KERNEL_VERSION}/build" ]; then
    print_metric "Kernel Headers" "INSTALLED" "ok"
else
    print_metric "Kernel Headers" "NOT FOUND" "error"
fi

if command -v gcc &> /dev/null; then
    GCC_VER=$(gcc --version | head -1 | cut -d' ' -f3)
    print_metric "GCC Version" "$GCC_VER" "ok"
fi
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# Process Status
echo -e "${BOLD}${CYAN}┌─ PROCESS STATUS ────────────────────────────────────────────┐${NC}"
MONITOR_COUNT=$(ps aux | grep -c "[m]odule_monitor" || echo "0")
if [ "$MONITOR_COUNT" -gt 0 ]; then
    print_metric "Monitor Processes" "$MONITOR_COUNT running" "ok"
    ps aux | grep "[m]odule_monitor" | awk '{print "  PID: " $2 " | CPU: " $3 "% | MEM: " $4 "%"}'
else
    print_metric "Monitor Processes" "NOT RUNNING" "warning"
fi
echo -e "${BOLD}${CYAN}└────────────────────────────────────────────────────────────┘${NC}\n"

# Quick Actions
echo -e "${BOLD}${BLUE}QUICK ACTIONS:${NC}"
if [ $MODULE_LOADED -eq 1 ]; then
    echo -e "  ${GREEN}Module is loaded${NC}"
    echo -e "  To unload: ${YELLOW}sudo rmmod hello_module${NC}"
    echo -e "  To reload:  ${YELLOW}sudo rmmod hello_module && sudo insmod src/hello_module.ko${NC}"
else
    echo -e "  ${RED}Module is not loaded${NC}"
    echo -e "  To load:   ${YELLOW}sudo insmod src/hello_module.ko${NC}"
    echo -e "  With params: ${YELLOW}sudo insmod src/hello_module.ko name=Alice count=3${NC}"
fi
echo -e "  Start monitor: ${YELLOW}./module_monitor${NC}"
echo -e "  Run tests:     ${YELLOW}./test.sh${NC}"
echo ""

