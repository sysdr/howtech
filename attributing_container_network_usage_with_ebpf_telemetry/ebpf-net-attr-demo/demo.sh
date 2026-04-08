#!/usr/bin/env bash
# Convenience launcher for this demo directory.
# - Ensures setup has created pinned BPF objects/maps.
# - Starts the ncurses dashboard.
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${DEMO_DIR}/.." && pwd)"
SETUP_SH="${REPO_ROOT}/setup.sh"
STARTUP_SH="${DEMO_DIR}/startup.sh"
STATS_PIN="${STATS_PIN:-/sys/fs/bpf/net_attr_stats}"

if [[ ! -x "${STARTUP_SH}" ]]; then
    chmod +x "${STARTUP_SH}" 2>/dev/null || true
fi

if [[ ! -e "${STATS_PIN}" ]]; then
    if [[ ! -f "${SETUP_SH}" ]]; then
        echo "error: ${SETUP_SH} not found." >&2
        exit 1
    fi
    echo "[*] pinned map missing (${STATS_PIN}); running setup first..."
    if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
        "${SETUP_SH}" --setup-only
    else
        sudo "${SETUP_SH}" --setup-only
    fi
fi

exec "${STARTUP_SH}"
