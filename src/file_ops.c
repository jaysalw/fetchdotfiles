#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "file_ops.h"

// ANSI Color Codes
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define DRED     "\033[0;31m"
#define GREEN    "\033[1;32m"
#define DGREEN   "\033[0;32m"
#define YELLOW   "\033[1;33m"
#define DYELLOW  "\033[0;33m"

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void print_colored_warning(const char *msg) {
    printf(DYELLOW "[" YELLOW " WARNING " DYELLOW "]" RESET " %s\n", msg);
}

int place_dotfile(const char *src, const char *dest, int force) {
    // Check if source file exists first
    if (!file_exists(src)) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " Source file not found: %s\n", src);
        return 1;
    }
    
    if (file_exists(dest) && !force) {
        print_colored_warning("A dot file already exists:");
        printf("            DOTFILE - %s\n", dest);
        printf("            Overwrite? [Y/N]: ");
        char ans = getchar();
        while (getchar() != '\n');
        if (ans != 'Y' && ans != 'y') {
            printf(DYELLOW "[" YELLOW " INFO " DYELLOW "]" RESET " Skipping %s\n", dest);
            return 0;  // Not an error, user chose to skip
        }
    }
    FILE *f_src = fopen(src, "rb");
    FILE *f_dest = fopen(dest, "wb");
    if (!f_src) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " Cannot read source: %s\n", src);
        return 1;
    }
    if (!f_dest) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " Cannot write to destination: %s\n", dest);
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
    printf(DGREEN "[" GREEN " SUCCESS " DGREEN "]" RESET " Placed %s -> %s\n", src, dest);
    return 0;
}
