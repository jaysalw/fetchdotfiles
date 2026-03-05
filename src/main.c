#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"
#include "diff_viewer.h"
#include "file_ops.h"
#include "docs_viewer.h"

// ANSI Color Codes
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define DRED     "\033[0;31m"
#define GREEN    "\033[1;32m"
#define DGREEN   "\033[0;32m"
#define YELLOW   "\033[1;33m"
#define DYELLOW  "\033[0;33m"
#define BLUE     "\033[1;34m"
#define DBLUE    "\033[0;34m"
#define CYAN     "\033[1;36m"
#define DCYAN    "\033[0;36m"

enum { MAX_FDF_FILES = 32, MAX_PATH_LEN = 512 };

static void print_usage(void) {
    printf("\n");
    printf(CYAN "FDF" RESET " - Fetch Dot Files\n");
    printf("-----------------------------------\n\n");
    printf("Website: " CYAN "https://fetchdots.net" RESET "\n\n");
    printf("Usage:\n");
    printf("  fdf --repo <repo_url> [--force-placement] [--show-diff]\n");
    printf("  fdf -r <repo_url> [-f] [-d]\n\n");
    printf("  fdf rollback\n");
    printf("  fdf docs\n\n");
    printf("Options:\n");
    printf("  -r, --repo             Repository URL (git/GitHub)\n");
    printf("  -f, --force-placement  Overwrite existing files\n");
    printf("  -d, --show-diff        Show diff in TUI before placing files\n");
    printf("  rollback               Undo latest file changes\n");
    printf("  docs                   View fdf documentation\n\n");
}

static void cleanup_repo_tmp(void) {
    system("rm -rf repo_tmp");
}

static void free_fdf_files(char *fdf_files[], int count) {
    for (int i = 0; i < count; ++i) {
        free(fdf_files[i]);
        fdf_files[i] = NULL;
    }
}

static int collect_fdf_files(char *fdf_files[], int max_files) {
    DIR *directory = opendir("repo_tmp");
    struct dirent *entry;
    int count = 0;

    if (!directory) {
        return 0;
    }

    while ((entry = readdir(directory)) != NULL && count < max_files) {
        size_t name_len = strlen(entry->d_name);
        if (name_len <= 4 || strcmp(entry->d_name + name_len - 4, ".fdf") != 0) {
            continue;
        }

        fdf_files[count] = malloc(MAX_PATH_LEN);
        if (!fdf_files[count]) {
            closedir(directory);
            return -1;
        }

        snprintf(fdf_files[count], MAX_PATH_LEN, "repo_tmp/%s", entry->d_name);
        count++;
    }

    closedir(directory);
    return count;
}

static int select_fdf_file(char *fdf_files[], int count) {
    int selected = 0;

    if (count == 1) {
        printf(DGREEN "[" GREEN "INFO" DGREEN "]" RESET " Found: %s\n", fdf_files[0]);
        return 0;
    }

    printf(DYELLOW "[" YELLOW "INFO" DYELLOW "]" RESET " Multiple .fdf files found:\n");
    for (int i = 0; i < count; ++i) {
        printf("         [%d] %s\n", i + 1, fdf_files[i]);
    }

    printf("         Select file [1-%d]: ", count);
    if (scanf("%d", &selected) != 1) {
        while (getchar() != '\n');
        return -1;
    }
    while (getchar() != '\n');

    if (selected < 1 || selected > count) {
        return -1;
    }

    return selected - 1;
}

int main(int argc, char *argv[]) {
    char *fdf_files[MAX_FDF_FILES] = {0};
    char repo[512] = {0};
    int force = 0;
    int show_diff = 0;
    int selected;
    int fdf_count;
    int parse_result;

    if (argc == 2 && strcmp(argv[1], "rollback") == 0) {
        return perform_rollback();
    }

    if (argc == 2 && strcmp(argv[1], "docs") == 0) {
        const char *home = getenv("HOME");
        if (!home || !*home) {
            fprintf(stderr, "Error: HOME environment variable not set\n");
            return 1;
        }
        char docs_path[MAX_PATH_LEN];
        snprintf(docs_path, sizeof(docs_path), "%s/.fdf_docs.txt", home);
        return view_docs(docs_path);
    }

    if (argc < 3) {
        print_usage();
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--repo") == 0 || strcmp(argv[i], "-r") == 0) && i+1 < argc) {
            strncpy(repo, argv[++i], sizeof(repo)-1);
        } else if (strcmp(argv[i], "--force-placement") == 0 || strcmp(argv[i], "-f") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "--show-diff") == 0 || strcmp(argv[i], "-d") == 0) {
            show_diff = 1;
        }
    }
    
    // Set global diff mode
    set_diff_enabled(show_diff);
    if (repo[0] == '\0') {
        print_usage();
        return 1;
    }

    if (rollback_session_start() != 0) {
        return 1;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git clone %s repo_tmp", repo);
    printf(DCYAN "[" CYAN "INFO" DCYAN "]" RESET " Cloning repository: %s\n\n", repo);
    if (system(cmd) != 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to clone repository\n");
        return 1;
    }

    fdf_count = collect_fdf_files(fdf_files, MAX_FDF_FILES);
    if (fdf_count < 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to allocate memory for .fdf list\n");
        cleanup_repo_tmp();
        return 1;
    }

    if (fdf_count == 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " No .fdf files found in repo_tmp\n");
        cleanup_repo_tmp();
        return 1;
    }

    selected = select_fdf_file(fdf_files, fdf_count);
    if (selected < 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Invalid selection.\n");
        free_fdf_files(fdf_files, fdf_count);
        cleanup_repo_tmp();
        return 1;
    }

    printf(DBLUE "[" BLUE "TASK" DBLUE "]" RESET " Processing dotfile...\n");
    parse_result = parse_dotfile(fdf_files[selected], force);

    free_fdf_files(fdf_files, fdf_count);
    cleanup_repo_tmp();

    if (parse_result != 0) {
        printf(DRED "[" RED "FAILED" DRED "]" RESET " Dotfile could not be fetched.\n");
        return 1;
    }

    printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Dotfiles fetched successfully.\n");
    return 0;
}
