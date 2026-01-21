# ASAN Demonstration Suite

This directory contains working examples demonstrating Address Sanitizer (ASAN) capabilities.

## Quick Start
```bash
# Build all examples with ASAN
make asan

# Run individual tests
export ASAN_OPTIONS=detect_leaks=1:symbolize=1:abort_on_error=1
./heap_overflow_asan
./use_after_free_asan
./stack_overflow_asan
./memory_leak_asan
./double_free_asan

# Compare performance
./benchmark_normal
./benchmark_asan
```

## Docker Build
```bash
docker build -t asan-demo .
docker run -it asan-demo
```

## Examples Included

1. **heap_overflow** - Detects writes past allocated buffer
2. **use_after_free** - Catches access to freed memory
3. **stack_overflow** - Finds stack buffer overruns
4. **memory_leak** - Reports unfreed allocations
5. **double_free** - Detects multiple free() calls
6. **shadow_inspect** - Shows shadow memory state
7. **benchmark** - Measures ASAN overhead
8. **monitor** - Visual test runner

## ASAN Options
```bash
export ASAN_OPTIONS=detect_leaks=1:symbolize=1:abort_on_error=1:quarantine_size_mb=256
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
```
