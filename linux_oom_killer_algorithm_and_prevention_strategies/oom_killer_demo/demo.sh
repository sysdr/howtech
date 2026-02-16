#!/usr/bin/env bash
# Quick demo runner script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$SCRIPT_DIR/oom_killer_demo"

if [ ! -d "$WORK_DIR/bin" ] || [ ! -f "$WORK_DIR/bin/oom_demo" ]; then
    echo "Error: Demo not built. Run ./setup.sh first."
    exit 1
fi

echo "Running OOM Demo..."
"$WORK_DIR/bin/oom_demo"
