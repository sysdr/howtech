#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

PID=$1

if [ -z "$PID" ]; then
    echo "Usage: $0 <pid>"
    exit 1
fi

echo -e "${BLUE}=== Demonstrating Tracepoints, KProbes, and UProbes ===${NC}\n"

# Check for debugfs/tracefs
if [ ! -d /sys/kernel/debug/tracing ]; then
    echo -e "${RED}Error: /sys/kernel/debug/tracing not available${NC}"
    echo "Try: sudo mount -t debugfs none /sys/kernel/debug"
    exit 1
fi

TRACE_DIR="/sys/kernel/debug/tracing"

# Clean up any existing probes
echo > $TRACE_DIR/kprobe_events 2>/dev/null || true
echo > $TRACE_DIR/uprobe_events 2>/dev/null || true

echo -e "${GREEN}[1] Listing available tracepoints (first 20)${NC}"
perf list tracepoint 2>/dev/null | head -20
echo ""

echo -e "${GREEN}[2] Setting up KProbe on do_sys_openat2${NC}"
# Check if function exists in kallsyms
if grep -q "do_sys_openat2" /proc/kallsyms; then
    echo 'p:myprobe_open do_sys_openat2' > $TRACE_DIR/kprobe_events
    echo "KProbe created: myprobe_open on do_sys_openat2"
else
    echo "Function do_sys_openat2 not found, trying do_sys_open"
    echo 'p:myprobe_open do_sys_open' > $TRACE_DIR/kprobe_events
fi
cat $TRACE_DIR/kprobe_events
echo ""

echo -e "${GREEN}[3] Setting up UProbe on my_function${NC}"
# Find the target binary
TARGET_BIN="./build/target_program"
if [ -f "$TARGET_BIN" ]; then
    # Get the offset of my_function using nm
    OFFSET=$(nm $TARGET_BIN | grep my_function | awk '{print $1}')
    if [ ! -z "$OFFSET" ]; then
        echo "p:myprobe_func $TARGET_BIN:0x$OFFSET" > $TRACE_DIR/uprobe_events
        echo "UProbe created: myprobe_func on my_function at offset 0x$OFFSET"
        cat $TRACE_DIR/uprobe_events
    else
        echo "Could not find my_function symbol"
    fi
else
    echo "Target binary not found at $TARGET_BIN"
fi
echo ""

echo -e "${GREEN}[4] Tracing syscall tracepoints for PID $PID (5 seconds)${NC}"
echo -e "${YELLOW}Watching: sys_enter_openat, sys_exit_openat${NC}"
timeout 5 perf trace -e 'syscalls:sys_enter_openat,syscalls:sys_exit_openat' -p $PID 2>&1 | head -20 || true
echo ""

echo -e "${GREEN}[5] Tracing our kprobe (5 seconds)${NC}"
timeout 5 perf trace -e 'probe:myprobe_open' -p $PID 2>&1 | head -10 || true
echo ""

echo -e "${GREEN}[6] Tracing our uprobe (5 seconds)${NC}"
timeout 5 perf trace -e 'probe:myprobe_func' -p $PID 2>&1 | head -10 || true
echo ""

echo -e "${GREEN}[7] Comparing overhead - tracepoint vs kprobe vs uprobe${NC}"
echo "Running each for 3 seconds and counting events..."

# Count tracepoint events
TRACEPOINT_COUNT=$(timeout 3 perf stat -e 'syscalls:sys_enter_openat' -p $PID 2>&1 | grep sys_enter_openat | awk '{print $1}' | tr -d ',')

# Count kprobe events  
KPROBE_COUNT=$(timeout 3 perf stat -e 'probe:myprobe_open' -p $PID 2>&1 | grep myprobe_open | awk '{print $1}' | tr -d ',')

# Count uprobe events
UPROBE_COUNT=$(timeout 3 perf stat -e 'probe:myprobe_func' -p $PID 2>&1 | grep myprobe_func | awk '{print $1}' | tr -d ',')

echo "Tracepoint events: ${TRACEPOINT_COUNT:-0}"
echo "KProbe events: ${KPROBE_COUNT:-0}"
echo "UProbe events: ${UPROBE_COUNT:-0}"
echo ""

echo -e "${BLUE}Demo complete!${NC}"

# Cleanup
echo > $TRACE_DIR/kprobe_events 2>/dev/null || true
echo > $TRACE_DIR/uprobe_events 2>/dev/null || true
