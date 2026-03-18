#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

rm -f "${SCRIPT_DIR}/src/scx_task_migration" "${SCRIPT_DIR}/src/scx_revert_monitor"

if command -v docker &>/dev/null; then
    for name in scx-migration scx-monitor; do
        if docker ps -a --format '{{.Names}}' | grep -qx "$name"; then
            docker rm -f "$name" >/dev/null 2>&1 || true
        fi
    done
    if docker image inspect scx-revert-demo:latest >/dev/null 2>&1; then
        docker rmi -f scx-revert-demo:latest >/dev/null 2>&1 || true
    fi
fi

echo "cleanup: done"
