#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "diff_viewer.h"

// Global flag for diff mode
static int diff_enabled = 0;

void set_diff_enabled(int enabled) {
    diff_enabled = enabled;
}

int is_diff_enabled(void) {
    return diff_enabled;
}

// Maximum lines to load
#define MAX_LINES 10000
#define MAX_LINE_LEN 1024

typedef struct {
    char **lines;
    int count;
} FileContent;

static FileContent load_file(const char *path) {
    FileContent fc = {NULL, 0};
    FILE *fp = fopen(path, "r");
    if (!fp) return fc;

    fc.lines = malloc(sizeof(char*) * MAX_LINES);
    if (!fc.lines) {
        fclose(fp);
        return fc;
    }

    char buf[MAX_LINE_LEN];
    while (fgets(buf, sizeof(buf), fp) && fc.count < MAX_LINES) {
        // Remove trailing newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        
        fc.lines[fc.count] = strdup(buf);
        fc.count++;
    }
    fclose(fp);
    return fc;
}

static void free_file_content(FileContent *fc) {
    if (fc->lines) {
        for (int i = 0; i < fc->count; i++) {
            free(fc->lines[i]);
        }
        free(fc->lines);
        fc->lines = NULL;
        fc->count = 0;
    }
}

// Simple diff status for each line
typedef enum {
    DIFF_SAME,
    DIFF_CHANGED,
    DIFF_ADDED,
    DIFF_REMOVED
} DiffStatus;

typedef struct {
    char *old_line;
    char *new_line;
    DiffStatus status;
} DiffLine;

typedef struct {
    DiffLine *lines;
    int count;
} DiffResult;

// Simple line-by-line diff (not LCS for simplicity)
static DiffResult compute_diff(FileContent *old, FileContent *new) {
    DiffResult result = {NULL, 0};
    int max_lines = old->count + new->count;
    result.lines = malloc(sizeof(DiffLine) * (max_lines + 1));
    if (!result.lines) return result;

    int o = 0, n = 0;
    
    while (o < old->count || n < new->count) {
        DiffLine *dl = &result.lines[result.count];
        
        if (o >= old->count) {
            // Only new lines left
            dl->old_line = strdup("");
            dl->new_line = strdup(new->lines[n]);
            dl->status = DIFF_ADDED;
            n++;
        } else if (n >= new->count) {
            // Only old lines left
            dl->old_line = strdup(old->lines[o]);
            dl->new_line = strdup("");
            dl->status = DIFF_REMOVED;
            o++;
        } else if (strcmp(old->lines[o], new->lines[n]) == 0) {
            // Lines match
            dl->old_line = strdup(old->lines[o]);
            dl->new_line = strdup(new->lines[n]);
            dl->status = DIFF_SAME;
            o++;
            n++;
        } else {
            // Lines differ - check if it's a change or add/remove
            // Simple heuristic: look ahead for matches
            int found_old = 0, found_new = 0;
            for (int i = n; i < new->count && i < n + 3; i++) {
                if (strcmp(old->lines[o], new->lines[i]) == 0) {
                    found_old = i - n;
                    break;
                }
            }
            for (int i = o; i < old->count && i < o + 3; i++) {
                if (strcmp(old->lines[i], new->lines[n]) == 0) {
                    found_new = i - o;
                    break;
                }
            }
            
            if (found_old == 0 && found_new == 0) {
                // Changed line
                dl->old_line = strdup(old->lines[o]);
                dl->new_line = strdup(new->lines[n]);
                dl->status = DIFF_CHANGED;
                o++;
                n++;
            } else if (found_new > 0 && (found_old == 0 || found_new <= found_old)) {
                // Line was removed
                dl->old_line = strdup(old->lines[o]);
                dl->new_line = strdup("");
                dl->status = DIFF_REMOVED;
                o++;
            } else {
                // Line was added
                dl->old_line = strdup("");
                dl->new_line = strdup(new->lines[n]);
                dl->status = DIFF_ADDED;
                n++;
            }
        }
        result.count++;
    }
    
    return result;
}

static void free_diff_result(DiffResult *dr) {
    if (dr->lines) {
        for (int i = 0; i < dr->count; i++) {
            free(dr->lines[i].old_line);
            free(dr->lines[i].new_line);
        }
        free(dr->lines);
        dr->lines = NULL;
        dr->count = 0;
    }
}

