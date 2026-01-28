# eBPF JIT Compilation Deep Dive

This demonstration shows the performance impact of eBPF JIT compilation vs bytecode interpretation in the Linux kernel.

## Quick Start

```bash
sudo ./demo.sh
```

That's it! The script will:
- Install all dependencies
- Build eBPF programs and tools
- Run performance benchmarks
- Launch real-time monitoring dashboard

## What Gets Demonstrated

1. **Performance Benchmark**: Direct comparison of interpreter vs JIT execution
2. **Real-time Monitoring**: Live view of JIT status and performance metrics
3. **Code Inspection**: View actual JIT-compiled x86-64 assembly

## Manual Usage

### Run Benchmark Only
```bash
sudo ./build/loader
```

### Launch Monitor
```bash
sudo ./build/monitor
```

### Toggle JIT Compilation
```bash
# Disable JIT (use interpreter)
echo 0 | sudo tee /proc/sys/net/core/bpf_jit_enable

# Enable JIT
echo 1 | sudo tee /proc/sys/net/core/bpf_jit_enable
```

### Inspect JIT Code
```bash
# List loaded programs
sudo bpftool prog list

# Dump JIT assembly
sudo bpftool prog dump jited id <ID>
```

## Expected Results

| Metric | Interpreter | JIT | Speedup |
|--------|-------------|-----|---------|
| Throughput | 2-3M ops/sec | 10-14M ops/sec | 4-5x |
| Dispatch Overhead | ~250ns | ~5ns | 50x |
| CPU Usage | High | Low | 40-60% reduction |

## Requirements

- Linux kernel 4.14+ (5.x recommended)
- Root/sudo access for eBPF operations
- x86-64 or ARM64 architecture

## Cleanup

```bash
sudo ./cleanup.sh
```

Removes all build artifacts and temporary files.

## Architecture

```
User Space
    ├── loader (C + libbpf)
    └── monitor (ncurses)
        │
        ↓ bpf() syscall
        │
Kernel Space
    ├── Verifier (safety checks)
    ├── JIT Compiler (arch/x86/net/bpf_jit_comp.c)
    └── Execution (XDP, kprobe, tc)
```

## Learn More

- Article: See article.md for deep technical explanation
- Diagrams: Visual representations of JIT compilation flow
- Source: All code in src/ with detailed comments

