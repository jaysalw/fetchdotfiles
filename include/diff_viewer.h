#ifndef DIFF_VIEWER_H
#define DIFF_VIEWER_H

/**
 * Show a side-by-side diff of two files using ncurses TUI
 * @param old_file Path to the existing file (destination)
 * @param new_file Path to the new file (source)
 * @return 0 on success, 1 on error
 */
int show_diff_tui(const char *old_file, const char *new_file);

/**
 * Set whether diff viewer is enabled globally
 * @param enabled 1 to enable, 0 to disable
 */
void set_diff_enabled(int enabled);

/**
 * Check if diff viewer is enabled
 * @return 1 if enabled, 0 if disabled
 */
int is_diff_enabled(void);

#endif // DIFF_VIEWER_H
