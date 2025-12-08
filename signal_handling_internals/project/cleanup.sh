#!/bin/bash
set -euo pipefail

echo "Cleaning up build artifacts..."

# Remove build directory
if [ -d "build" ]; then
    rm -rf build
    echo "✓ Removed build directory"
fi

# Optionally remove source files (commented out by default)
# if [ -d "src" ]; then
#     rm -rf src
#     echo "✓ Removed src directory"
# fi

# Remove Makefile and Dockerfile if desired (commented out by default)
# rm -f Makefile Dockerfile

echo "Cleanup complete!"