int show_diff_tui(const char *old_file, const char *new_file) {
    FileContent old_content = load_file(old_file);
    FileContent new_content = load_file(new_file);
    
    // Handle case where old file doesn't exist (new file)
    int old_exists = (old_content.lines != NULL);
    if (!old_exists) {
        old_content.lines = malloc(sizeof(char*));
        old_content.count = 0;
    }
    
    if (!new_content.lines) {
        fprintf(stderr, "Error: Cannot read new file: %s\n", new_file);
        free_file_content(&old_content);
        return 1;
    }
    
    DiffResult diff = compute_diff(&old_content, &new_content);
    if (!diff.lines) {
        fprintf(stderr, "Error: Failed to compute diff\n");
        free_file_content(&old_content);
        free_file_content(&new_content);
        return 1;
    }
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Define color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // Normal
    init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Added
    init_pair(3, COLOR_RED, COLOR_BLACK);     // Removed
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Changed
    init_pair(5, COLOR_CYAN, COLOR_BLACK);    // Header
    init_pair(6, COLOR_BLACK, COLOR_WHITE);   // Status bar
    
    int scroll_pos = 0;
    int max_scroll = diff.count > 0 ? diff.count - 1 : 0;
    int running = 1;
    
    while (running) {
        clear();
        
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int half_width = (max_x - 3) / 2;
        int content_height = max_y - 4;  // Leave room for header and footer
        
        // Draw header
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(0, 0, " DIFF VIEWER ");
        attroff(COLOR_PAIR(5) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(0, 14, "| Old: %.*s", max_x - 20, old_exists ? old_file : "(new file)");
        attroff(COLOR_PAIR(1));
        
        // Draw column headers
        attron(COLOR_PAIR(5));
        mvprintw(1, 0, "%-*.*s", half_width, half_width, " EXISTING FILE");
        mvprintw(1, half_width + 3, "%-*.*s", half_width, half_width, " NEW FILE");
        attroff(COLOR_PAIR(5));
        
        // Draw separator
        for (int y = 1; y < max_y - 1; y++) {
            mvaddch(y, half_width + 1, ACS_VLINE);
        }
        
        // Draw diff content
        for (int i = 0; i < content_height && (scroll_pos + i) < diff.count; i++) {
            int idx = scroll_pos + i;
            DiffLine *dl = &diff.lines[idx];
            
            int y = i + 2;
            int color_pair = 1;
            char prefix_old = ' ', prefix_new = ' ';
            
            switch (dl->status) {
                case DIFF_SAME:
                    color_pair = 1;
                    break;
                case DIFF_ADDED:
                    color_pair = 2;
                    prefix_new = '+';
                    break;
                case DIFF_REMOVED:
                    color_pair = 3;
                    prefix_old = '-';
                    break;
                case DIFF_CHANGED:
                    color_pair = 4;
                    prefix_old = '~';
                    prefix_new = '~';
                    break;
            }
            
            // Left side (old)
            attron(COLOR_PAIR(dl->status == DIFF_REMOVED || dl->status == DIFF_CHANGED ? color_pair : 1));
            mvprintw(y, 0, "%c%-*.*s", prefix_old, half_width - 1, half_width - 1, dl->old_line);
            attroff(COLOR_PAIR(dl->status == DIFF_REMOVED || dl->status == DIFF_CHANGED ? color_pair : 1));
            
            // Right side (new)
            attron(COLOR_PAIR(dl->status == DIFF_ADDED || dl->status == DIFF_CHANGED ? color_pair : 1));
            mvprintw(y, half_width + 3, "%c%-*.*s", prefix_new, half_width - 1, half_width - 1, dl->new_line);
            attroff(COLOR_PAIR(dl->status == DIFF_ADDED || dl->status == DIFF_CHANGED ? color_pair : 1));
        }
        
        // Draw status bar
        attron(COLOR_PAIR(6));
        mvprintw(max_y - 2, 0, "%-*s", max_x, "");
        mvprintw(max_y - 2, 1, "Line %d/%d | ", scroll_pos + 1, diff.count > 0 ? diff.count : 1);
        
        // Legend
        attroff(COLOR_PAIR(6));
        attron(COLOR_PAIR(2));
        printw("+Added ");
        attroff(COLOR_PAIR(2));
        attron(COLOR_PAIR(3));
        printw("-Removed ");
        attroff(COLOR_PAIR(3));
        attron(COLOR_PAIR(4));
        printw("~Changed");
        attroff(COLOR_PAIR(4));
        
        // Help bar
        attron(COLOR_PAIR(5));
        mvprintw(max_y - 1, 0, " [↑/↓] Scroll | [PgUp/PgDn] Page | [Home/End] Jump | [q] Close ");
        attroff(COLOR_PAIR(5));
        
        refresh();
        
        // Handle input
        int ch = getch();
        switch (ch) {
            case 'q':
            case 'Q':
            case 27:  // ESC
                running = 0;
                break;
            case KEY_UP:
            case 'k':
                if (scroll_pos > 0) scroll_pos--;
                break;
            case KEY_DOWN:
            case 'j':
                if (scroll_pos < max_scroll) scroll_pos++;
                break;
            case KEY_PPAGE:  // Page Up
                scroll_pos -= content_height;
                if (scroll_pos < 0) scroll_pos = 0;
                break;
            case KEY_NPAGE:  // Page Down
                scroll_pos += content_height;
                if (scroll_pos > max_scroll) scroll_pos = max_scroll;
                break;
            case KEY_HOME:
            case 'g':
                scroll_pos = 0;
                break;
            case KEY_END:
            case 'G':
                scroll_pos = max_scroll;
                break;
        }
    }
    
    endwin();
    
    // Cleanup
    free_diff_result(&diff);
    free_file_content(&old_content);
    free_file_content(&new_content);
    
    return 0;
}
