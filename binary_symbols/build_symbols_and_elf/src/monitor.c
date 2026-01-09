#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_SYMBOLS 100
#define MAX_LINE 256

typedef struct {
    char type;
    char binding;
    char name[128];
    char section[32];
} Symbol;

void draw_border(WINDOW *win, int height, int width) {
    box(win, 0, 0);
    wrefresh(win);
}

void read_symbols(const char* binary, Symbol* symbols, int* count) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "nm -C %s 2>/dev/null", binary);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) return;
    
    char line[MAX_LINE];
    *count = 0;
    
    while (fgets(line, sizeof(line), fp) && *count < MAX_SYMBOLS) {
        char addr[32], type, name[128];
        if (sscanf(line, "%s %c %[^\n]", addr, &type, name) == 3) {
            symbols[*count].type = type;
            strncpy(symbols[*count].name, name, 127);
            symbols[*count].name[127] = '\0';
            (*count)++;
        }
    }
    
    pclose(fp);
}

const char* get_type_description(char type) {
    switch(type) {
        case 'T': return "Global Code (text)";
        case 't': return "Local Code (text)";
        case 'D': return "Global Data";
        case 'd': return "Local Data";
        case 'B': return "Uninitialized Data (BSS)";
        case 'b': return "Local Uninitialized";
        case 'U': return "Undefined (external)";
        case 'W': return "Weak Symbol";
        case 'w': return "Weak (local)";
        case 'C': return "Common (uninitialized)";
        case 'R': return "Read-only Data";
        case 'r': return "Local Read-only";
        default: return "Other";
    }
}

int get_color_pair(char type) {
    switch(type) {
        case 'T': case 't': return 1; // Green
        case 'D': case 'd': return 2; // Blue  
        case 'U': return 3; // Red
        case 'W': case 'w': return 4; // Yellow
        case 'B': case 'b': case 'C': return 5; // Magenta
        default: return 6; // Cyan
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        return 1;
    }
    
    const char* binary = argv[1];
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(2000); // 2 second refresh
    
    // Initialize color pairs
    init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Code
    init_pair(2, COLOR_BLUE, COLOR_BLACK);    // Data
    init_pair(3, COLOR_RED, COLOR_BLACK);     // Undefined
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Weak
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // BSS
    init_pair(6, COLOR_CYAN, COLOR_BLACK);    // Other
    
    Symbol symbols[MAX_SYMBOLS];
    int symbol_count = 0;
    int scroll_offset = 0;
    
    while (1) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == KEY_DOWN && scroll_offset < symbol_count - LINES + 10) scroll_offset++;
        if (ch == KEY_UP && scroll_offset > 0) scroll_offset--;
        
        clear();
        
        // Header
        attron(A_BOLD | COLOR_PAIR(6));
        mvprintw(0, 0, "═══════════════════════════════════════════════════════════════════════════");
        mvprintw(1, 2, "Binary Symbol Analysis Monitor - %s", binary);
        mvprintw(2, 0, "═══════════════════════════════════════════════════════════════════════════");
        attroff(A_BOLD | COLOR_PAIR(6));
        
        // Read symbols
        read_symbols(binary, symbols, &symbol_count);
        
        // Count by type
        int counts[256] = {0};
        for (int i = 0; i < symbol_count; i++) {
            counts[(unsigned char)symbols[i].type]++;
        }
        
        // Statistics
        attron(COLOR_PAIR(6));
        mvprintw(4, 2, "Total Symbols: %d", symbol_count);
        attroff(COLOR_PAIR(6));
        
        int row = 5;
        attron(COLOR_PAIR(1));
        mvprintw(row++, 2, "Global Code (T): %d", counts['T']);
        attroff(COLOR_PAIR(1));
        
        attron(COLOR_PAIR(2));
        mvprintw(row++, 2, "Global Data (D): %d", counts['D']);
        attroff(COLOR_PAIR(2));
        
        attron(COLOR_PAIR(3));
        mvprintw(row++, 2, "Undefined (U):   %d", counts['U']);
        attroff(COLOR_PAIR(3));
        
        attron(COLOR_PAIR(4));
        mvprintw(row++, 2, "Weak Symbols (W): %d", counts['W']);
        attroff(COLOR_PAIR(4));
        
        attron(COLOR_PAIR(5));
        mvprintw(row++, 2, "BSS/Common (B/C): %d", counts['B'] + counts['C']);
        attroff(COLOR_PAIR(5));
        
        // Symbol list header
        row += 2;
        attron(A_BOLD);
        mvprintw(row++, 2, "Type  Symbol Name                                    Description");
        mvprintw(row++, 2, "────  ──────────────────────────────────────────    ─────────────────────");
        attroff(A_BOLD);
        
        // Display symbols
        int display_count = LINES - row - 3;
        for (int i = scroll_offset; i < symbol_count && i < scroll_offset + display_count; i++) {
            int color = get_color_pair(symbols[i].type);
            attron(COLOR_PAIR(color));
            mvprintw(row++, 2, " %c    %-45.45s  %s", 
                     symbols[i].type, 
                     symbols[i].name,
                     get_type_description(symbols[i].type));
            attroff(COLOR_PAIR(color));
        }
        
        // Footer
        attron(A_BOLD | COLOR_PAIR(6));
        mvprintw(LINES-2, 2, "Controls: ↑/↓ scroll | q quit | Auto-refresh every 2s");
        attroff(A_BOLD | COLOR_PAIR(6));
        
        refresh();
    }
    
    endwin();
    return 0;
}
