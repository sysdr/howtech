#!/bin/bash

echo "Cleaning up multi-level scheduler demo..."

# Remove built binaries
rm -f multi_level_sched.bpf.o
rm -f mlq_loader mlq_monitor workload_gen

# Remove generated headers
rm -f include/vmlinux.h

# Remove build artifacts
rm -rf build/*

# Remove Docker containers if any
docker ps -a | grep mlq_sched | awk '{print $1}' | xargs -r docker rm -f 2>/dev/null

# Remove Docker images
docker images | grep mlq_sched | awk '{print $3}' | xargs -r docker rmi -f 2>/dev/null

echo "Cleanup complete!"
