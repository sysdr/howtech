#!/bin/bash
# Start the performance monitor dashboard

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
MONITOR="${BUILD_DIR}/monitor"

# Check if monitor exists
if [ ! -f "${MONITOR}" ]; then
    echo "Error: Monitor binary not found. Please run setup.sh first."
    exit 1
fi

# Check if servers are running
if ! pgrep -f "traditional_server.*testfile" > /dev/null || \
   ! pgrep -f "sendfile_server.*testfile" > /dev/null || \
   ! pgrep -f "splice_server.*testfile" > /dev/null; then
    echo "Warning: Some servers may not be running."
    echo "Please start servers first with: ./start_servers.sh"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo "Starting performance monitor dashboard..."
echo "Press Ctrl+C to stop"
echo ""

"${MONITOR}"

