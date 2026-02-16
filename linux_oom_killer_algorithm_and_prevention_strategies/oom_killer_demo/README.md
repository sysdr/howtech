# Linux OOM Killer — Algorithm Deep Dive Demo

This demo provides a comprehensive exploration of the Linux Out-of-Memory (OOM) Killer algorithm, including heuristics, scoring mechanisms, and prevention strategies.

## Contents

- **oom_demo.c**: Interactive demonstration of OOM scoring, RSS growth effects, and PSI monitoring
- **oom_monitor.c**: Real-time ncurses-based monitor showing top processes by OOM score
- **Dockerfile**: Containerized environment for running the demos

## Quick Start

```bash
# Run the setup script
./setup.sh

# Run the demo
cd oom_killer_demo && ./bin/oom_demo

# Run the interactive monitor
cd oom_killer_demo && ./bin/oom_monitor
```

## Building

```bash
cd oom_killer_demo
make
```

## Docker Usage

```bash
# Build the image
docker build -t oom-killer-demo:latest oom_killer_demo/

# Run the demo
docker run --rm --cap-add SYS_RESOURCE --cap-add DAC_READ_SEARCH \
    oom-killer-demo:latest /app/bin/oom_demo

# Run monitor snapshot
docker run --rm --cap-add SYS_RESOURCE --cap-add DAC_READ_SEARCH \
    oom-killer-demo:latest /app/bin/oom_monitor --snapshot
```

## Features

- OOM score calculation demonstration
- RSS growth impact on scoring
- oom_score_adj manipulation
- PSI (Pressure Stall Information) monitoring
- Memory commit tracking
- Top processes by OOM score

## Requirements

- Linux kernel 4.20+ (for PSI support)
- Docker (for containerized demos)
- ncurses library (for interactive monitor)
- GCC compiler

## License

Educational use — Systems Programming Deep Dive Newsletter
