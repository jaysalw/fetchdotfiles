#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void execute_command(const char *cmd) {
    printf("Running: %s\n", cmd);
    system(cmd);
}
