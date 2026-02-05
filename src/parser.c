#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "file_ops.h"
#include "utils.h"

// Simple parser for demonstration
int parse_dotfile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open dotfile");
        return 1;
    }
    char line[1024];
    char src[512], dest[512], cmd[512];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "PUT %s IN %s", src, dest) == 2) {
            printf("Placing %s in %s\n", src, dest);
            place_dotfile(src, dest, 0);
        } else if (sscanf(line, "EXECUTE \"%[^"]\"", cmd) == 1) {
            printf("Executing: %s\n", cmd);
            execute_command(cmd);
        }
    }
    fclose(fp);
    return 0;
}
