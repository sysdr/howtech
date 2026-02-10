# Multi-Level FIFO Scheduler with sched_ext

Custom BPF-based scheduler implementing three-tier priority queues using `BPF_MAP_TYPE_QUEUE`.

## Requirements

- Linux kernel 6.6+ (for sched_ext support)
- clang/LLVM with BPF support
- libbpf-dev
- bpftool
- ncurses-dev

## Quick Start

```bash
./demo.sh
```

This will:
1. Generate all source files
2. Build BPF programs and userspace tools
3. Create Docker container for isolated builds
4. Run demonstration workload (if kernel supports sched_ext)

## Manual Build

```bash
make all
```

## Components

- `multi_level_sched.bpf.c` - BPF scheduler implementation
- `mlq_loader.c` - Userspace loader and statistics monitor
- `mlq_monitor.c` - Real-time ncurses-based monitor
- `workload_gen.c` - Test workload generator

## Priority Levels

- **HIGH**: nice < 0 (latency-sensitive tasks)
- **MEDIUM**: nice 0-10 (normal tasks)
- **LOW**: nice > 10 (batch processing)

## Monitoring

View statistics with:
```bash
./mlq_monitor <stats_map_fd>
```

## Cleanup

```bash
./cleanup.sh
```
