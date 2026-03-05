#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "docs_viewer.h"

#define MAX_LINES 10000
#define MAX_LINE_LEN 1024

typedef struct {
    char **lines;
    int count;
} TextFile;

static TextFile load_text_file(const char *filename) {
    TextFile tf = {NULL, 0};
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return tf;
    }

    tf.lines = malloc(sizeof(char *) * MAX_LINES);
    if (!tf.lines) {
        fclose(fp);
        return tf;
    }

    char buf[MAX_LINE_LEN];
    while (fgets(buf, sizeof(buf), fp) && tf.count < MAX_LINES) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }

        tf.lines[tf.count] = malloc(strlen(buf) + 1);
        if (!tf.lines[tf.count]) {
            fclose(fp);
            return tf;
        }
        strcpy(tf.lines[tf.count], buf);
        tf.count++;
    }

    fclose(fp);
    return tf;
}

static void free_text_file(TextFile *tf) {
    if (tf->lines) {
        for (int i = 0; i < tf->count; i++) {
            free(tf->lines[i]);
        }
        free(tf->lines);
        tf->lines = NULL;
        tf->count = 0;
    }
}

int view_docs(const char *filename) {
    TextFile tf = load_text_file(filename);
    if (!tf.lines || tf.count == 0) {
        fprintf(stderr, "Error: Could not load file '%s'\n", filename);
        free_text_file(&tf);
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int scroll_offset = 0;
    int running = 1;

    while (running) {
        clear();

        // Display current view
        int display_lines = (max_y - 1);
        for (int i = 0; i < display_lines && scroll_offset + i < tf.count; i++) {
            const char *line = tf.lines[scroll_offset + i];
            size_t line_len = strlen(line);

            // Truncate or wrap long lines
            if (line_len > (size_t)max_x) {
                mvaddnstr(i, 0, line, max_x - 1);
            } else {
                mvaddstr(i, 0, line);
            }
        }

        // Display footer with navigation hint
        attron(A_REVERSE);
        mvaddstr(max_y - 1, 0, "q/ESC: Quit | ↑/↓: Scroll | PgUp/PgDn: Page | Home/End: Jump");
        attroff(A_REVERSE);

        refresh();

        int ch = getch();
        switch (ch) {
            case 'q':
            case 27:  // ESC
                running = 0;
                break;
            case KEY_UP:
                if (scroll_offset > 0) {
                    scroll_offset--;
                }
                break;
            case KEY_DOWN:
                if (scroll_offset < tf.count - 1) {
                    scroll_offset++;
                }
                break;
            case KEY_PPAGE:  // Page Up
                scroll_offset -= (max_y - 2);
                if (scroll_offset < 0) {
                    scroll_offset = 0;
                }
                break;
            case KEY_NPAGE:  // Page Down
                scroll_offset += (max_y - 2);
                if (scroll_offset >= tf.count) {
                    scroll_offset = tf.count - 1;
                }
                break;
            case KEY_HOME:
                scroll_offset = 0;
                break;
            case KEY_END:
                scroll_offset = tf.count - (max_y - 2);
                if (scroll_offset < 0) {
                    scroll_offset = 0;
                }
                break;
            default:
                break;
        }
    }

    endwin();
    free_text_file(&tf);
    return 0;
}
