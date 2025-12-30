#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

info() { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Check if running as root or with sudo
if [[ $EUID -ne 0 ]]; then
   warn "This script needs root privileges for ftrace access"
   info "Re-running with sudo..."
   exec sudo bash "$0" "$@"
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if build directory exists
if [[ ! -d "build" ]]; then
    error "Build directory not found. Run 'make' first."
fi

# Check if binaries exist
if [[ ! -f "build/workload" ]] || [[ ! -f "build/analyzer" ]]; then
    error "Binaries not found. Run 'make' first."
fi

# Find tracefs
TRACEFS_ROOT=""
for path in /sys/kernel/tracing /sys/kernel/debug/tracing; do
    if [[ -d "$path" ]]; then
        TRACEFS_ROOT="$path"
        break
    fi
done

if [[ -z "$TRACEFS_ROOT" ]]; then
    error "tracefs not found. Ensure CONFIG_TRACING=y in kernel"
fi

info "Found tracefs at: $TRACEFS_ROOT"

# Setup function to run on exit
cleanup_on_exit() {
    info "Cleaning up tracing..."
    echo 0 > "$TRACEFS_ROOT/tracing_on" 2>/dev/null || true
    echo 0 > "$TRACEFS_ROOT/events/block/enable" 2>/dev/null || true
}
trap cleanup_on_exit EXIT

info "\nStarting demonstration..."
info "This will:"
info "  1. Clear existing traces"
info "  2. Enable block layer tracing"
info "  3. Run I/O workload"
info "  4. Capture trace data"
info "  5. Analyze latencies"
echo ""

# Clear existing trace
echo > "$TRACEFS_ROOT/trace" 2>/dev/null || true

# Set buffer size
echo 16384 > "$TRACEFS_ROOT/buffer_size_kb" 2>/dev/null || warn "Could not set buffer size"

# Enable block events
echo 1 > "$TRACEFS_ROOT/events/block/block_rq_issue/enable" 2>/dev/null || true
echo 1 > "$TRACEFS_ROOT/events/block/block_rq_complete/enable" 2>/dev/null || true
echo 1 > "$TRACEFS_ROOT/tracing_on" 2>/dev/null || true

success "Ftrace block tracing enabled"

# Run workload
info "Running I/O workload..."
./build/workload

# Small delay to let trace buffer flush
sleep 1

# Capture trace
info "Capturing trace data..."
mkdir -p traces
cat "$TRACEFS_ROOT/trace" > traces/trace.out
TRACE_SIZE=$(wc -l < traces/trace.out)
success "Captured $TRACE_SIZE lines of trace data"

# Disable tracing
echo 0 > "$TRACEFS_ROOT/tracing_on" 2>/dev/null || true

# Analyze
info "Analyzing trace data..."
./build/analyzer traces/trace.out

success "\nDemo complete!"
info "Trace data saved to: $(pwd)/traces/trace.out"
info "You can re-analyze with: sudo ./build/analyzer traces/trace.out"
