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

# Check if running as root or with sudo
if [[ $EUID -ne 0 ]]; then
   warn "This script needs root privileges for ftrace access"
   info "Re-running with sudo..."
   exec sudo bash "$0" "$@"
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
    warn "tracefs not found. Nothing to clean up."
    exit 0
fi

info "Cleaning up ftrace configuration..."

# Disable tracing
echo 0 > "$TRACEFS_ROOT/tracing_on" 2>/dev/null || warn "Could not disable tracing"
echo 0 > "$TRACEFS_ROOT/events/block/enable" 2>/dev/null || warn "Could not disable block events"

# Clear trace buffer
echo > "$TRACEFS_ROOT/trace" 2>/dev/null || warn "Could not clear trace buffer"

success "Ftrace cleanup complete!"
