#!/bin/bash

set -e

echo "Building SLAB allocator demo..."

# Build kernel module
if [ -d "/lib/modules/$(uname -r)/build" ]; then
    echo "Building kernel module..."
    cd src
    make clean 2>/dev/null || true
    make
    cd ..
    echo "✓ Kernel module built: src/slab_demo.ko"
else
    echo "⚠ Kernel headers not found, skipping kernel module build"
    echo "  Install with: sudo apt-get install linux-headers-$(uname -r)"
fi

# Build userspace programs
echo "Building userspace programs..."
gcc -Wall -Wextra -Werror -O2 -g -o build/allocator_benchmark src/allocator_benchmark.c
gcc -Wall -Wextra -Werror -O2 -g -o build/slab_monitor monitor/slab_monitor.c -lncurses

echo "✓ Build complete!"
echo ""
echo "Available commands:"
echo "  ./build/allocator_benchmark  - Run allocation benchmarks"
echo "  sudo ./build/slab_monitor    - Monitor slab caches in real-time"
if [ -f "src/slab_demo.ko" ]; then
    echo "  sudo insmod src/slab_demo.ko - Load kernel module"
    echo "  cat /proc/slab_demo          - View module statistics"
    echo "  sudo rmmod slab_demo         - Unload kernel module"
fi
