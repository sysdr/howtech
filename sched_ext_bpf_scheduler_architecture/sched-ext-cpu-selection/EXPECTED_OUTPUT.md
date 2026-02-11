# Expected Demo Output

## Console Output from run_demo.sh

```
=== sched_ext CPU Selection Demo ===

[1/5] Loading BPF scheduler...
[2/5] Getting BPF map file descriptors...
Group map FD: 123
Events map FD: 124
[3/5] Starting monitor...
[4/5] Starting interactive workloads (group 0)...
[Interactive] PID 1234 started (group 0 - low latency)
[Interactive] Pattern: sleep 10ms, work 50μs, repeat
[Interactive] PID 1235 started (group 0 - low latency)
[Interactive] Pattern: sleep 10ms, work 50μs, repeat
[Interactive] PID 1236 started (group 0 - low latency)
[Interactive] Pattern: sleep 10ms, work 50μs, repeat
[Interactive] PID 1237 started (group 0 - low latency)
[Interactive] Pattern: sleep 10ms, work 50μs, repeat
[5/5] Starting batch workloads (group 1)...
[Batch] PID 1238 started (group 1 - throughput)
[Batch] Pattern: continuous computation
[Batch] PID 1239 started (group 1 - throughput)
[Batch] Pattern: continuous computation
[Batch] PID 1240 started (group 1 - throughput)
[Batch] Pattern: continuous computation
[Batch] PID 1241 started (group 1 - throughput)
[Batch] Pattern: continuous computation

=== Demo Running ===
Monitor PID: 1233
Press Ctrl+C to stop
```

## Real-Time Monitor Display (ncurses)

The monitor displays a real-time ncurses interface that looks like:

```
╔════════════════════════════════════════════════════════════════════════════╗
║        sched_ext CPU Selection Monitor - Real-Time Statistics             ║
╚════════════════════════════════════════════════════════════════════════════╝

Group 0: Interactive (Low Latency)
  Wakeup Count:  15234
  Avg Latency:   245 μs
  Min Latency:   12 μs
  Max Latency:   1850 μs
  P50 Latency:   198 μs
  P95 Latency:   512 μs
  P99 Latency:   756 μs

Group 1: Batch (Throughput)
  Wakeup Count:  8234
  Avg Latency:   1245 μs
  Min Latency:   45 μs
  Max Latency:   3420 μs
  P50 Latency:   1123 μs
  P95 Latency:   2156 μs
  P99 Latency:   2890 μs

Cache Hierarchy Impact:
  L1/L2 (Same Core):    4-12 cycles   [Optimal]
  L3 (Same Socket):     40-50 cycles  [Acceptable]
  RAM (Cross-Socket):   200+ cycles   [Avoid]

Press Ctrl+C to exit
Statistics update every 100ms
```

## Expected Performance Metrics

### Interactive Tasks (Group 0):
- **P99 latency**: <800μs (green color in monitor)
- **Cache miss rate**: <8%
- **CPU placement**: Preferential to cache-warm CPUs
- **Behavior**: Tasks wake up every 10ms, do small work bursts

### Batch Tasks (Group 1):
- **Throughput optimized**: Higher latency acceptable
- **CPU usage**: Maximizes available CPU capacity
- **Cache miss rate**: Higher acceptable (throughput over latency)
- **Behavior**: Continuous computation, CPU-intensive

## Final Statistics (on exit)

```
=== Final Statistics ===

Group 0:
  Wakeup Count: 152340
  Avg Latency:  245 μs
  Min Latency:  12 μs
  Max Latency:  1850 μs
  P99 Latency:  756 μs

Group 1:
  Wakeup Count: 82340
  Avg Latency:  1245 μs
  Min Latency:  45 μs
  Max Latency:  3420 μs
  P99 Latency:  2890 μs
```

## What the Scheduler Does

1. **CPU Selection**: When a task wakes up, the scheduler:
   - Checks if the task's last CPU is cache-warm and lightly loaded
   - If yes, dispatches to that CPU (preserves cache locality)
   - Otherwise, finds the least loaded CPU

2. **Group-Based Scheduling**:
   - Group 0 (Interactive): Lower load threshold (10), aggressive cache preference
   - Group 1 (Batch): Higher load threshold (5), throughput focused

3. **Latency Measurement**: Tracks time from wakeup (enqueue) to execution (running)

4. **Real-Time Monitoring**: Updates statistics every 100ms via ring buffer

