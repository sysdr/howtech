# Linux OOM Killer: Algorithm Deep Dive

## Introduction

The Linux Out-of-Memory (OOM) Killer is a last-resort mechanism that terminates processes when the system runs out of memory. This article explores the algorithm's internals, scoring mechanisms, and prevention strategies.

## OOM Score Calculation

The OOM killer uses a scoring algorithm to determine which process to kill:

```
oom_score = (memory_usage * 1000) / total_memory + oom_score_adj
```

Key factors:
- **RSS (Resident Set Size)**: Physical memory currently in use
- **oom_score_adj**: Adjustment factor (-1000 to +1000)
- **Process age**: Older processes may be slightly favored
- **Root processes**: May have slight protection

## Prevention Strategies

1. **oom_score_adj tuning**: Set negative values for critical processes
2. **Memory limits**: Use cgroups to limit memory per process/container
3. **Monitoring**: Track PSI (Pressure Stall Information) for early warnings
4. **Swap configuration**: Ensure adequate swap space
5. **Overcommit settings**: Tune vm.overcommit_memory sysctl

## PSI Monitoring

Pressure Stall Information (PSI) provides early warning of memory pressure:

- **some**: At least one task stalled due to memory
- **full**: All tasks stalled (critical condition)

## Demo Programs

This repository includes:

- `oom_demo.c`: Demonstrates OOM scoring and RSS growth effects
- `oom_monitor.c`: Real-time monitoring tool with ncurses UI

Run `./setup.sh` to build and test the demos.
