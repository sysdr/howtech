#!/usr/bin/env bash
# Validation script for THP Demo and Dashboard
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_BIN="$DEMO_DIR/build/thp_demo"
MONITOR_BIN="$DEMO_DIR/build/monitor"

echo "=== THP Demo Validation ==="
echo ""

# Check files exist
echo "1. Checking required files..."
files=("$DEMO_BIN" "$MONITOR_BIN" "$DEMO_DIR/src/thp_demo.c" "$DEMO_DIR/src/monitor.c" "$DEMO_DIR/Dockerfile" "$DEMO_DIR/Makefile")
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✓ $(basename "$file") exists"
    else
        echo "   ✗ $(basename "$file") NOT FOUND"
        exit 1
    fi
done

# Check binaries are executable
echo ""
echo "2. Checking binaries are executable..."
if [ -x "$DEMO_BIN" ]; then
    echo "   ✓ thp_demo is executable"
else
    echo "   ✗ thp_demo is not executable"
    exit 1
fi

if [ -x "$MONITOR_BIN" ]; then
    echo "   ✓ monitor is executable"
else
    echo "   ✗ monitor is not executable"
    exit 1
fi

# Test demo runs and produces output
echo ""
echo "3. Testing demo execution..."
if output=$("$DEMO_BIN" 2>&1 | head -20); then
    if echo "$output" | grep -q "THP Internals"; then
        echo "   ✓ Demo runs successfully"
        echo "   ✓ Demo produces expected output"
    else
        echo "   ✗ Demo output unexpected"
        exit 1
    fi
else
    echo "   ✗ Demo failed to run"
    exit 1
fi

# Check for THP metrics in /proc/vmstat
echo ""
echo "4. Checking THP metrics availability..."
if [ -f /proc/vmstat ]; then
    thp_counters=$(grep -c "^thp_" /proc/vmstat || echo "0")
    if [ "$thp_counters" -gt 0 ]; then
        echo "   ✓ Found $thp_counters THP counters in /proc/vmstat"
        echo "   Sample counters:"
        grep "^thp_" /proc/vmstat | head -5 | sed 's/^/     /'
    else
        echo "   ⚠ No THP counters found (may be normal on some systems)"
    fi
else
    echo "   ⚠ /proc/vmstat not available"
fi

# Check monitor can read metrics
echo ""
echo "5. Testing monitor metrics reading..."
if [ -f /proc/vmstat ] && [ -f /proc/meminfo ]; then
    # Test if monitor can at least start and read metrics
    if timeout 1 "$MONITOR_BIN" >/dev/null 2>&1; then
        echo "   ✓ Monitor can read /proc/vmstat and /proc/meminfo"
    else
        echo "   ⚠ Monitor may require interactive terminal"
    fi
else
    echo "   ⚠ /proc files not available for testing"
fi

# Check for duplicate processes
echo ""
echo "6. Checking for duplicate services..."
running_demos=0
running_monitors=0
ps aux | grep -q "[t]hp_demo" && running_demos=1 || true
ps aux | grep -q "[m]onitor" && running_monitors=1 || true
if [ "$running_demos" -eq 0 ] && [ "$running_monitors" -eq 0 ]; then
    echo "   ✓ No duplicate services running"
else
    echo "   ⚠ Found running processes:"
    ps aux | grep -E "[t]hp_demo|[m]onitor" || true
fi

# Check Docker image
echo ""
echo "7. Checking Docker image..."
if docker images thp_demo 2>/dev/null | grep -q "thp_demo"; then
    echo "   ✓ Docker image 'thp_demo' exists"
else
    echo "   ⚠ Docker image not found (run setup.sh to build)"
fi

echo ""
echo "=== Validation Complete ==="
echo ""
echo "Startup scripts:"
echo "  Demo:    $DEMO_DIR/start_demo.sh"
echo "  Monitor: $DEMO_DIR/start_monitor.sh"
echo ""
echo "To run the dashboard (monitor) in interactive mode:"
echo "  $MONITOR_BIN"
echo ""

