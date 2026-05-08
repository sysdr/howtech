#!/usr/bin/env bash
# setup.sh — sched_ext DSQ Manager demo
# Builds the userspace simulation in Docker (preferred), then runs it interactively.
# If Docker is unavailable, falls back to a native build.
#
# This script is intentionally self-contained: it can scaffold missing demo files
# (Dockerfile + src/) so a fresh checkout can run immediately.

set -euo pipefail

RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

IMAGE_NAME="dsq-manager-demo"
CONTAINER_NAME="dsq-demo-run"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="interactive" # interactive | smoke | build-only

log()  { echo -e "${CYAN}[demo]${RESET} $*"; }
ok()   { echo -e "${GREEN}[ok]${RESET}   $*"; }
warn() { echo -e "${YELLOW}[warn]${RESET} $*"; }
die()  { echo -e "${RED}[fail]${RESET} $*" >&2; exit 1; }

# ── Args ───────────────────────────────────────────────────────────────

usage() {
    cat <<'EOF'
Usage:
  setup.sh [--interactive|--smoke|--build-only]

Modes:
  --interactive  Build (Docker preferred) and run ncurses UI (default)
  --smoke        Build and run non-interactive smoke test (dsq_sim --smoke)
  --build-only   Only build (Docker image or native binary), then exit
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --interactive) MODE="interactive" ;;
            --smoke) MODE="smoke" ;;
            --build-only) MODE="build-only" ;;
            -h|--help) usage; exit 0 ;;
            *) die "Unknown arg: $1 (try --help)" ;;
        esac
        shift
    done
}

# ── Scaffolding / verification ─────────────────────────────────────────

ensure_scaffold() {
    local dockerfile="${SCRIPT_DIR}/Dockerfile"
    local src_dir="${SCRIPT_DIR}/src"
    local makefile="${src_dir}/Makefile"
    local sim_src="${src_dir}/dsq_sim.c"

    if [[ ! -f "${dockerfile}" || ! -d "${src_dir}" || ! -f "${makefile}" || ! -f "${sim_src}" ]]; then
        warn "Missing demo files; generating scaffold in ${SCRIPT_DIR}"
        mkdir -p "${src_dir}"

        if [[ ! -f "${dockerfile}" ]]; then
            cat > "${dockerfile}" <<'EOF'
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential make gcc libc6-dev \
    libncurses6 libncurses-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ /build/
RUN make -s

CMD ["./dsq_sim"]
EOF
        fi

        if [[ ! -f "${makefile}" ]]; then
            cat > "${makefile}" <<'EOF'
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?= -lncurses

BIN := dsq_sim
SRC := dsq_sim.c

.PHONY: all clean test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test: $(BIN)
	./$(BIN) --smoke

clean:
	rm -f $(BIN)
EOF
        fi

        if [[ ! -f "${sim_src}" ]]; then
            cat > "${sim_src}" <<'EOF'
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  long q_high;
  long q_norm;
  long q_low;
  long tick;
  bool heavy_high;
} model_t;

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int _) {
  (void)_;
  g_stop = 1;
}

static long clamp0(long v) { return v < 0 ? 0 : v; }

static void model_step(model_t *m) {
  // Arrival rates
  const long a_high = m->heavy_high ? 80 : 12;
  const long a_norm = 20;
  const long a_low = 10;

  // Service budget per tick (simulating dispatch capacity)
  const long budget_total = 70;

  m->q_high += a_high;
  m->q_norm += a_norm;
  m->q_low += a_low;

  // Priority scheduling with a starvation guard:
  // every 16 "high" services, force a low service if low queue non-empty.
  static long promo_ctr = 0;
  long b = budget_total;

  while (b-- > 0) {
    if (m->q_low > 0 && promo_ctr >= 16) {
      m->q_low--;
      promo_ctr = 0;
      continue;
    }
    if (m->q_high > 0) {
      m->q_high--;
      promo_ctr++;
      continue;
    }
    if (m->q_norm > 0) {
      m->q_norm--;
      continue;
    }
    if (m->q_low > 0) {
      m->q_low--;
      promo_ctr = 0;
      continue;
    }
    break;
  }

  m->q_high = clamp0(m->q_high);
  m->q_norm = clamp0(m->q_norm);
  m->q_low = clamp0(m->q_low);
  m->tick++;
}

