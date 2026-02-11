#!/bin/bash
# Test script to validate setup and files

set -e

echo "=== Testing sched_ext CPU Selection Demo Setup ==="
echo ""

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

ERRORS=0

# Check required files
echo "[1/8] Checking required files..."
REQUIRED_FILES=(
    "src/scheduler.bpf.c"
    "include/scheduler.h"
    "src/monitor.c"
    "src/interactive_workload.c"
    "src/batch_workload.c"
    "src/group_manager.c"
    "src/get_map_fd.c"
    "Makefile"
    "Dockerfile"
    "scripts/run_demo.sh"
    "scripts/cleanup.sh"
    "README.md"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "  ERROR: Missing file: $file"
        ERRORS=$((ERRORS + 1))
    else
        echo "  OK: $file"
    fi
done

# Check file permissions
echo ""
echo "[2/8] Checking script permissions..."
SCRIPTS=("scripts/run_demo.sh" "scripts/cleanup.sh")
for script in "${SCRIPTS[@]}"; do
    if [ -f "$script" ] && [ ! -x "$script" ]; then
        echo "  WARNING: $script is not executable, fixing..."
        chmod +x "$script"
    fi
    if [ -x "$script" ]; then
        echo "  OK: $script is executable"
    fi
done

# Check for duplicate services
echo ""
echo "[3/8] Checking for duplicate services..."
RUNNING_PROCS=$(ps aux | grep -E "(monitor|interactive_workload|batch_workload|group_manager)" | grep -v grep | grep -v test_setup || true)
if [ -n "$RUNNING_PROCS" ]; then
    echo "  WARNING: Found running processes:"
    echo "$RUNNING_PROCS"
    echo "  Consider running cleanup.sh first"
else
    echo "  OK: No duplicate services running"
fi

# Check BPF tools availability (if in container)
echo ""
echo "[4/8] Checking BPF tools..."
if command -v bpftool &> /dev/null; then
    echo "  OK: bpftool is available"
    BPFTOOL_VERSION=$(bpftool version 2>&1 | head -1 || echo "unknown")
    echo "    Version: $BPFTOOL_VERSION"
else
    echo "  INFO: bpftool not found (expected if not in container)"
fi

# Check build tools
echo ""
echo "[5/8] Checking build tools..."
if command -v clang &> /dev/null; then
    echo "  OK: clang is available"
else
    echo "  INFO: clang not found (expected if not in container)"
fi

if command -v gcc &> /dev/null; then
    echo "  OK: gcc is available"
else
    echo "  WARNING: gcc not found"
    ERRORS=$((ERRORS + 1))
fi

# Check libraries
echo ""
echo "[6/8] Checking required libraries..."
if pkg-config --exists libbpf 2>/dev/null; then
    echo "  OK: libbpf is available"
else
    echo "  INFO: libbpf not found via pkg-config (may be available via package manager)"
fi

if pkg-config --exists ncurses 2>/dev/null; then
    echo "  OK: ncurses is available"
else
    echo "  INFO: ncurses not found via pkg-config (may be available via package manager)"
fi

# Validate script syntax
echo ""
echo "[7/8] Validating script syntax..."
if bash -n scripts/run_demo.sh 2>&1; then
    echo "  OK: run_demo.sh syntax is valid"
else
    echo "  ERROR: run_demo.sh has syntax errors"
    ERRORS=$((ERRORS + 1))
fi

if bash -n scripts/cleanup.sh 2>&1; then
    echo "  OK: cleanup.sh syntax is valid"
else
    echo "  ERROR: cleanup.sh has syntax errors"
    ERRORS=$((ERRORS + 1))
fi

# Check Makefile targets
echo ""
echo "[8/8] Checking Makefile..."
if [ -f "Makefile" ]; then
    if grep -q "get_map_fd" Makefile; then
        echo "  OK: get_map_fd target found in Makefile"
    else
        echo "  ERROR: get_map_fd target missing from Makefile"
        ERRORS=$((ERRORS + 1))
    fi
    
    if grep -q "PROGS.*get_map_fd" Makefile; then
        echo "  OK: get_map_fd in PROGS list"
    else
        echo "  ERROR: get_map_fd not in PROGS list"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo "  ERROR: Makefile not found"
    ERRORS=$((ERRORS + 1))
fi

# Summary
echo ""
echo "==================================="
if [ $ERRORS -eq 0 ]; then
    echo "All tests passed!"
    echo "==================================="
    exit 0
else
    echo "Found $ERRORS error(s)"
    echo "==================================="
    exit 1
fi

