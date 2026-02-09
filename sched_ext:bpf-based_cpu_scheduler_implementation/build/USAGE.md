# sched_ext DSQ Demo Usage

## Prerequisites
- Linux kernel >= 6.11 with CONFIG_SCHED_CLASS_EXT=y
- Root privileges (for loading BPF schedulers)

## Quick Start

### 1. Load the Scheduler
```bash
sudo ./loader scx_dsq_demo.bpf.o
```

This attaches the three-priority BPF scheduler to the system.

### 2. Monitor DSQ Activity (separate terminal)
```bash
sudo ./monitor scx_dsq_demo.bpf.o
```

### 3. Generate Test Workload (separate terminal)
```bash
./workload.sh
```

## Understanding the Output

The monitor shows:
- **Dispatch Statistics**: Tasks placed into HIGH/NORMAL/LOW priority DSQs
- **Consume Statistics**: Tasks pulled from DSQs for execution
- **DSQ Type Usage**: LOCAL vs GLOBAL vs Custom DSQ usage
- **Performance Metrics**: Cache hits, IPIs, errors

## What You're Observing

1. **Task Classification**: Tasks with nice < 0 go to HIGH DSQ, 0-10 to NORMAL, >10 to LOW
2. **Priority Enforcement**: consume() pulls from HIGH→NORMAL→LOW order
3. **Cache Locality**: select_cpu() tries to keep tasks on same CPU (cache hits)
4. **Cross-CPU Cost**: Remote dispatches generate IPIs (visible in metrics)

## Customization

Edit `src/scx_dsq_demo.bpf.c` to modify:
- Priority classification logic (classify_priority function)
- DSQ selection (enqueue operation)
- Consume order (dispatch operation)
- Timeslice duration (DEFAULT_SLICE_NS)

Rebuild with `make` and reload.

## Troubleshooting

**"Failed to attach scheduler"**
- Ensure no other sched_ext scheduler is active
- Check `dmesg` for kernel errors
- Verify kernel has CONFIG_SCHED_CLASS_EXT=y

**"Failed to load BPF object"**
- Kernel too old (< 6.11)
- Missing BPF features (check /boot/config)
- Run with `--verbose` for detailed errors

**High dispatch errors**
- DSQ IDs may be invalid
- Resource exhaustion (check `ulimit`)
- BPF verifier issues (check dmesg)

## Advanced Usage

### Trace BPF Operations
```bash
sudo bpftrace -e 'kfunc:scx_bpf_dispatch { @[args->dsq_id] = count(); }'
```

### Check Kernel DSQ Stats
```bash
cat /sys/kernel/debug/sched/ext
```

### Perf Analysis
```bash
sudo perf stat -e sched:sched_switch,sched:sched_wakeup,irq:irq_handler_entry \
    sleep 10
```

## Cleanup

1. Stop workload: Ctrl+C in workload terminal
2. Stop monitor: Ctrl+C in monitor terminal  
3. Detach scheduler: Ctrl+C in loader terminal

The system returns to CFS (default scheduler) automatically.
