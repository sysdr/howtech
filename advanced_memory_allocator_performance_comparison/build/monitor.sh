#!/usr/bin/env bash
# monitor.sh — live RSS + page-fault monitor for running bench processes
# Polls /proc/<PID>/status every 500ms and prints a live table
set -euo pipefail
PID="$1"
BOLD='\033[1m'; RESET='\033[0m'; CYAN='\033[0;36m'; GREEN='\033[0;32m'
YELLOW='\033[0;33m'; RED='\033[0;31m'

clear
echo -e "${BOLD}${CYAN}┌──────────────────────────────────────────────────────────┐${RESET}"
echo -e "${BOLD}${CYAN}│       Live Memory Monitor  PID: $PID                      │${RESET}"
echo -e "${BOLD}${CYAN}└──────────────────────────────────────────────────────────┘${RESET}"
echo ""

prev_minflt=0; prev_majflt=0; prev_ts=$(date +%s%N)

while kill -0 "$PID" 2>/dev/null; do
    sleep 0.5
    [ -f "/proc/$PID/status" ] || break

    vmrss=$(awk '/VmRSS:/{print $2}' "/proc/$PID/status" 2>/dev/null || echo 0)
    vmvirt=$(awk '/VmSize:/{print $2}' "/proc/$PID/status" 2>/dev/null || echo 0)
    threads=$(awk '/Threads:/{print $2}' "/proc/$PID/status" 2>/dev/null || echo 0)

    stat=( $(cat "/proc/$PID/stat" 2>/dev/null || echo "0 0 0 0 0 0 0 0 0 0 0") )
    minflt=${stat[9]:-0}; majflt=${stat[11]:-0}
    utime=${stat[13]:-0}; stime=${stat[14]:-0}

    cur_ts=$(date +%s%N)
    delta_minflt=$((minflt - prev_minflt))
    prev_minflt=$minflt; prev_majflt=$majflt; prev_ts=$cur_ts

    printf "\r  ${BOLD}RSS:${RESET} ${GREEN}%-8s KB${RESET}  ${BOLD}VmSize:${RESET} %-8s KB  ${BOLD}Threads:${RESET} %-3s  ${BOLD}MinFlt/s:${RESET} ${YELLOW}%-6s${RESET}" \
        "$vmrss" "$vmvirt" "$threads" "$((delta_minflt * 2))"
done
echo -e "\n\n${BOLD}Process exited.${RESET}\n"
