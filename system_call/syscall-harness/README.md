# System Call Test Harness

Instrumented test framework for measuring syscall performance with cycle-accurate timing.

## Quick Start
```bash
./demo.sh
```

## Components

- **syscall_test**: Multi-threaded syscall workload with RDTSC timing
- **monitor**: Real-time ncurses display of syscall statistics

## Features

- RDTSC/RDTSCP cycle-accurate timing with proper serialization
- Atomic counters for thread-safe measurement
- Logarithmic histogram bucketing
- Per-syscall statistics aggregation

## Build Manually
```bash
make all
./build/syscall_test
./build/monitor  # In separate terminal
```

