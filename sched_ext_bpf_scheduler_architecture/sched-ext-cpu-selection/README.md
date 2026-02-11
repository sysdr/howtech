# sched_ext CPU Selection with Dynamic Grouping

Complete working implementation of a custom Linux scheduler using sched_ext (kernel 6.12+).

## Quick Start
```bash
# Build Docker container
docker build -t sched-ext-demo .

# Run container with privileges
docker run --rm -it --privileged \
    --pid=host --cgroupns=host \
    -v /sys/kernel/debug:/sys/kernel/debug:rw \
    -v /sys/fs/bpf:/sys/fs/bpf:rw \
    --name sched-ext-demo \
    sched-ext-demo

# Inside container, run demo
cd /workspace
./scripts/run_demo.sh

# In another terminal, check scheduler status
docker exec -it sched-ext-demo bpftool prog show

# Cleanup when done
./scripts/cleanup.sh
```

## What This Demonstrates

- **CPU Selection Logic**: Tasks assigned to cache-warm CPUs when possible
- **Dynamic Grouping**: Interactive tasks (group 0) get low latency, batch tasks (group 1) get throughput
- **Wakeup Latency**: Measures time from wakeup to execution
- **Cache Impact**: Shows performance difference based on cache hierarchy

## Files

- `src/scheduler.bpf.c` - BPF scheduler implementation
- `src/monitor.c` - Real-time ncurses monitor
- `src/interactive_workload.c` - Low-latency workload simulator
- `src/batch_workload.c` - Throughput workload simulator
- `src/group_manager.c` - Assigns PIDs to scheduler groups

## Requirements

- Linux kernel 6.12 or newer
- Docker with `--privileged` mode
- BPF/BTF support enabled

## Expected Results

**Interactive Tasks (Group 0)**:
- P99 latency: <800μs
- Cache miss rate: <8%
- Preferential CPU placement

**Batch Tasks (Group 1)**:
- Throughput optimized
- Uses available CPU capacity
- Higher cache miss rate acceptable

## Verification
```bash
# View BPF maps
bpftool map dump name group_map
bpftool map dump name cpu_load

# Trace scheduler events
bpftrace -e 'tracepoint:sched:sched_wakeup { printf("%s woken\n", comm); }'

# Measure context switches
perf stat -e context-switches,cache-misses -a sleep 10
```

## Architecture

The scheduler makes decisions in `select_cpu()`:
1. Lookup task group from BPF hash map
2. Check if last CPU is cache-warm and lightly loaded
3. If yes, dispatch there (preserves cache)
4. Otherwise, find least loaded CPU
5. Track statistics for monitoring

## License

GPL-2.0
