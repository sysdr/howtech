# Buddy System Allocator Demo

## Quick Start
```bash
./demo.sh
```

This will build and run the buddy system monitor.

## What It Shows

- Real-time visualization of `/proc/buddyinfo`
- Per-order block counts across all memory zones
- Fragmentation index calculation
- Color-coded display showing memory health

## Controls

- `q`: Quit
- `a`: Allocate 100MB test memory
- `f`: Free test memory

## Manual Testing

1. Run monitor in one terminal: `./buddy_monitor`
2. Run stress test in another: `./alloc_stress 1000`
3. Watch fragmentation increase in real-time

## Understanding the Output

- **Order 0-2**: Small allocations (4KB-16KB)
- **Order 3-6**: Medium allocations (32KB-256KB)
- **Order 7-10**: Large/huge page allocations (512KB-4MB)

Green values (order 3+) indicate healthy reserves.
Red zeros mean that order is depleted.

High fragmentation index (>80%) means many small blocks, few large ones.
