// Minimal ncurses monitor for per-thread scheduler info.
// Usage: ./sched_monitor <pid>
//
// Displays for each TID:
// - policy name (from /proc/<pid>/task/<tid>/sched)
// - voluntary / involuntary context switches
//
// Press q/Q to quit.

#define _GNU_SOURCE
#include <errno.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

static bool read_kv_u64(const char *path, const char *key, unsigned long long *out) {
  FILE *f = fopen(path, "r");
  if (!f) return false;

  char *line = NULL;
  size_t cap = 0;
  bool ok = false;
  while (getline(&line, &cap, f) != -1) {
    if (strncmp(line, key, strlen(key)) == 0) {
      const char *p = line + strlen(key);
      while (*p == ' ' || *p == '\t') p++;
      unsigned long long v = 0;
      if (sscanf(p, "%llu", &v) == 1) {
        *out = v;
        ok = true;
        break;
      }
    }
  }
  free(line);
  fclose(f);
  return ok;
}

static bool read_policy(const char *sched_path, char *buf, size_t buflen) {
  FILE *f = fopen(sched_path, "r");
  if (!f) return false;

  // Find line like: "policy                                  :                    0"
  // And map known values to human-readable string.
  char *line = NULL;
  size_t cap = 0;
  int pol = -1;
  while (getline(&line, &cap, f) != -1) {
    if (strncmp(line, "policy", 6) == 0) {
      const char *p = strchr(line, ':');
      if (p) {
        while (*p && (*p == ':' || *p == ' ' || *p == '\t')) p++;
        (void)sscanf(p, "%d", &pol);
      }
      break;
    }
  }
  free(line);
  fclose(f);

  const char *name = "UNKNOWN";
  switch (pol) {
    case 0: name = "SCHED_OTHER"; break;
    case 1: name = "SCHED_FIFO"; break;
    case 2: name = "SCHED_RR"; break;
    case 3: name = "SCHED_BATCH"; break;
    case 5: name = "SCHED_IDLE"; break;
    case 6: name = "SCHED_DEADLINE"; break;
    default: name = "UNKNOWN"; break;
  }
  snprintf(buf, buflen, "%s(%d)", name, pol);
  return true;
}

static int cmp_int(const void *a, const void *b) {
  int ia = *(const int *)a, ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
    return 2;
  }

  pid_t pid = (pid_t)strtol(argv[1], NULL, 10);
  if (pid <= 0) {
    fprintf(stderr, "Invalid pid: %s\n", argv[1]);
    return 2;
  }

  char task_dir[256];
  snprintf(task_dir, sizeof(task_dir), "/proc/%d/task", pid);
  if (access(task_dir, R_OK | X_OK) != 0) {
    fprintf(stderr, "Cannot access %s: %s\n", task_dir, strerror(errno));
    return 1;
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);

  const int refresh_ms = 250;

  while (1) {
    int ch = getch();
    if (ch == 'q' || ch == 'Q') break;

    clear();
    mvprintw(0, 0, "sched_monitor pid=%d  (q to quit)  refresh=%dms", pid, refresh_ms);

    // Collect TIDs.
    DIR *d = opendir(task_dir);
    if (!d) {
      mvprintw(2, 0, "Failed to open %s: %s", task_dir, strerror(errno));
      refresh();
      usleep((useconds_t)refresh_ms * 1000);
      continue;
    }

    int tids[4096];
    int nt = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
      if (de->d_name[0] < '0' || de->d_name[0] > '9') continue;
      if (nt < (int)(sizeof(tids) / sizeof(tids[0]))) {
        tids[nt++] = atoi(de->d_name);
      }
    }
    closedir(d);
    qsort(tids, (size_t)nt, sizeof(tids[0]), cmp_int);

    mvprintw(2, 0, "%-8s %-16s %-14s %-14s", "TID", "POLICY", "vol_switches", "invol_switches");

    int row = 3;
    for (int i = 0; i < nt && row < LINES - 1; i++) {
      int tid = tids[i];
      char sched_path[512];
      snprintf(sched_path, sizeof(sched_path), "%s/%d/sched", task_dir, tid);

      unsigned long long vol = 0, invol = 0;
      (void)read_kv_u64(sched_path, "nr_voluntary_switches", &vol);
      (void)read_kv_u64(sched_path, "nr_involuntary_switches", &invol);

      char polbuf[64];
      if (!read_policy(sched_path, polbuf, sizeof(polbuf))) {
        snprintf(polbuf, sizeof(polbuf), "N/A");
      }

      mvprintw(row++, 0, "%-8d %-16s %-14llu %-14llu", tid, polbuf, vol, invol);
    }

    refresh();
    usleep((useconds_t)refresh_ms * 1000);
  }

  endwin();
  return 0;
}

