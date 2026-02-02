#!/bin/bash
echo "Cleaning up CFS Scheduler demo files..."
rm -rf build output
make clean 2>/dev/null || true
echo "Cleanup complete."
