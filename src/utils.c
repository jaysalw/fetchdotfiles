#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

// ANSI Color Codes
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define DRED     "\033[0;31m"
#define GREEN    "\033[1;32m"
#define DGREEN   "\033[0;32m"

int execute_command(const char *cmd) {
    int result = system(cmd);
    if (result == 0) {
        printf(DGREEN "[" GREEN "SUCCESS" DGREEN "]" RESET " Command completed\n");
        return 0;
    } else {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Command failed (exit: %d)\n", result);
        return 1;
    }
}