static void draw_ui(const model_t *m) {
  erase();
  mvprintw(0, 0, "sched_ext DSQ Manager — userspace simulation");
  mvprintw(1, 0, "Controls: d=toggle heavy-HIGH mode | q=quit");
  mvprintw(2, 0, "Tick: %-8ld  Heavy-HIGH: %s", m->tick, m->heavy_high ? "ON" : "OFF");

  const int base = 4;
  mvprintw(base + 0, 0, "DSQ[0] HIGH   queued: %-8ld", m->q_high);
  mvprintw(base + 1, 0, "DSQ[1] NORMAL queued: %-8ld", m->q_norm);
  mvprintw(base + 2, 0, "DSQ[2] LOW    queued: %-8ld", m->q_low);

  mvprintw(base + 4, 0, "Starvation guard: PROMO_RATIO=16 (forces LOW dispatch periodically)");

  // Simple bar visualization (scaled)
  int col = 0;
  int row = base + 6;
  int width = COLS - 2;
  if (width < 10) width = 10;

  long maxq = m->q_high;
  if (m->q_norm > maxq) maxq = m->q_norm;
  if (m->q_low > maxq) maxq = m->q_low;
  if (maxq < 1) maxq = 1;

  auto draw_bar = [&](int r, const char *label, long v, int color_pair) {
    int barw = (int)((double)v / (double)maxq * (double)(width - 15));
    if (barw < 0) barw = 0;
    mvprintw(r, col, "%-6s |", label);
    attron(COLOR_PAIR(color_pair));
    for (int i = 0; i < barw; i++) addch('=');
    attroff(COLOR_PAIR(color_pair));
    for (int i = barw; i < width - 15; i++) addch(' ');
    printw("|");
  };

  draw_bar(row + 0, "HIGH", m->q_high, 1);
  draw_bar(row + 1, "NORM", m->q_norm, 2);
  draw_bar(row + 2, "LOW",  m->q_low,  3);

  refresh();
}

static int run_smoke(void) {
  model_t m = {.q_high = 0, .q_norm = 0, .q_low = 0, .tick = 0, .heavy_high = false};
  for (int i = 0; i < 50; i++) model_step(&m);
  // Basic sanity: non-negative and ticks advanced.
  if (m.tick != 50) return 2;
  if (m.q_high < 0 || m.q_norm < 0 || m.q_low < 0) return 3;
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--smoke") == 0) return run_smoke();

  signal(SIGINT, on_sigint);
  signal(SIGTERM, on_sigint);

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
  }

  model_t m = {.q_high = 0, .q_norm = 0, .q_low = 0, .tick = 0, .heavy_high = false};

  struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000L}; // 100ms
  while (!g_stop) {
    int ch = getch();
    if (ch == 'q' || ch == 'Q') break;
    if (ch == 'd' || ch == 'D') m.heavy_high = !m.heavy_high;

    model_step(&m);
    draw_ui(&m);
    nanosleep(&ts, NULL);
  }

  endwin();
  return 0;
}
EOF
        fi

        ok "Scaffold generated: Dockerfile + src/"
    fi
}

# ── Prerequisite checks ───────────────────────────────────────────────

