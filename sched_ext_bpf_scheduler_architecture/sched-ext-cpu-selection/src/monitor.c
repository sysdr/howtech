// SPDX-License-Identifier: GPL-2.0
/* Real-time Scheduler Monitor */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include "scheduler.h"

#define MAX_CPUS 256
#define MAX_LATENCY_SAMPLES 10000

struct latency_stats {
    unsigned long count;
    unsigned long long total_ns;
    unsigned long long min_ns;
    unsigned long long max_ns;
    unsigned long long samples[MAX_LATENCY_SAMPLES];
    unsigned long sample_idx;
};

static struct latency_stats group_stats[2];
static volatile sig_atomic_t stop = 0;
static int events_fd = -1;

static void sig_handler(int sig)
{
    stop = 1;
}

static void update_latency_stats(struct latency_event *event)
{
    if (event->group_id >= 2)
        return;
    
    struct latency_stats *stats = &group_stats[event->group_id];
    
    stats->count++;
    stats->total_ns += event->latency_ns;
    
    if (stats->count == 1 || event->latency_ns < stats->min_ns)
        stats->min_ns = event->latency_ns;
    
    if (event->latency_ns > stats->max_ns)
        stats->max_ns = event->latency_ns;
    
    if (stats->sample_idx < MAX_LATENCY_SAMPLES) {
        stats->samples[stats->sample_idx++] = event->latency_ns;
    }
}

static int compare_u64(const void *a, const void *b)
{
    unsigned long long ua = *(const unsigned long long *)a;
    unsigned long long ub = *(const unsigned long long *)b;
    return (ua > ub) - (ua < ub);
}

static unsigned long long calculate_percentile(struct latency_stats *stats, int percentile)
{
    if (stats->sample_idx == 0)
        return 0;
    
    qsort(stats->samples, stats->sample_idx, sizeof(unsigned long long), compare_u64);
    
    unsigned long idx = (stats->sample_idx * percentile) / 100;
    if (idx >= stats->sample_idx)
        idx = stats->sample_idx - 1;
    
    return stats->samples[idx];
}

static void draw_header(void)
{
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 0, "╔════════════════════════════════════════════════════════════════════════════╗");
    mvprintw(1, 0, "║        sched_ext CPU Selection Monitor - Real-Time Statistics             ║");
    mvprintw(2, 0, "╚════════════════════════════════════════════════════════════════════════════╝");
    attroff(COLOR_PAIR(1) | A_BOLD);
}

static void draw_group_stats(int row, int group_id, const char *name, struct latency_stats *stats)
{
    unsigned long long avg_ns = stats->count ? stats->total_ns / stats->count : 0;
    unsigned long long p50 = calculate_percentile(stats, 50);
    unsigned long long p95 = calculate_percentile(stats, 95);
    unsigned long long p99 = calculate_percentile(stats, 99);
    
    int color = COLOR_PAIR(2);
    if (group_id == 0) {
        if (p99 < 800000)
            color = COLOR_PAIR(3); /* Green - good */
        else if (p99 < 2000000)
            color = COLOR_PAIR(4); /* Yellow - borderline */
        else
            color = COLOR_PAIR(5); /* Red - bad */
    }
    
    attron(A_BOLD);
    mvprintw(row, 2, "Group %d: %s", group_id, name);
    attroff(A_BOLD);
    
    mvprintw(row + 1, 4, "Wakeup Count:  %lu", stats->count);
    
    attron(color);
    mvprintw(row + 2, 4, "Avg Latency:   %llu μs", avg_ns / 1000);
    mvprintw(row + 3, 4, "Min Latency:   %llu μs", stats->min_ns / 1000);
    mvprintw(row + 4, 4, "Max Latency:   %llu μs", stats->max_ns / 1000);
    attroff(color);
    
    mvprintw(row + 5, 4, "P50 Latency:   %llu μs", p50 / 1000);
    
    attron(color);
    mvprintw(row + 6, 4, "P95 Latency:   %llu μs", p95 / 1000);
    mvprintw(row + 7, 4, "P99 Latency:   %llu μs", p99 / 1000);
    attroff(color);
}

static void draw_cache_impact(void)
{
    int row = 22;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row, 2, "Cache Hierarchy Impact:");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(3));
    mvprintw(row + 1, 4, "L1/L2 (Same Core):    4-12 cycles   [Optimal]");
    attroff(COLOR_PAIR(3));
    
    attron(COLOR_PAIR(4));
    mvprintw(row + 2, 4, "L3 (Same Socket):     40-50 cycles  [Acceptable]");
    attroff(COLOR_PAIR(4));
    
    attron(COLOR_PAIR(5));
    mvprintw(row + 3, 4, "RAM (Cross-Socket):   200+ cycles   [Avoid]");
    attroff(COLOR_PAIR(5));
}

static void draw_footer(void)
{
    int row = 27;
    attron(COLOR_PAIR(2));
    mvprintw(row, 2, "Press Ctrl+C to exit");
    mvprintw(row + 1, 2, "Statistics update every 100ms");
    attroff(COLOR_PAIR(2));
}

static int handle_event(void *ctx, void *data, size_t data_sz)
{
    struct latency_event *event = data;
    update_latency_stats(event);
    return 0;
}

int main(int argc, char **argv)
{
    struct ring_buffer *rb = NULL;
    int err = 0;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <events_map_fd>\n", argv[0]);
        return 1;
    }
    
    events_fd = atoi(argv[1]);
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    /* Initialize ncurses */
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100);
    
    /* Color pairs */
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    
    /* Set up ring buffer */
    rb = ring_buffer__new(events_fd, handle_event, NULL, NULL);
    if (!rb) {
        endwin();
        fprintf(stderr, "Failed to create ring buffer\n");
        return 1;
    }
    
    while (!stop) {
        /* Poll ring buffer */
        ring_buffer__poll(rb, 100);
        
        /* Clear and redraw */
        clear();
        draw_header();
        draw_group_stats(4, 0, "Interactive (Low Latency)", &group_stats[0]);
        draw_group_stats(13, 1, "Batch (Throughput)", &group_stats[1]);
        draw_cache_impact();
        draw_footer();
        refresh();
        
        getch(); /* Non-blocking due to timeout() */
    }
    
    ring_buffer__free(rb);
    endwin();
    
    /* Print final summary */
    printf("\n=== Final Statistics ===\n");
    for (int i = 0; i < 2; i++) {
        printf("\nGroup %d:\n", i);
        printf("  Wakeup Count: %lu\n", group_stats[i].count);
        if (group_stats[i].count > 0) {
            unsigned long long avg = group_stats[i].total_ns / group_stats[i].count;
            printf("  Avg Latency:  %llu μs\n", avg / 1000);
            printf("  Min Latency:  %llu μs\n", group_stats[i].min_ns / 1000);
            printf("  Max Latency:  %llu μs\n", group_stats[i].max_ns / 1000);
            printf("  P99 Latency:  %llu μs\n", calculate_percentile(&group_stats[i], 99) / 1000);
        }
    }
    
    return 0;
}
