#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "file_ops.h"
#include "diff_viewer.h"

#define ROLLBACK_PATH_MAX 4096

// ANSI Color Codes
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define DRED     "\033[0;31m"
#define GREEN    "\033[1;32m"
#define DGREEN   "\033[0;32m"
#define YELLOW   "\033[1;33m"
#define DYELLOW  "\033[0;33m"
#define CYAN     "\033[1;36m"
#define DCYAN    "\033[0;36m"

static char rollback_root[ROLLBACK_PATH_MAX] = {0};
static char rollback_manifest[ROLLBACK_PATH_MAX] = {0};
static char rollback_backups[ROLLBACK_PATH_MAX] = {0};
static int rollback_index = 0;

static int build_path(char *out, size_t out_size, const char *left, const char *right) {
    size_t left_len;
    size_t right_len;

    if (!out || out_size == 0 || !left || !right) {
        return 1;
    }

    left_len = strlen(left);
    right_len = strlen(right);

    if (left_len + 1 + right_len + 1 > out_size) {
        return 1;
    }

    memcpy(out, left, left_len);
    out[left_len] = '/';
    memcpy(out + left_len + 1, right, right_len + 1);
    return 0;
}

static int ensure_dir(const char *path) {
    char tmp[ROLLBACK_PATH_MAX];
    size_t len;

    if (!path || !*path) {
        return 1;
    }

    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    len = strlen(tmp);

    if (len == 0) {
        return 1;
    }

    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
                return 1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
        return 1;
    }

    return 0;
}

static int copy_file_binary(const char *src, const char *dest) {
    FILE *f_src = fopen(src, "rb");
    FILE *f_dest;
    char buf[4096];
    size_t n;

    if (!f_src) {
        return 1;
    }

    f_dest = fopen(dest, "wb");
    if (!f_dest) {
        fclose(f_src);
        return 1;
    }

    while ((n = fread(buf, 1, sizeof(buf), f_src)) > 0) {
        if (fwrite(buf, 1, n, f_dest) != n) {
            fclose(f_src);
            fclose(f_dest);
            return 1;
        }
    }

    fclose(f_src);
    fclose(f_dest);
    return 0;
}

static int append_manifest_entry(const char *entry_type, const char *dest, const char *backup) {
    FILE *manifest;

    manifest = fopen(rollback_manifest, "a");
    if (!manifest) {
        return 1;
    }

    if (backup) {
        fprintf(manifest, "%s\t%s\t%s\n", entry_type, dest, backup);
    } else {
        fprintf(manifest, "%s\t%s\n", entry_type, dest);
    }

    fclose(manifest);
    return 0;
}

static int record_prechange_state(const char *dest) {
    char backup_path[ROLLBACK_PATH_MAX];
    char backup_name[64];
    int written;

    if (file_exists(dest)) {
        written = snprintf(backup_name, sizeof(backup_name), "backup_%06d", rollback_index++);
        if (written < 0 || (size_t)written >= sizeof(backup_name)) {
            return 1;
        }
        if (build_path(backup_path, sizeof(backup_path), rollback_backups, backup_name) != 0) {
            return 1;
        }
        if (copy_file_binary(dest, backup_path) != 0) {
            return 1;
        }
        return append_manifest_entry("RESTORE", dest, backup_path);
    }

    return append_manifest_entry("DELETE", dest, NULL);
}

int rollback_session_start(void) {
    const char *home = getenv("HOME");
    FILE *manifest;

    if (!home || !*home) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " HOME is not set; rollback unavailable\n");
        return 1;
    }

    if (build_path(rollback_root, sizeof(rollback_root), home, ".fdf_rollback") != 0 ||
        build_path(rollback_backups, sizeof(rollback_backups), rollback_root, "backups") != 0 ||
        build_path(rollback_manifest, sizeof(rollback_manifest), rollback_root, "manifest.log") != 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Rollback paths are too long\n");
        return 1;
    }
    rollback_index = 0;

    if (ensure_dir(rollback_root) != 0 || ensure_dir(rollback_backups) != 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to initialize rollback storage\n");
        return 1;
    }

    manifest = fopen(rollback_manifest, "w");
    if (!manifest) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to create rollback manifest\n");
        return 1;
    }
    fclose(manifest);

    return 0;
}

