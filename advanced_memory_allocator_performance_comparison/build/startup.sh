#!/usr/bin/env bash
# startup.sh — Run all benchmarks and collect results
set -euo pipefail
BOLD='\033[1m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; RESET='\033[0m'
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${BOLD}${CYAN}Starting memory allocator benchmarks...${RESET}\n"

RESULTS_DIR="$WORKDIR/results"
mkdir -p "$RESULTS_DIR"

echo -e "${CYAN}[1/3] Running ptmalloc2 benchmark...${RESET}"
docker run --rm --name alloc-bench-ptmalloc alloc-bench:latest \
    /bench/bench_system "ptmalloc2(glibc)" > "$RESULTS_DIR/ptmalloc2.txt" 2>&1 || true
cat "$RESULTS_DIR/ptmalloc2.txt"

echo -e "\n${CYAN}[2/3] Running tcmalloc benchmark...${RESET}"
docker run --rm --name alloc-bench-tcmalloc alloc-bench:latest \
    /bench/bench_tcmalloc "tcmalloc" > "$RESULTS_DIR/tcmalloc.txt" 2>&1 || true
cat "$RESULTS_DIR/tcmalloc.txt"

echo -e "\n${CYAN}[3/3] Running jemalloc benchmark...${RESET}"
docker run --rm --name alloc-bench-jemalloc alloc-bench:latest \
    /bench/bench_jemalloc "jemalloc" > "$RESULTS_DIR/jemalloc.txt" 2>&1 || true
cat "$RESULTS_DIR/jemalloc.txt"

echo -e "\n${GREEN}✓ All benchmarks completed. Results saved to $RESULTS_DIR${RESET}\n"
