#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "file_ops.h"

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void print_colored_warning(const char *msg) {
    printf("\033[1;33m[ WARNING ] %s\033[0m\n", msg);
}

int place_dotfile(const char *src, const char *dest, int force) {
    if (file_exists(dest) && !force) {
        print_colored_warning("A dot file already exists:");
        printf("DOTFILE - %s\n", dest);
        printf("Would you like to overwrite the file? [Y/N] ");
        char ans = getchar();
        if (ans != 'Y' && ans != 'y') {
            printf("Skipping %s\n", dest);
            return 1;
        }
    }
    FILE *f_src = fopen(src, "rb");
    FILE *f_dest = fopen(dest, "wb");
    if (!f_src || !f_dest) {
        perror("File open error");
        if (f_src) fclose(f_src);
        if (f_dest) fclose(f_dest);
        return 1;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f_src)) > 0) {
        fwrite(buf, 1, n, f_dest);
    }
    fclose(f_src);
    fclose(f_dest);
    printf("Placed %s -> %s\n", src, dest);
    return 0;
}
