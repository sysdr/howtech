#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Cleaning up generated files...${NC}\n"

# Remove build directory
if [ -d "build" ]; then
    rm -rf build
    echo -e "${GREEN}✓${NC} Removed build directory"
fi

# Remove source files (optional - uncomment if you want to remove source too)
# if [ -d "src" ]; then
#     rm -rf src
#     echo -e "${GREEN}✓${NC} Removed src directory"
# fi

# Remove generated scripts and files
files_to_remove=(
    "build.sh"
    "Dockerfile"
    "README.md"
    "demo.sh"
    "cleanup.sh"
)

for file in "${files_to_remove[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        echo -e "${GREEN}✓${NC} Removed $file"
    fi
done

# Remove output directory if it exists
if [ -d "output" ]; then
    rm -rf output
    echo -e "${GREEN}✓${NC} Removed output directory"
fi

echo -e "\n${CYAN}Cleanup complete!${NC}\n"

