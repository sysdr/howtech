#!/bin/bash

# Syscall Deep Dive - Cleanup Script
# Removes all generated files, build artifacts, and Docker resources

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}Cleaning up syscall demo artifacts...${NC}\n"

# Remove compiled binaries
for binary in syscall_demo monitor syscall_demo_debug; do
    if [ -f "$binary" ]; then
        rm -f "$binary"
        echo -e "${GREEN}✓${NC} Removed $binary"
    fi
done

# Remove object files and build artifacts
if ls *.o 2>/dev/null | grep -q .; then
    rm -f *.o
    echo -e "${GREEN}✓${NC} Removed object files"
fi

# Remove other build artifacts
rm -f *.out core.* a.out

# Remove Docker containers (if any)
if command -v docker &> /dev/null; then
    # Stop and remove any running/stopped containers
    CONTAINERS=$(docker ps -a --filter "name=syscall" --format "{{.ID}}" 2>/dev/null)
    if [ -n "$CONTAINERS" ]; then
        echo "$CONTAINERS" | xargs docker rm -f 2>/dev/null
        echo -e "${GREEN}✓${NC} Removed Docker containers"
    fi
    
    # Remove Docker image if it exists
    if docker images --format "{{.Repository}}" 2>/dev/null | grep -q "^syscall-demo$"; then
        docker rmi syscall-demo 2>/dev/null
        echo -e "${GREEN}✓${NC} Removed Docker image: syscall-demo"
    fi
fi

# Remove build log files
if [ -f "/tmp/docker_build.log" ]; then
    rm -f /tmp/docker_build.log
    echo -e "${GREEN}✓${NC} Removed Docker build log"
fi

# Note: Source files (syscall_demo.c, monitor.c), Makefile, and Dockerfile are preserved
# as they are part of the project repository

echo -e "\n${GREEN}Cleanup complete!${NC}"
echo -e "${YELLOW}Note:${NC} Source files, Makefile, and Dockerfile preserved.\n"
echo -e "Demo files removed. Article and diagrams preserved.\n"