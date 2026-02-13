#!/bin/bash

if [ ! -f "build/slab_monitor" ]; then
    echo "Building monitor first..."
    ./build.sh
fi

echo "Starting SLAB monitor (requires root for /proc/slabinfo access)"
echo "Press Ctrl+C to exit"
echo ""

sudo ./build/slab_monitor