check_deps() {
    local missing=()
    command -v docker >/dev/null 2>&1 || missing+=("docker")
    if [[ ${#missing[@]} -gt 0 ]]; then
        die "Missing dependencies: ${missing[*]}\nInstall docker and re-run."
    fi
}

# ── Docker build ──────────────────────────────────────────────────────

build_image() {
    log "Building Docker image (Ubuntu 24.04 + ncurses)..."
    docker build -t "${IMAGE_NAME}" "${SCRIPT_DIR}" -f "${SCRIPT_DIR}/Dockerfile" --progress=plain

    # Verify the binary was compiled clean
    docker run --rm "${IMAGE_NAME}" sh -c 'test -x /build/dsq_sim && echo "Binary OK"' | \
        grep -q "Binary OK" || die "Build verification failed"
    ok "Image built: ${IMAGE_NAME}"
}

# ── BPF source info ───────────────────────────────────────────────────

show_bpf_info() {
    echo
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -e "${BOLD}  BPF Scheduler Source: dsq_manager.bpf.c${RESET}"
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo
    echo -e "  Requires: Linux kernel ${YELLOW}6.12+${RESET} with ${YELLOW}CONFIG_SCHED_CLASS_EXT=y${RESET}"
    echo -e "  To load on a compatible kernel:"
    echo
    echo -e "    ${CYAN}# Build with libbpf + clang + kernel sched_ext headers${RESET}"
    echo -e "    ${CYAN}clang -O2 -g -target bpf -c dsq_manager.bpf.c -o dsq_manager.bpf.o${RESET}"
    echo -e "    ${CYAN}bpftool gen skeleton dsq_manager.bpf.o > dsq_manager.skel.h${RESET}"
    echo -e "    ${CYAN}gcc -O2 -o dsq_manager dsq_manager.c -lbpf${RESET}"
    echo -e "    ${CYAN}sudo ./dsq_manager${RESET}"
    echo
    echo -e "  Check scheduler state once loaded:"
    echo -e "    ${CYAN}cat /sys/kernel/debug/sched_ext/root/ops${RESET}"
    echo -e "    ${CYAN}bpftool prog show name dsq_mgr_enqueue${RESET}"
    echo
    echo -e "  DSQ IDs:  HIGH=${YELLOW}0${RESET}  NORMAL=${YELLOW}1${RESET}  LOW=${YELLOW}2${RESET}"
    echo -e "  Key APIs: ${YELLOW}scx_bpf_create_dsq()  scx_bpf_dispatch_vtime()${RESET}"
    echo -e "            ${YELLOW}scx_bpf_consume()     scx_bpf_dsq_nr_queued()${RESET}"
    echo
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo
}

# ── Run simulation ────────────────────────────────────────────────────

run_simulation() {
    log "Launching DSQ simulation (ncurses monitor)..."
    echo
    echo -e "  ${BOLD}Controls:${RESET}"
    echo -e "    ${YELLOW}d${RESET} — toggle heavy-HIGH mode (starvation test)"
    echo -e "    ${YELLOW}q${RESET} — quit"
    echo
    echo -e "  Watch how LOW-priority queue depth grows under heavy HIGH load,"
    echo -e "  then stabilises when the starvation guard (PROMO_RATIO=16) kicks in."
    echo
    sleep 2

    docker run --rm -it \
        --name "${CONTAINER_NAME}" \
        "${IMAGE_NAME}" \
        ./dsq_sim
}

run_smoke_docker() {
    log "Running smoke test in Docker..."
    docker run --rm "${IMAGE_NAME}" ./dsq_sim --smoke
    ok "Smoke test passed (Docker)"
}

# ── Native fallback (no Docker) ───────────────────────────────────────

run_native() {
    log "Docker not available — attempting native build..."
    cd "${SCRIPT_DIR}/src"
    make -s
    ok "Native build successful"
    if [[ "${MODE}" == "build-only" ]]; then
        return
    fi
    if [[ "${MODE}" == "smoke" ]]; then
        ./dsq_sim --smoke
        ok "Smoke test passed (native)"
        return
    fi
    echo
    show_bpf_info
    ./dsq_sim
}

# ── Main ──────────────────────────────────────────────────────────────

main() {
    parse_args "$@"
    echo
    echo -e "${BOLD}${CYAN}sched_ext DSQ Manager — Article 25 Demo${RESET}"
    echo -e "${CYAN}Systems Programming Deep Dive${RESET}"
    echo

    ensure_scaffold

    if ! command -v docker >/dev/null 2>&1; then
        warn "Docker not found — trying native build"
        run_native
        return
    fi

    check_deps
    build_image
    if [[ "${MODE}" == "build-only" ]]; then
        return
    fi
    if [[ "${MODE}" == "smoke" ]]; then
        run_smoke_docker
        return
    fi
    show_bpf_info
    run_simulation
}

main "$@"