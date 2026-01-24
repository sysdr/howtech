# eBPF Network Event Correlation Demo

This demo shows how to use eBPF maps to correlate network events with process context atomically in kernel space.

## What This Demonstrates

- **eBPF fentry hooks** on tcp_connect, tcp_sendmsg, tcp_recvmsg, tcp_close
- **BPF hash maps** storing socket → process correlation
- **BPF ring buffers** for efficient event streaming to userspace
- **BTF & CO-RE** for portable eBPF across kernel versions
- **Automatic cleanup** to prevent memory leaks (864M entries/day at 10k conn/sec)
- **Edge case handling** for fork/exec and connection reuse

## Quick Start

```bash
./demo.sh
```

The script will:
1. Generate all source code
2. Build using Docker
3. Load eBPF programs
4. Run a test HTTP client
5. Show live event correlation

## Files Generated

- `netcorr.bpf.c` - eBPF kernel program
- `netcorr_user.c` - Userspace loader using libbpf
- `test_client.c` - HTTP client for testing
- `monitor.c` - ncurses dashboard showing map contents
- `Makefile` - Build configuration
- `Dockerfile` - Reproducible build environment

## Key Concepts

### Map-Based Correlation
- Store process context at socket creation
- Look up context on every send/recv (20-30ns)
- Clean up on connection close

### Modern eBPF Stack
- libbpf (not BCC)
- fentry/fexit (not kprobes)
- Ring buffers (not perf buffers)
- BTF for portability

### Production Considerations
- LRU maps for automatic eviction
- Per-CPU maps for high-frequency events
- Verifier-friendly code patterns
- Proper error handling

## Requirements

- Linux kernel 5.8+ (for BPF ring buffers)
- BTF enabled (check: ls /sys/kernel/btf/vmlinux)
- CAP_BPF + CAP_PERFMON (or CAP_SYS_ADMIN on older kernels)

## Cleanup

```bash
./cleanup.sh
```
