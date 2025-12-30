#!/bin/bash
# Test script to verify all binaries are built and executable

set -e

echo "Testing binaries..."
echo "=================="

cd "$(dirname "$0")"

# Check if build directory exists
if [[ ! -d "build" ]]; then
    echo "ERROR: build directory not found"
    exit 1
fi

# Test each binary
BINARIES=("workload" "ftrace_controller" "analyzer" "monitor")
ALL_OK=true

for bin in "${BINARIES[@]}"; do
    if [[ -f "build/$bin" ]]; then
        if [[ -x "build/$bin" ]]; then
            echo "✓ $bin exists and is executable"
            # Try to get file info
            file "build/$bin" | grep -q "ELF" && echo "  - Valid ELF binary"
        else
            echo "✗ $bin exists but is not executable"
            ALL_OK=false
        fi
    else
        echo "✗ $bin not found"
        ALL_OK=false
    fi
done

# Test analyzer with invalid file (should not crash)
echo ""
echo "Testing analyzer with invalid file..."
if ./build/analyzer /nonexistent 2>&1 | grep -q "Failed to open"; then
    echo "✓ Analyzer handles errors correctly"
else
    echo "✗ Analyzer error handling test failed"
    ALL_OK=false
fi

# Check source files
echo ""
echo "Checking source files..."
SOURCES=("src/workload.c" "src/ftrace_controller.c" "src/analyzer.c" "src/monitor.c")
for src in "${SOURCES[@]}"; do
    if [[ -f "$src" ]]; then
        echo "✓ $(basename $src) exists"
    else
        echo "✗ $(basename $src) missing"
        ALL_OK=false
    fi
done

# Check scripts
echo ""
echo "Checking scripts..."
SCRIPTS=("demo.sh" "cleanup.sh")
for script in "${SCRIPTS[@]}"; do
    if [[ -f "$script" ]] && [[ -x "$script" ]]; then
        echo "✓ $script exists and is executable"
    else
        echo "✗ $script missing or not executable"
        ALL_OK=false
    fi
done

echo ""
if $ALL_OK; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi

