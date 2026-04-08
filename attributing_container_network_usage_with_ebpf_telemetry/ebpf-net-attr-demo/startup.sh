#!/usr/bin/env bash
# Launch the ncurses dashboard after: sudo ../setup.sh [--setup-only]
#
# Layout: ../setup.sh and this directory (ebpf-net-attr-demo/) share the same parent.
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${DEMO_DIR}/.." && pwd)"
STATS_PIN="${STATS_PIN:-/sys/fs/bpf/net_attr_stats}"
NET_ATTR="${DEMO_DIR}/net_attr"

if [[ ! -f "$NET_ATTR" ]]; then
    echo "error: $NET_ATTR not found. See $REPO_ROOT/setup.sh (--generate-only to build; sudo for full demo)" >&2
    exit 1
fi
if [[ ! -x "$NET_ATTR" ]]; then
    echo "error: $NET_ATTR is not executable" >&2
    exit 1
fi
if [[ ! -e "$STATS_PIN" ]]; then
    echo "error: pinned map missing: $STATS_PIN (run: sudo $REPO_ROOT/setup.sh [--setup-only])" >&2
    exit 1
fi

cd "$DEMO_DIR"
exec ./net_attr "$STATS_PIN"
