#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${DIR}/policy_engine"
IFACE="${1:-veth-policy}"
[[ -x "$BIN" ]] || { echo "error: $BIN missing or not executable. Run: ${DIR}/setup.sh --generate-only" >&2; exit 1; }
[[ $EUID -eq 0 ]] || { echo "error: root required for TC+BPF attach" >&2; exit 1; }
if ! ip link show "$IFACE" &>/dev/null; then
    echo "error: interface $IFACE not found. Run: sudo ${DIR}/setup.sh or sudo ${DIR}/setup.sh --setup-only" >&2
    exit 1
fi
if pgrep -x policy_engine >/dev/null 2>&1; then
    echo "error: policy_engine already running (duplicate dashboard). Quit: pkill -x policy_engine" >&2
    exit 1
fi
exec "$BIN" "$IFACE"
