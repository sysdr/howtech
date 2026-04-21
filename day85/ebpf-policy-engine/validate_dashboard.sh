#!/usr/bin/env bash
# Confirms ncurses monitor exposes expected metrics (source + binary strings).
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="${DIR}/policy_engine.c"
BIN="${DIR}/policy_engine"
[[ -f "$SRC" ]] || { echo "error: $SRC not found" >&2; exit 1; }
for needle in n_allow n_deny "Allowed:" "Denied:" "Rate:" "RECENT EVENTS" "ACTIVE POLICIES"; do
    grep -q "$needle" "$SRC" || { echo "error: dashboard metric missing in source: $needle" >&2; exit 1; }
done
[[ -x "$BIN" ]] || { echo "error: $BIN not built" >&2; exit 1; }
for s in "Allowed" "Denied" "RECENT EVENTS" "eBPF Policy Engine"; do
    strings "$BIN" | grep -q "$s" || { echo "error: expected string not in binary: $s" >&2; exit 1; }
done
echo "Dashboard (policy_engine) metrics: OK (source + strings check)."
