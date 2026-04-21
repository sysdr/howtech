#!/usr/bin/env bash
# Remove demo netns/veth, traffic helpers, and optionally build artifacts.
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NETNS="policy-test"
IFACE="veth-policy"
cd "$DIR"
if [[ -f .traffic_pids ]]; then
    # shellcheck disable=SC1091
    source .traffic_pids
    kill "${PING_PID:-0}" "${TCP80_PID:-0}" "${TCP8080_PID:-0}" 2>/dev/null || true
    rm -f .traffic_pids
fi
ip netns del "$NETNS" 2>/dev/null || true
ip link del "$IFACE" 2>/dev/null || true
rm -f /sys/fs/bpf/policy_engine_prog 2>/dev/null || true
if [[ "${1:-}" == "--all" ]]; then
    rm -f policy_engine.bpf.o policy_engine
    echo "Removed build artifacts and network demo state."
else
    echo "Removed network demo state (use --all to delete policy_engine binaries too)."
fi
