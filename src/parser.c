#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "file_ops.h"
#include "utils.h"

// ANSI Color Codes
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define DRED     "\033[0;31m"
#define GREEN    "\033[1;32m"
#define DGREEN   "\033[0;32m"
#define BLUE     "\033[1;34m"
#define DBLUE    "\033[0;34m"
#define CYAN     "\033[1;36m"
#define DCYAN    "\033[0;36m"

int parse_dotfile(const char *filename, int force) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " Failed to open dotfile: %s\n", filename);
        return 1;
    }
    char line[1024];
    char src[512], dest[512], cmd[512];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "PUT %s IN %s", src, dest) == 2) {
            printf(DBLUE "[" BLUE " TASK " DBLUE "]" RESET " Placing %s -> %s\n", src, dest);
            place_dotfile(src, dest, force);
        } else if (sscanf(line, "EXECUTE \"%[^\"]\"", cmd) == 1) {
            printf(DCYAN "[" CYAN " EXEC " DCYAN "]" RESET " Running: %s\n", cmd);
            execute_command(cmd);
        }
    }
    fclose(fp);
    return 0;
}
