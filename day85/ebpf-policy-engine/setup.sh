#!/usr/bin/env bash
# Run from ebpf-policy-engine/: forwards to the canonical demo script one level up.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec "$ROOT/setup.sh" "$@"
