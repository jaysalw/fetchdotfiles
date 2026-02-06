#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

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

void print_usage() {
    printf("\n");
    printf(CYAN "FDF" RESET " - Fetch Dot Files\n");
    printf("-----------------------------------\n\n");
    printf("Usage:\n");
    printf("  fdf --repo <repo_url> [--force-placement]\n");
    printf("  fdf -r <repo_url> [-f]\n\n");
    printf("Options:\n");
    printf("  -r, --repo             Repository URL (git/GitHub)\n");
    printf("  -f, --force-placement  Overwrite existing files\n\n");
}


#include <dirent.h>

int main(int argc, char *argv[]) {
    char repo[512] = {0};
    int force = 0;
    if (argc < 3) {
        print_usage();
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--repo") == 0 || strcmp(argv[i], "-r") == 0) && i+1 < argc) {
            strncpy(repo, argv[++i], sizeof(repo)-1);
        } else if (strcmp(argv[i], "--force-placement") == 0 || strcmp(argv[i], "-f") == 0) {
            force = 1;
        }
    }
    if (repo[0] == '\0') {
        print_usage();
        return 1;
    }
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git clone %s repo_tmp", repo);
    printf(DCYAN "[" CYAN "INFO" DCYAN "]" RESET " Cloning repository: %s\n\n", repo);
    system(cmd);

    // Find all .fdf files in repo_tmp
    DIR *d;
    struct dirent *dir;
    char *fdf_files[32];
    int fdf_count = 0;
    d = opendir("repo_tmp");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            size_t len = strlen(dir->d_name);
            if (len > 4 && strcmp(dir->d_name + len - 4, ".fdf") == 0) {
                fdf_files[fdf_count] = malloc(512);
                snprintf(fdf_files[fdf_count], 512, "repo_tmp/%s", dir->d_name);
                fdf_count++;
            }
        }
        closedir(d);
    }

    if (fdf_count == 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " No .fdf files found in repo_tmp\n");
        system("rm -rf repo_tmp");
        return 1;
    }

    int selected = 0;
    if (fdf_count > 1) {
        printf(DYELLOW "[" YELLOW "INFO" DYELLOW "]" RESET " Multiple .fdf files found:\n");
        for (int i = 0; i < fdf_count; ++i) {
            printf("         [%d] %s\n", i+1, fdf_files[i]);
        }
        printf("         Select file [1-%d]: ", fdf_count);
        scanf("%d", &selected);
        if (selected < 1 || selected > fdf_count) {
            printf(DRED "[" RED "ERROR" DRED "]" RESET " Invalid selection.\n");
            system("rm -rf repo_tmp");
            return 1;
        }
        selected--;
    } else {
        printf(DGREEN "[" GREEN "INFO" DGREEN "]" RESET " Found: %s\n", fdf_files[0]);
    }

    printf(DBLUE "[" BLUE "TASK" DBLUE "]" RESET " Processing dotfile...\n\n");
    int result = parse_dotfile(fdf_files[selected], force);

    // Free allocated memory
    for (int i = 0; i < fdf_count; ++i) {
        free(fdf_files[i]);
    }

    // Clean up cloned repo
    system("rm -rf repo_tmp");

    if (result != 0) {
        printf("\n");
        printf(DRED "[" RED "FAILED" DRED "]" RESET " Dotfile processing failed!\n\n");
        return 1;
    }

    printf("\n");
    printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Dotfiles applied successfully!\n\n");
    return 0;
}
