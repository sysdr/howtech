#!/usr/bin/env bash
# demo.sh — SCHED_BATCH article demo
# Run as regular user; requires: gcc, libncurses-dev, strace, procps
set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
mkdir -p "${BUILD_DIR}"

banner() { printf "\n${BOLD}${CYAN}══════════════════════════════════════════${RESET}\n${BOLD} %s${RESET}\n${BOLD}${CYAN}══════════════════════════════════════════${RESET}\n\n" "$1"; }
step()   { printf "${GREEN}▶${RESET} %s\n" "$1"; }
info()   { printf "${YELLOW}  ℹ${RESET}  %s\n" "$1"; }
die()    { printf "${RED}✗ %s${RESET}\n" "$1" >&2; exit 1; }

# ── Dependency check ────────────────────────────────────────────────
banner "Phase 0: Dependency Check"
for cmd in gcc make strace cat grep awk; do
    command -v "$cmd" >/dev/null 2>&1 || die "Required tool not found: $cmd"
    step "Found: $cmd"
done

# Optional helpers used by later phases (script will degrade gracefully if missing)
for cmd in chrt timeout tail; do
    if command -v "$cmd" >/dev/null 2>&1; then
        step "Found (optional): $cmd"
    else
        info "Optional tool not found: $cmd (some phases may be skipped)"
    fi
done

# Check for libncurses
if ! pkg-config --exists ncurses 2>/dev/null && ! dpkg -s libncurses-dev >/dev/null 2>&1; then
    info "libncurses-dev not detected — monitor build may fail"
    info "Install with: sudo apt-get install libncurses-dev"
fi

# ── Build ────────────────────────────────────────────────────────────
banner "Phase 1: Build"

# Copy sources to build dir
cp "${SCRIPT_DIR}/sched_bench.c"   "${BUILD_DIR}/"
cp "${SCRIPT_DIR}/sched_monitor.c" "${BUILD_DIR}/"
cp "${SCRIPT_DIR}/Makefile"        "${BUILD_DIR}/"

cd "${BUILD_DIR}"

step "Compiling sched_bench.c (-Wall -Wextra -Werror -O2)"
gcc -Wall -Wextra -Werror -O2 -g -o sched_bench sched_bench.c -lpthread \
    || die "sched_bench compilation failed"
step "sched_bench: OK"

step "Compiling sched_monitor.c"
if gcc -Wall -Wextra -Werror -O2 -g -o sched_monitor sched_monitor.c -lncurses 2>/dev/null; then
    step "sched_monitor: OK"
    HAVE_MONITOR=1
else
    info "sched_monitor skipped (libncurses not available)"
    HAVE_MONITOR=0
fi

# ── Phase 2: Sysctl inspection ───────────────────────────────────────
banner "Phase 2: Current Scheduler Tunables"

for knob in sched_latency_ns sched_min_granularity_ns sched_wakeup_granularity_ns; do
    val=$(cat "/proc/sys/kernel/${knob}" 2>/dev/null || echo "N/A")
    printf "  ${CYAN}/proc/sys/kernel/%-32s${RESET} = ${YELLOW}%s ns${RESET}\n" "${knob}" "${val}"
done
echo
info "sched_wakeup_granularity_ns controls minimum vruntime delta before a waking"
info "SCHED_OTHER task preempts the running task. SCHED_BATCH bypasses this check entirely."

# ── Phase 3: Run benchmark (no monitor) ─────────────────────────────
banner "Phase 3: Benchmark — SCHED_OTHER vs SCHED_BATCH"
info "Spawning 4 interactive (SCHED_OTHER) + 4 batch (SCHED_BATCH) threads"
info "Each interactive thread: 500 x 1ms nanosleep, records wakeup latency"
info "Each batch thread: 500 x 5M FNV-1a iterations, no sleep"
echo
./sched_bench

# ── Phase 4: /proc inspection ───────────────────────────────────────
banner "Phase 4: Verify Scheduler Policy via chrt"

