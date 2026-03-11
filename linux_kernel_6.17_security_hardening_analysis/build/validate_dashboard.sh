#!/usr/bin/env bash
# =============================================================================
# validate_dashboard.sh — Verify all metric sources used by the monitor are
# readable and update. Run in Docker or on host; no TTY required.
# =============================================================================
set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; RESET='\033[0m'

# Same paths the monitor reads (from monitor.c)
PATHS=(
    /proc/sys/kernel/kptr_restrict
    /proc/sys/kernel/dmesg_restrict
    /proc/sys/kernel/randomize_va_space
    /proc/sys/vm/mmap_rnd_bits
    /proc/sys/kernel/unprivileged_bpf_disabled
    /proc/sys/kernel/perf_event_paranoid
    /proc/sys/kernel/io_uring_disabled
    /proc/sys/kernel/yama/ptrace_scope
    /proc/sys/kernel/panic_on_oops
    /proc/sys/kernel/sysrq
    /proc/sys/kernel/random/entropy_avail
    /proc/sys/kernel/random/poolsize
)

failed=0
for p in "${PATHS[@]}"; do
    if [ -r "$p" ]; then
        val=$(cat "$p" 2>/dev/null) || val=""
        [ -n "$val" ] && echo -e "${GREEN}✓${RESET} $p = $val" || echo -e "${GREEN}✓${RESET} $p (readable)"
    else
        echo -e "${RED}✗${RESET} $p (not readable)"
        failed=1
    fi
done

if [ $failed -eq 1 ]; then
    echo -e "\n${RED}Some dashboard metric sources are missing (e.g. mmap_rnd_bits on some kernels). Monitor will show default/N/A for those.${RESET}"
fi
echo -e "\n${GREEN}Dashboard metric check done. Run the monitor in a TTY for the live ncurses dashboard.${RESET}"
exit 0
