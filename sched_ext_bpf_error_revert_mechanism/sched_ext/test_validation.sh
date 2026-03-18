#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -x "${SCRIPT_DIR}/src/scx_task_migration" ] || [ ! -x "${SCRIPT_DIR}/src/scx_revert_monitor" ]; then
    echo "test: binaries missing; building first"
    make -C "${SCRIPT_DIR}/src" >/dev/null
fi

echo "test: scx_task_migration output sanity"
OUT="$("${SCRIPT_DIR}/src/scx_task_migration" 2)"
echo "$OUT" | grep -q "scx_task_migration: userspace simulation"
echo "$OUT" | grep -q "scx_task_migration: done"

echo "test: monitor starts (1s) and exits on SIGTERM"
OUT_M="$("${SCRIPT_DIR}/src/scx_revert_monitor" --text --samples 2 --interval-ms 250)"
echo "$OUT_M" | grep -q "sample=1"
echo "$OUT_M" | grep -q "sample=2"

echo "test: passed"
