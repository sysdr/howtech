#!/usr/bin/env bash
# Startup script for THP Demo
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_BIN="$DEMO_DIR/build/thp_demo"

if [ ! -f "$DEMO_BIN" ]; then
    echo "Error: Demo binary not found at $DEMO_BIN"
    echo "Please run: make all"
    exit 1
fi

if [ ! -x "$DEMO_BIN" ]; then
    chmod +x "$DEMO_BIN"
fi

echo "Starting THP Demo from: $DEMO_BIN"
exec "$DEMO_BIN" "$@"

