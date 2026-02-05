#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// ANSI Color Codes
#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

void print_usage() {
    printf(CYAN "╔══════════════════════════════════════════╗\n" RESET);
    printf(CYAN "║" YELLOW "       FDF - Fetch Dot Files              " CYAN "║\n" RESET);
    printf(CYAN "╚══════════════════════════════════════════╝\n" RESET);
    printf(WHITE "Usage:\n" RESET);
    printf("  " GREEN "fdf" RESET " --repo <repo_url> [--force-placement]\n");
    printf("  " GREEN "fdf" RESET " -r <repo_url> [-f]\n\n");
    printf(MAGENTA "Options:\n" RESET);
    printf("  " CYAN "-r, --repo" RESET "            Repository URL (git/GitHub)\n");
    printf("  " CYAN "-f, --force-placement" RESET "  Overwrite existing files\n");
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
    printf(CYAN "\n🔄 Cloning repository: " YELLOW "%s" RESET "\n\n", repo);
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
        printf(RED "\n❌ No .fdf files found in repo_tmp\n" RESET);
        system("rm -rf repo_tmp");
        return 1;
    }

    int selected = 0;
    if (fdf_count > 1) {
        printf(YELLOW "\n📂 Multiple .fdf files found:\n" RESET);
        for (int i = 0; i < fdf_count; ++i) {
            printf("  " CYAN "[%d]" RESET " %s\n", i+1, fdf_files[i]);
        }
        printf(WHITE "\nSelect which .fdf file to use [1-%d]: " RESET, fdf_count);
        scanf("%d", &selected);
        if (selected < 1 || selected > fdf_count) {
            printf(RED "❌ Invalid selection.\n" RESET);
            system("rm -rf repo_tmp");
            return 1;
        }
        selected--;
    } else {
        printf(GREEN "\n✔ Found: " RESET "%s\n", fdf_files[0]);
    }

    printf(CYAN "\n🚀 Processing dotfile...\n\n" RESET);
    parse_dotfile(fdf_files[selected], force);

    // Free allocated memory
    for (int i = 0; i < fdf_count; ++i) {
        free(fdf_files[i]);
    }

    // Clean up cloned repo
    system("rm -rf repo_tmp");

    printf(GREEN "\n╔══════════════════════════════════════════╗\n" RESET);
    printf(GREEN "║  ✅ SUCCESS! Dotfiles applied!           ║\n" RESET);
    printf(GREEN "╚══════════════════════════════════════════╝\n\n" RESET);
    return 0;
}
