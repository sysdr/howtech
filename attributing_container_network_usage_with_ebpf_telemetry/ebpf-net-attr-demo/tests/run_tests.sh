#!/usr/bin/env bash
# Smoke tests: generated tree, build artifacts, optional --once when map exists
set -euo pipefail

# Repo layout: setup.sh and ebpf-net-attr-demo/ are siblings under the repo root.
# This script may live at repo/tests/ (go up ..) or ebpf-net-attr-demo/tests/ (../..).
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "${HERE}/../setup.sh" && -d "${HERE}/../ebpf-net-attr-demo" ]]; then
    ROOT="$(cd "${HERE}/.." && pwd)"
elif [[ -f "${HERE}/../../setup.sh" && -d "${HERE}/../../ebpf-net-attr-demo" ]]; then
    ROOT="$(cd "${HERE}/../.." && pwd)"
else
    echo "error: cannot find repo root (expected setup.sh and ebpf-net-attr-demo/ as siblings)" >&2
    exit 1
fi
DEMO="${ROOT}/ebpf-net-attr-demo"
SETUP="${ROOT}/setup.sh"
STATS_PIN="${STATS_PIN:-/sys/fs/bpf/net_attr_stats}"

echo "[*] Ensuring sources and build via --generate-only..."
bash "$SETUP" --generate-only

echo "[*] Verifying expected files..."
for f in net_attr.bpf.c net_attr.c Makefile net_attr.bpf.o net_attr; do
    [[ -e "${DEMO}/${f}" ]] || { echo "missing: ${DEMO}/${f}" >&2; exit 1; }
done
[[ -x "${DEMO}/net_attr" ]] || { echo "not executable: ${DEMO}/net_attr" >&2; exit 1; }

echo "[*] Checking for duplicate net_attr processes..."
if pgrep -x net_attr >/dev/null 2>&1; then
    echo "warning: net_attr already running (pgrep -x net_attr)" >&2
fi

echo "[*] Optional: map dump (--once) when ${STATS_PIN} exists..."
if [[ -e "$STATS_PIN" ]]; then
    "${DEMO}/net_attr" "$STATS_PIN" --once
else
    echo "    (skipped - no pinned map; run: sudo $SETUP --setup-only)"
fi

echo "[OK] All tests passed."
