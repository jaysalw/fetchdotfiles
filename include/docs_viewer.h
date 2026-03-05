#ifndef DOCS_VIEWER_H
#define DOCS_VIEWER_H

/**
 * Display a text file in an ncurses TUI viewer
 * @param filename Path to the text file to display
 * @return 0 on success, 1 on error
 */
int view_docs(const char *filename);

#endif // DOCS_VIEWER_H
