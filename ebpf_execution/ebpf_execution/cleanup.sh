#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Cleaning up eBPF JIT Demo...${NC}\n"

# Remove build artifacts
echo "Removing build artifacts..."
rm -rf build/
rm -rf output/
rm -rf src/
rm -f Makefile
rm -f Dockerfile

# Stop any running Docker containers
if command -v docker &> /dev/null; then
    containers=$(docker ps -a --filter "ancestor=ebpf-jit-demo" -q)
    if [ -n "$containers" ]; then
        echo "Stopping Docker containers..."
        docker stop $containers 2>/dev/null || true
        docker rm $containers 2>/dev/null || true
    fi
    
    # Remove Docker image
    if docker images | grep -q "ebpf-jit-demo"; then
        echo "Removing Docker image..."
        docker rmi ebpf-jit-demo 2>/dev/null || true
    fi
fi

echo -e "\n${GREEN}✓ Cleanup complete!${NC}"
echo "Preserved: demo.sh, cleanup.sh, article.md, diagrams"
