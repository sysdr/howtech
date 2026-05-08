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

  // Draw one horizontal bar scaled to current maxq.
  // Kept as a small helper to avoid requiring C++ lambdas.
  void (*draw_bar)(int, const char *, long, int) = NULL;
  (void)draw_bar; // suppress unused warnings in some compilers

  // C doesn't support nested named functions portably; inline the rendering.
  int barw = (int)((double)m->q_high / (double)maxq * (double)(width - 15));
  if (barw < 0) barw = 0;
  mvprintw(row + 0, col, "%-6s |", "HIGH");
  attron(COLOR_PAIR(1));
  for (int i = 0; i < barw; i++) addch('=');
  attroff(COLOR_PAIR(1));
  for (int i = barw; i < width - 15; i++) addch(' ');
  printw("|");

  barw = (int)((double)m->q_norm / (double)maxq * (double)(width - 15));
  if (barw < 0) barw = 0;
  mvprintw(row + 1, col, "%-6s |", "NORM");
  attron(COLOR_PAIR(2));
  for (int i = 0; i < barw; i++) addch('=');
  attroff(COLOR_PAIR(2));
  for (int i = barw; i < width - 15; i++) addch(' ');
  printw("|");

  barw = (int)((double)m->q_low / (double)maxq * (double)(width - 15));
  if (barw < 0) barw = 0;
  mvprintw(row + 2, col, "%-6s |", "LOW");
  attron(COLOR_PAIR(3));
  for (int i = 0; i < barw; i++) addch('=');
  attroff(COLOR_PAIR(3));
  for (int i = barw; i < width - 15; i++) addch(' ');
  printw("|");

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

static int run_headless(long steps, bool heavy_high) {
  if (steps < 1) steps = 1;
  model_t m = {.q_high = 0, .q_norm = 0, .q_low = 0, .tick = 0, .heavy_high = heavy_high};
  for (long i = 0; i < steps; i++) {
    model_step(&m);
    // CSV-ish line for easy grepping/plotting
    printf("tick=%ld high=%ld norm=%ld low=%ld heavy_high=%s\n",
           m.tick, m.q_high, m.q_norm, m.q_low, m.heavy_high ? "1" : "0");
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--smoke") == 0) return run_smoke();
  if (argc > 2 && strcmp(argv[1], "--headless") == 0) {
    char *end = NULL;
    long steps = strtol(argv[2], &end, 10);
    if (end == argv[2]) steps = 20;
    bool heavy = false;
    if (argc > 3 && strcmp(argv[3], "--heavy-high") == 0) heavy = true;
    return run_headless(steps, heavy);
  }

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
