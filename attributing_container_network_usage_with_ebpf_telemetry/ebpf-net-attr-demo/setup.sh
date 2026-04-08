#!/usr/bin/env bash
# Always delegate to the canonical setup next to this directory (same layout as article).
# Do not keep a second copy of setup logic here — it goes out of date and breaks WSL/bpftool fixes.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${HERE}/../setup.sh" "$@"