if command -v chrt >/dev/null 2>&1; then
    # Run bench briefly in background, inspect it
    ./sched_bench &
    BENCH_PID=$!
    sleep 0.5

    if [ -d "/proc/${BENCH_PID}/task" ]; then
        step "Threads visible in /proc/${BENCH_PID}/task:"
        for tid in /proc/${BENCH_PID}/task/*/; do
            tid_num=$(basename "${tid}")
            policy_raw=$(chrt -p "${tid_num}" 2>/dev/null || echo "N/A")
            printf "    TID %-8s  %s\n" "${tid_num}" "${policy_raw}"
            # Read involuntary switches
            invol=$(grep "nr_involuntary_switches" "${tid}/sched" 2>/dev/null | awk '{print $3}' || echo "?")
            printf "               invol_switches so far: %s\n" "${invol}"
        done
    fi

    wait "${BENCH_PID}" 2>/dev/null || true
else
    info "Phase 4 skipped (chrt not available)"
fi

# ── Phase 5: strace syscall summary ─────────────────────────────────
banner "Phase 5: strace Syscall Summary"
info "Tracing sched_bench with strace -c (syscall count + time)"
echo

if command -v timeout >/dev/null 2>&1; then
    timeout 8 strace -c -f ./sched_bench 2>&1 | tail -20 || true
else
    strace -c -f ./sched_bench 2>&1 | tail -20 || true
fi

# ── Phase 6: ncurses monitor ────────────────────────────────────────
if [ "${HAVE_MONITOR}" = "1" ] && [ -t 1 ]; then
    banner "Phase 6: Live Scheduler Monitor"
    info "Starting sched_bench in background, attaching ncurses monitor"
    info "Press Q inside monitor to exit"
    sleep 1

    ./sched_bench &
    BENCH_PID=$!
    sleep 0.3

    if kill -0 "${BENCH_PID}" 2>/dev/null; then
        ./sched_monitor "${BENCH_PID}" || true
        wait "${BENCH_PID}" 2>/dev/null || true
    fi
else
    info "Phase 6: Monitor skipped (no ncurses or non-interactive terminal)"
fi

# ── Phase 7: Practical usage snippet ────────────────────────────────
banner "Phase 7: Production Usage Reference"
cat <<'EOF'
  Set SCHED_BATCH from shell:
    chrt --batch 0 ./your_batch_job

  Set SCHED_BATCH programmatically (no elevated privileges needed):
    struct sched_param sp = { .sched_priority = 0 };
    sched_setscheduler(0, SCHED_BATCH, &sp);
    setpriority(PRIO_PROCESS, 0, 5);   /* optional: also lower nice */

  Combined with CPU affinity:
    taskset -c 8-15 chrt --batch 0 ./your_batch_job

  Verify:
    chrt -p $(pgrep your_batch_job)
    grep -E "nr_involuntary|se.sum_exec|nr_voluntary" /proc/$(pgrep your_batch_job)/sched

  Read /proc/sys/kernel/sched_wakeup_granularity_ns before and after
  to confirm the knob state. SCHED_BATCH bypasses this check — you
  do not need to raise this global value to protect interactive tasks.
EOF

banner "Demo Complete"
printf "${GREEN}All phases done. See build/ for binaries.${RESET}\n"
if [ -f "${SCRIPT_DIR}/article.md" ]; then
    printf "Article: article.md\n"
fi
if [ -f "${SCRIPT_DIR}/diagram-1.jpg" ] || [ -f "${SCRIPT_DIR}/diagram-2.jpg" ]; then
    printf "Diagrams:"
    [ -f "${SCRIPT_DIR}/diagram-1.jpg" ] && printf " diagram-1.jpg"
    [ -f "${SCRIPT_DIR}/diagram-2.jpg" ] && printf " diagram-2.jpg"
    printf "\n"
fi
printf "\n"