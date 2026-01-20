#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

echo "=== System Call Test Harness Demo ==="
echo

if [ ! -f build/syscall_test ]; then
    echo "Building syscall test harness..."
    make all
fi

echo "=== Running syscall instrumentation test ==="
echo
./build/syscall_test

echo
echo "=== Starting real-time monitor (press Ctrl+C or 'q' to stop) ==="
echo
sleep 2
./build/monitor

echo
echo "=== Demo complete! ==="

