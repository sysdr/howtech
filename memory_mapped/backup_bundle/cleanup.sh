#!/bin/bash
set -e
echo "Removing generated binaries and data files..."
rm -f coherence_test dirty_monitor dirty_generator msync_benchmark
rm -f *.dat *.o
echo "Done."
