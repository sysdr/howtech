# SLAB Allocator Demo

Demonstrates SLAB/SLUB/SLOB kernel cache allocators with working code and real-time monitoring.

## Quick Start
```bash
./demo.sh          # Builds everything
./build.sh         # Manual build
./monitor.sh       # Start real-time monitor
```

## Components

### 1. Kernel Module (`src/slab_demo.c`)
- Creates custom slab cache
- Benchmarks allocation/deallocation
- View with: `cat /proc/slab_demo`

### 2. Userspace Benchmark (`build/allocator_benchmark`)
- Compares allocation patterns
- Shows relevant slabinfo entries
- Demonstrates cycles per allocation

### 3. Real-time Monitor (`build/slab_monitor`)
- ncurses-based live monitoring
- Shows cache usage, memory pressure
- Color-coded utilization

## Usage

### Load kernel module:
```bash
cd src
sudo insmod slab_demo.ko
cat /proc/slab_demo
grep slab_demo /proc/slabinfo
sudo rmmod slab_demo
```

### Run benchmark:
```bash
./build/allocator_benchmark
```

### Monitor live:
```bash
sudo ./build/slab_monitor
```

## What to Observe

1. **Per-CPU Caching**: SLUB uses per-CPU freelists (fast path)
2. **Memory Overhead**: Compare active_objs vs num_objs
3. **Allocation Speed**: Cycles per allocation (RDTSC measurements)
4. **Cache Pressure**: High usage percentages indicate memory pressure

## Key Concepts

- **SLAB**: Deprecated, complex queue management
- **SLUB**: Default, simplified with per-CPU optimization
- **SLOB**: Removed in kernel 6.4 (use SLUB_TINY instead)

## Files
```
slab-demo/
├── src/
│   ├── slab_demo.c           # Kernel module
│   ├── allocator_benchmark.c # Userspace benchmark
│   └── Makefile
├── monitor/
│   └── slab_monitor.c        # Real-time monitor
├── build/                    # Compiled binaries
├── build.sh                  # Build script
├── monitor.sh                # Monitor launcher
└── README.md
```

## Requirements

- Linux kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- Build tools: `build-essential`
- ncurses: `libncurses-dev`
- Root access for module loading and /proc/slabinfo

## Debugging

View all slab caches:
```bash
cat /proc/slabinfo
```

View specific cache details:
```bash
sudo cat /sys/kernel/slab/kmalloc-512/*
```

Trace allocations:
```bash
sudo perf probe kmem_cache_alloc
sudo perf record -e probe:kmem_cache_alloc -a sleep 5
sudo perf script
```
