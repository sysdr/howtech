#!/bin/bash

set -e

echo "=================================="
echo "SLAB Allocator Demo"
echo "=================================="

# Build everything
if [ ! -f "build.sh" ]; then
    echo "Error: build.sh not found. Run setup.sh first."
    exit 1
fi

./build.sh

echo ""
echo "Demo ready! Available commands:"
echo "  ./build/allocator_benchmark    - Run allocation benchmarks"
echo "  sudo ./build/slab_monitor       - Monitor slab caches in real-time"
if [ -f "src/slab_demo.ko" ]; then
    echo "  sudo insmod src/slab_demo.ko   - Load kernel module"
    echo "  cat /proc/slab_demo            - View module statistics"
    echo "  sudo rmmod slab_demo           - Unload module"
fi
