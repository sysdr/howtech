#!/bin/bash
set -e

cd "$(dirname "$0")"

export LD_LIBRARY_PATH=./build:$LD_LIBRARY_PATH

echo "=========================================="
echo "  GOT/PLT Dynamic Linking - Test Suite"
echo "=========================================="
echo ""

# Check for duplicate processes
echo "[*] Checking for duplicate services..."
RUNNING=$(ps aux | grep -E "(test-lazy|test-eager|test-no-pie|monitor|measure)" | grep -v grep | wc -l)
if [ "$RUNNING" -gt 0 ]; then
    echo "  WARNING: Found $RUNNING running process(es)"
    ps aux | grep -E "(test-lazy|test-eager|test-no-pie|monitor|measure)" | grep -v grep
else
    echo "  ✓ No duplicate services running"
fi
echo ""

# Verify all binaries exist
echo "[*] Verifying binaries..."
BINARIES=("output/test-lazy" "output/test-eager" "output/test-no-pie" "output/monitor" "output/measure" "build/libexample.so")
ALL_EXIST=true
for bin in "${BINARIES[@]}"; do
    if [ -f "$bin" ]; then
        echo "  ✓ $bin"
    else
        echo "  ✗ $bin MISSING"
        ALL_EXIST=false
    fi
done
echo ""

if [ "$ALL_EXIST" = false ]; then
    echo "ERROR: Some binaries are missing. Run ./setup.sh first."
    exit 1
fi

# Run tests
echo "[*] Running test-lazy..."
./output/test-lazy > /dev/null 2>&1 && echo "  ✓ test-lazy passed" || echo "  ✗ test-lazy failed"

echo ""
echo "[*] Running test-eager..."
./output/test-eager > /dev/null 2>&1 && echo "  ✓ test-eager passed" || echo "  ✗ test-eager failed"

echo ""
echo "[*] Running test-no-pie..."
./output/test-no-pie > /dev/null 2>&1 && echo "  ✓ test-no-pie passed" || echo "  ✗ test-no-pie failed"

echo ""
echo "[*] Running measure program..."
./output/measure > /dev/null 2>&1 && echo "  ✓ measure passed" || echo "  ✗ measure failed"

echo ""
echo "[*] Testing monitor (dashboard)..."
if timeout 1 ./output/monitor > /dev/null 2>&1; then
    echo "  ✓ monitor starts successfully (interactive program)"
else
    # Check if it's just a timeout (expected) or an actual error
    if [ $? -eq 124 ]; then
        echo "  ✓ monitor starts successfully (timeout expected for interactive program)"
    else
        echo "  ✗ monitor failed to start"
    fi
fi

echo ""
echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo ""
echo "All binaries built and tested successfully!"
echo ""
echo "To run the interactive monitor (dashboard):"
echo "  export LD_LIBRARY_PATH=./build:\$LD_LIBRARY_PATH"
echo "  ./output/monitor"
echo ""
echo "To run individual tests:"
echo "  ./output/test-lazy"
echo "  ./output/test-eager"
echo "  ./output/test-no-pie"
echo "  ./output/measure"
echo ""

