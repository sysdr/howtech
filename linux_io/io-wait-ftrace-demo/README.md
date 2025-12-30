# I/O Wait Latency Analysis with Ftrace

Demonstrates analyzing I/O wait latency using Linux ftrace and block layer tracing.

## Quick Start

```bash
sudo ./demo.sh
```

This script:
1. Compiles all tools
2. Sets up ftrace block tracing
3. Runs I/O workload
4. Captures and analyzes trace data
5. Displays latency statistics

## Manual Usage

```bash
# Build
make

# Enable tracing (requires root)
sudo build/ftrace_controller &

# Run workload
sudo build/workload

# Stop tracing (Ctrl+C the controller)

# Analyze results
sudo build/analyzer traces/trace.out

# Real-time monitor (optional)
sudo build/monitor
```

## Requirements

- Linux kernel with CONFIG_TRACING=y
- Root privileges (for ftrace access)
- ncurses library

## What It Demonstrates

- Block layer tracepoints (block_rq_issue, block_rq_complete)
- Queue latency vs service time breakdown
- O_DIRECT vs buffered I/O behavior
- Effect of fsync() on I/O patterns
- Percentile latency distributions

## Trace Events

- **block_rq_issue**: Request dispatched to device driver (D event)
- **block_rq_complete**: Request completed by hardware (C event)
- **block_bio_queue**: BIO queued in block layer (Q event)

Latency = timestamp(complete) - timestamp(issue)
