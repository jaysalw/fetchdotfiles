#ifndef FILE_OPS_H
#define FILE_OPS_H

int place_dotfile(const char *src, const char *dest, int force);
int file_exists(const char *path);
void print_colored_warning(const char *msg);
int rollback_session_start(void);
int perform_rollback(void);

#endif // FILE_OPS_H