int perform_rollback(void) {
    const char *home = getenv("HOME");
    FILE *manifest;
    char line[ROLLBACK_PATH_MAX * 2];
    char **entries = NULL;
    size_t count = 0;
    size_t capacity = 0;
    int failures = 0;

    if (!home || !*home) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " HOME is not set; rollback unavailable\n");
        return 1;
    }

    if (build_path(rollback_root, sizeof(rollback_root), home, ".fdf_rollback") != 0 ||
        build_path(rollback_manifest, sizeof(rollback_manifest), rollback_root, "manifest.log") != 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Rollback paths are too long\n");
        return 1;
    }

    manifest = fopen(rollback_manifest, "r");
    if (!manifest) {
        printf(DYELLOW "[" YELLOW "INFO" DYELLOW "]" RESET " No rollback data found\n");
        return 1;
    }

    while (fgets(line, sizeof(line), manifest)) {
        size_t len = strlen(line);
        char *copy;

        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **new_entries = realloc(entries, new_capacity * sizeof(char *));
            if (!new_entries) {
                fclose(manifest);
                for (size_t j = 0; j < count; j++) {
                    free(entries[j]);
                }
                free(entries);
                return 1;
            }
            entries = new_entries;
            capacity = new_capacity;
        }

        copy = malloc(strlen(line) + 1);
        if (!copy) {
            fclose(manifest);
            for (size_t j = 0; j < count; j++) {
                free(entries[j]);
            }
            free(entries);
            return 1;
        }
        strcpy(copy, line);
        entries[count++] = copy;
    }
    fclose(manifest);

    if (count == 0) {
        printf(DYELLOW "[" YELLOW "INFO" DYELLOW "]" RESET " Nothing to rollback\n");
        free(entries);
        return 1;
    }

    for (ssize_t i = (ssize_t)count - 1; i >= 0; i--) {
        char *saveptr = NULL;
        char *kind = strtok_r(entries[i], "\t", &saveptr);
        char *dest = strtok_r(NULL, "\t", &saveptr);
        char *backup = strtok_r(NULL, "\t", &saveptr);

        if (!kind || !dest) {
            failures++;
            continue;
        }

        if (strcmp(kind, "RESTORE") == 0) {
            if (!backup || copy_file_binary(backup, dest) != 0) {
                printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to restore %s\n", dest);
                failures++;
            } else {
                printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Restored %s\n", dest);
            }
        } else if (strcmp(kind, "DELETE") == 0) {
            if (remove(dest) != 0 && errno != ENOENT) {
                printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to remove %s\n", dest);
                failures++;
            } else {
                printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Removed %s\n", dest);
            }
        } else {
            failures++;
        }
    }

    for (size_t i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);

    if (failures > 0) {
        printf(DRED "[" RED "FAILED" DRED "]" RESET " Rollback finished with %d error(s)\n", failures);
        return 1;
    }

    printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Rollback completed\n");
    return 0;
}

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void print_colored_warning(const char *msg) {
    printf(DYELLOW "[" YELLOW "WARNING" DYELLOW "]" RESET " %s\n", msg);
}

int place_dotfile(const char *src, const char *dest, int force) {
    // Check if source file exists first
    if (!file_exists(src)) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Source file not found: %s\n", src);
        return 1;
    }
    
    // Show diff if enabled and destination exists
    if (is_diff_enabled() && file_exists(dest)) {
        printf(DCYAN "[" CYAN "DIFF" DCYAN "]" RESET " Showing diff for %s\n", dest);
        show_diff_tui(dest, src);
    }
    
    if (file_exists(dest) && !force) {
        print_colored_warning("A dot file already exists:");
        printf("            DOTFILE - %s\n", dest);
        printf("            Overwrite? [Y/N]: ");
        char ans = getchar();
        while (getchar() != '\n');
        if (ans != 'Y' && ans != 'y') {
            printf(DYELLOW "[" YELLOW "INFO" DYELLOW "]" RESET " Skipping %s\n", dest);
            return 0;  // Not an error, user chose to skip
        }
    }

    if (record_prechange_state(dest) != 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to record rollback state for: %s\n", dest);
        return 1;
    }

    FILE *f_src = fopen(src, "rb");
    FILE *f_dest = fopen(dest, "wb");
    if (!f_src) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Cannot read source: %s\n", src);
        return 1;
    }
    if (!f_dest) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Cannot write to destination: %s\n", dest);
        printf("            (Check permissions or if parent directory exists)\n");
        fclose(f_src);
        return 1;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f_src)) > 0) {
        fwrite(buf, 1, n, f_dest);
    }
    fclose(f_src);
    fclose(f_dest);
    printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Placed %s -> %s\n", src, dest);
    return 0;
}
