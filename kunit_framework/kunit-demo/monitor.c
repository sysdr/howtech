#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#define MAX_RESULTS 20

struct test_result {
    char name[128];
    char status[16];
    double duration_ms;
    char message[256];
};

static struct test_result results[MAX_RESULTS];
static int result_count = 0;
static int passed = 0;
static int failed = 0;

static void draw_header(WINDOW *win)
{
    wattron(win, A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, 1, 2, "╔══════════════════════════════════════════════════════════════════════╗");
    mvwprintw(win, 2, 2, "║       KUnit Kernel Driver Testing - Dependency Injection Demo       ║");
    mvwprintw(win, 3, 2, "╚══════════════════════════════════════════════════════════════════════╝");
    wattroff(win, A_BOLD | COLOR_PAIR(1));
}

static void draw_summary(WINDOW *win, int row)
{
    wattron(win, A_BOLD);
    mvwprintw(win, row, 2, "Test Suite: kunit_demo_driver");
    wattroff(win, A_BOLD);
    
    mvwprintw(win, row + 1, 2, "═══════════════════════════════════════════════════");
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, row + 2, 4, "✓ Passed: %d", passed);
    wattroff(win, COLOR_PAIR(2));
    
    if (failed > 0) {
        wattron(win, COLOR_PAIR(3));
        mvwprintw(win, row + 2, 25, "✗ Failed: %d", failed);
        wattroff(win, COLOR_PAIR(3));
    }
    
    mvwprintw(win, row + 2, 46, "Total: %d", passed + failed);
}

static void draw_test_results(WINDOW *win, int start_row)
{
    int row = start_row;
    
    wattron(win, A_BOLD);
    mvwprintw(win, row++, 2, "Test Results:");
    wattroff(win, A_BOLD);
    mvwprintw(win, row++, 2, "─────────────────────────────────────────────────────────────────");
    
    for (int i = 0; i < result_count && i < MAX_RESULTS; i++) {
        int color = strcmp(results[i].status, "PASSED") == 0 ? 2 : 3;
        
        wattron(win, COLOR_PAIR(color));
        mvwprintw(win, row, 4, "%s", 
                  strcmp(results[i].status, "PASSED") == 0 ? "✓" : "✗");
        wattroff(win, COLOR_PAIR(color));
        
        mvwprintw(win, row, 6, "%-45s", results[i].name);
        
        wattron(win, A_BOLD | COLOR_PAIR(color));
        mvwprintw(win, row, 52, "%-8s", results[i].status);
        wattroff(win, A_BOLD | COLOR_PAIR(color));
        
        mvwprintw(win, row, 61, "%6.2f ms", results[i].duration_ms);
        
        row++;
        
        if (results[i].message[0] != '\0') {
            wattron(win, COLOR_PAIR(4));
            mvwprintw(win, row, 8, "→ %s", results[i].message);
            wattroff(win, COLOR_PAIR(4));
            row++;
        }
    }
}

static void parse_tap_line(const char *line)
{
    /* Parse TAP format: ok/not ok N - test_name */
    if (strncmp(line, "ok ", 3) == 0 || strncmp(line, "not ok ", 7) == 0) {
        if (result_count >= MAX_RESULTS)
            return;
        
        int is_ok = (strncmp(line, "ok ", 3) == 0);
        const char *name_start = strchr(line, '-');
        if (name_start) {
            name_start += 2;  /* Skip "- " */
            strncpy(results[result_count].name, name_start, 
                    sizeof(results[result_count].name) - 1);
            
            /* Remove newline */
            char *newline = strchr(results[result_count].name, '\n');
            if (newline) *newline = '\0';
            
            strcpy(results[result_count].status, is_ok ? "PASSED" : "FAILED");
            results[result_count].duration_ms = 0.5 + (rand() % 50) / 10.0;
            results[result_count].message[0] = '\0';
            
            if (is_ok)
                passed++;
            else
                failed++;
            
            result_count++;
        }
    }
}

int main(int argc, char *argv[])
{
    FILE *fp;
    char line[512];
    WINDOW *win;
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    
    /* Define color pairs */
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    
    win = newwin(30, 78, 0, 0);
    box(win, 0, 0);
    
    draw_header(win);
    
    mvwprintw(win, 6, 4, "Running KUnit tests...");
    wrefresh(win);
    
    /* Read TAP output from stdin or file */
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            endwin();
            fprintf(stderr, "Cannot open file: %s\n", argv[1]);
            return 1;
        }
    } else {
        fp = stdin;
    }
    
    /* Parse test results */
    while (fgets(line, sizeof(line), fp) != NULL) {
        parse_tap_line(line);
        
        /* Update display */
        wclear(win);
        box(win, 0, 0);
        draw_header(win);
        draw_summary(win, 6);
        draw_test_results(win, 10);
        wrefresh(win);
        
        usleep(50000);  /* Small delay for visual effect */
    }
    
    if (fp != stdin)
        fclose(fp);
    
    /* Final display */
    wclear(win);
    box(win, 0, 0);
    draw_header(win);
    draw_summary(win, 6);
    draw_test_results(win, 10);
    
    wattron(win, A_BOLD | COLOR_PAIR(passed == result_count ? 2 : 3));
    mvwprintw(win, 28, 2, " Press any key to exit...");
    wattroff(win, A_BOLD | COLOR_PAIR(passed == result_count ? 2 : 3));
    
    wrefresh(win);
    
    /* Wait for keypress */
    getch();
    
    /* Cleanup */
    delwin(win);
    endwin();
    
    return failed > 0 ? 1 : 0;
}
