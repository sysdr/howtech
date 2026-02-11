#!/bin/bash
# Cleanup script

echo "Cleaning up..."

# Kill all workloads and monitor
killall -9 monitor interactive_workload batch_workload 2>/dev/null || true

# Unload BPF programs
bpftool struct_ops unregister name cpu_select 2>/dev/null || true

# Stop Docker container
docker stop sched-ext-demo 2>/dev/null || true
docker rm sched-ext-demo 2>/dev/null || true

echo "Cleanup complete"
