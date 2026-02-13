#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=================================="
echo "SLAB Allocator Demo - Startup"
echo "=================================="

# Check for duplicate services
echo ""
echo "Checking for duplicate services..."
RUNNING=$(ps aux | grep -E "(slab_monitor|allocator_benchmark)" | grep -v grep | wc -l)
if [ "$RUNNING" -gt 0 ]; then
    echo "⚠ Warning: Found $RUNNING running service(s)"
    ps aux | grep -E "(slab_monitor|allocator_benchmark)" | grep -v grep
    echo ""
    read -p "Kill existing services? (y/N): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f "slab_monitor" || true
        pkill -f "allocator_benchmark" || true
        sleep 1
        echo "✓ Services stopped"
    fi
else
    echo "✓ No duplicate services running"
fi

# Ensure everything is built
echo ""
echo "Building components..."
if [ ! -f "build/allocator_benchmark" ] || [ ! -f "build/slab_monitor" ]; then
    echo "Running build..."
    ./build.sh
else
    echo "✓ Build artifacts already exist"
fi

# Run tests
echo ""
echo "Running tests..."
if [ -f "test.sh" ]; then
    ./test.sh
else
    echo "⚠ test.sh not found, skipping tests"
fi

# Validate dashboard
echo ""
echo "Validating dashboard..."
if [ -f "validate_dashboard.sh" ]; then
    ./validate_dashboard.sh
else
    echo "⚠ validate_dashboard.sh not found, skipping validation"
fi

# Summary
echo ""
echo "=================================="
echo "Startup Complete!"
echo "=================================="
echo ""
echo "Available commands:"
echo "  ./build/allocator_benchmark    - Run allocation benchmarks"
echo "  sudo ./build/slab_monitor      - Start real-time dashboard"
echo "  ./monitor.sh                   - Start monitor (with sudo prompt)"
echo ""
echo "Dashboard metrics:"
echo "  - Cache Name"
echo "  - Active/Total objects"
echo "  - Object size"
echo "  - Objects per slab"
echo "  - Active/Total slabs"
echo "  - Usage percentage (color-coded)"
echo ""
echo "To start the dashboard:"
echo "  sudo ./build/slab_monitor"
echo "  (Press 'q' to quit)"
echo "=================================="

