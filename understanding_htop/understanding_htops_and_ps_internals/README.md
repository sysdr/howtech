# Process Monitoring Deep Dive

This project demonstrates how htop and ps actually work by building a minimal process monitor from scratch.

## Quick Start

Just run the demo:

```bash
./demo.sh
```

This will:
1. Build all source code
2. Start a stress test workload
3. Launch the custom monitor
4. Show /proc filesystem inspection

## What's Included

- **Custom Monitor**: Real-time process monitoring with ncurses
- **Process Info Parser**: Reads and parses /proc/[pid]/* files
- **Stress Test**: Creates various process states (CPU, I/O, zombies)
- **Proc Inspector**: Shows raw /proc data

## Components

### monitor
Real-time process monitor that reads /proc and displays:
- CPU usage (user/sys split)
- Memory (VSZ/RSS)
- Thread count
- File descriptor count
- Process state

### stress
Generates test workload:
```bash
./build/stress <cpu_threads> <io_threads> <zombies>
```

### proc_inspector
Shows raw /proc data for a PID:
```bash
./build/proc_inspector <pid>
```

## Building

```bash
make
```

## Understanding the Code

1. **process_info.c**: Parses /proc/[pid]/stat and /proc/[pid]/status
2. **monitor.c**: ncurses display with CPU% calculation
3. **stress.c**: Multi-threaded workload generator

Key concepts demonstrated:
- /proc filesystem structure
- CPU time in clock ticks (jiffies)
- Memory metrics: VSZ vs RSS
- Process states and transitions
- File descriptor tracking
- Thread vs process monitoring

## Cleanup

```bash
./cleanup.sh
```
