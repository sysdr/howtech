#!/usr/bin/env bash
# Runs alloc_bench with strace to capture mmap/brk/madvise counts
set -euo pipefail
BIN="$1"; LABEL="$2"
echo -e "\n\033[1;36m[strace] syscall profile: $LABEL\033[0m"
strace -e trace=mmap,munmap,brk,madvise -c "$BIN" "$LABEL" 2>&1 | \
    grep -E "^(% time|---|----|mmap|munmap|brk|madvise|Total)" || true
