#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -x "${SCRIPT_DIR}/src/scx_task_migration" ] || [ ! -x "${SCRIPT_DIR}/src/scx_revert_monitor" ]; then
    echo "startup: binaries missing; building first"
    make -C "${SCRIPT_DIR}/src"
fi

echo "startup: running migration simulation"
"${SCRIPT_DIR}/src/scx_task_migration" 5

echo ""
echo "startup: launching dashboard monitor (press q to quit)"
"${SCRIPT_DIR}/src/scx_revert_monitor" || true
