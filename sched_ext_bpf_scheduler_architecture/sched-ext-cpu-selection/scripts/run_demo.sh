#!/bin/bash
# Run the complete sched_ext demo

set -e

echo "=== sched_ext CPU Selection Demo ==="
echo ""

# Load BPF scheduler
echo "[1/5] Loading BPF scheduler..."
bpftool struct_ops register src/scheduler.bpf.o

# Get map FDs
echo "[2/5] Getting BPF map file descriptors..."
GROUP_MAP_ID=$(bpftool map show | grep -w group_map | awk '{print $1}' | tr -d ':')
EVENTS_MAP_ID=$(bpftool map show | grep -w events | awk '{print $1}' | tr -d ':')

if [ -z "$GROUP_MAP_ID" ] || [ -z "$EVENTS_MAP_ID" ]; then
    echo "Error: Could not find BPF maps"
    echo "Available maps:"
    bpftool map show
    exit 1
fi

# Get map FDs using helper program
if [ -f "./get_map_fd" ]; then
    GROUP_MAP_FD=$(./get_map_fd $GROUP_MAP_ID)
    EVENTS_MAP_FD=$(./get_map_fd $EVENTS_MAP_ID)
else
    echo "Warning: get_map_fd helper not found, using map IDs (may not work)"
    GROUP_MAP_FD=$GROUP_MAP_ID
    EVENTS_MAP_FD=$EVENTS_MAP_ID
fi

echo "Group map FD: $GROUP_MAP_FD"
echo "Events map FD: $EVENTS_MAP_FD"

# Start monitor in background
echo "[3/5] Starting monitor..."
./monitor $EVENTS_MAP_FD &
MONITOR_PID=$!
sleep 2

# Start interactive workloads
echo "[4/5] Starting interactive workloads (group 0)..."
for i in {1..4}; do
    ./interactive_workload &
    WORK_PID=$!
    sleep 0.5
    ./group_manager $GROUP_MAP_FD $WORK_PID 0
done

# Start batch workloads
echo "[5/5] Starting batch workloads (group 1)..."
for i in {1..4}; do
    ./batch_workload &
    WORK_PID=$!
    sleep 0.5
    ./group_manager $GROUP_MAP_FD $WORK_PID 1
done

echo ""
echo "=== Demo Running ==="
echo "Monitor PID: $MONITOR_PID"
echo "Press Ctrl+C to stop"
echo ""

# Wait for monitor
wait $MONITOR_PID
