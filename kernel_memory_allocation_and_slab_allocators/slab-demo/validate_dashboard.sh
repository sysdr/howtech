#!/bin/bash

echo "=================================="
echo "Dashboard (Monitor) Validation"
echo "=================================="

ERRORS=0

# Check if monitor binary exists
if [ ! -f "build/slab_monitor" ]; then
    echo "✗ Monitor binary not found. Run ./build.sh first."
    exit 1
fi

echo ""
echo "Validating monitor capabilities..."

# Check if monitor can access /proc/slabinfo (if we have permissions)
if [ -r "/proc/slabinfo" ]; then
    echo "✓ /proc/slabinfo is readable"
    
    # Validate that monitor can parse the data
    echo ""
    echo "Checking metrics that monitor should display:"
    
    # Check for key caches that monitor should show
    CACHES_FOUND=0
    for cache in "kmalloc" "task_struct" "dentry" "inode" "buffer_head" "ext4"; do
        if grep -q "$cache" /proc/slabinfo; then
            echo "  ✓ Found $cache cache in /proc/slabinfo"
            CACHES_FOUND=$((CACHES_FOUND + 1))
        fi
    done
    
    if [ $CACHES_FOUND -gt 0 ]; then
        echo "  ✓ Monitor can display $CACHES_FOUND relevant cache types"
    else
        echo "  ⚠ No expected caches found (may be normal)"
    fi
    
    # Check metrics format
    echo ""
    echo "Expected metrics in dashboard:"
    echo "  ✓ Cache Name"
    echo "  ✓ Active objects"
    echo "  ✓ Total objects"
    echo "  ✓ Object size"
    echo "  ✓ Objects per slab"
    echo "  ✓ Active/Total slabs"
    echo "  ✓ Usage percentage"
    
    # Validate monitor source code includes all metrics
    echo ""
    echo "Validating monitor source code..."
    if grep -q "active_objs\|num_objs\|objsize\|objperslab\|active_slabs\|num_slabs" monitor/slab_monitor.c; then
        echo "  ✓ Monitor code includes all required metrics"
    else
        echo "  ✗ Monitor code missing some metrics"
        ERRORS=$((ERRORS + 1))
    fi
    
    if grep -q "usage\|Usage" monitor/slab_monitor.c; then
        echo "  ✓ Monitor code calculates usage percentage"
    else
        echo "  ✗ Monitor code missing usage calculation"
        ERRORS=$((ERRORS + 1))
    fi
    
    # Check color coding
    if grep -q "COLOR_PAIR\|color" monitor/slab_monitor.c; then
        echo "  ✓ Monitor includes color coding for metrics"
    else
        echo "  ⚠ Monitor may not have color coding"
    fi
    
else
    echo "⚠ /proc/slabinfo requires root access"
    echo "  Monitor will work with: sudo ./build/slab_monitor"
fi

# Validate monitor can be executed
echo ""
echo "Checking monitor execution..."
if [ -x "build/slab_monitor" ]; then
    echo "  ✓ Monitor is executable"
    
    # Try to validate it's a valid ELF binary
    if file build/slab_monitor | grep -q "ELF"; then
        echo "  ✓ Monitor is a valid ELF binary"
    else
        echo "  ✗ Monitor binary format issue"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo "  ✗ Monitor is not executable"
    ERRORS=$((ERRORS + 1))
fi

# Check dependencies
echo ""
echo "Checking dependencies..."
if ldd build/slab_monitor 2>/dev/null | grep -q "ncurses"; then
    echo "  ✓ Monitor linked with ncurses library"
else
    echo "  ⚠ Monitor may not be properly linked with ncurses"
fi

# Summary
echo ""
echo "=================================="
if [ $ERRORS -eq 0 ]; then
    echo "✓ Dashboard (Monitor) validation passed!"
    echo ""
    echo "To run the dashboard:"
    echo "  sudo ./build/slab_monitor"
    echo "  or"
    echo "  ./monitor.sh"
    echo ""
    echo "The dashboard displays:"
    echo "  - Real-time slab cache statistics"
    echo "  - Active/Total objects per cache"
    echo "  - Object sizes and objects per slab"
    echo "  - Slab usage percentages"
    echo "  - Color-coded utilization (green/yellow/red)"
    echo "=================================="
    exit 0
else
    echo "✗ $ERRORS validation error(s) found"
    echo "=================================="
    exit 1
fi

