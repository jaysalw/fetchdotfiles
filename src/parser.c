#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "file_ops.h"
#include "utils.h"

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
#define MAGENTA  "\033[1;35m"
#define DMAGENTA "\033[0;35m"

static char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int parse_dotfile(const char *filename, int force) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " Failed to open dotfile: %s\n", filename);
        return 1;
    }
    
    char line[1024];
    char src[512], dest[512], cmd[512];
    int line_num = 0;
    int errors = 0;
    int end_fetch_found = 0;
    
    // Get the repo directory from filename (e.g., "repo_tmp/testing.fdf" -> "repo_tmp")
    char repo_dir[512];
    strncpy(repo_dir, filename, sizeof(repo_dir) - 1);
    char *last_slash = strrchr(repo_dir, '/');
    if (last_slash) *last_slash = '\0';
    else strcpy(repo_dir, ".");
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char *trimmed = trim(line);
        
        // Skip empty lines and comments
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        // Check for END FETCH
        if (strcmp(trimmed, "END FETCH") == 0) {
            end_fetch_found = 1;
            printf(DGREEN "[" GREEN " INFO " DGREEN "]" RESET " End of fetch instructions\n");
            break;
        }
        
        // Parse PUT command
        if (sscanf(trimmed, "PUT %s IN %s", src, dest) == 2) {
            // Build full source path: repo_dir/dotfiles/src
            char full_src[1024];
            snprintf(full_src, sizeof(full_src), "%s/dotfiles/%s", repo_dir, src);
            
            printf(DBLUE "[" BLUE " TASK " DBLUE "]" RESET " Placing %s -> %s\n", src, dest);
            place_dotfile(full_src, dest, force);
        }
        // Parse EXECUTE command
        else if (sscanf(trimmed, "EXECUTE \"%[^\"]\"", cmd) == 1) {
            printf(DCYAN "[" CYAN " EXEC " DCYAN "]" RESET " Running: %s\n", cmd);
            execute_command(cmd);
        }
        // Parse ECHO command
        else if (sscanf(trimmed, "ECHO \"%[^\"]\"", cmd) == 1) {
            printf(DMAGENTA "[" MAGENTA " SCRIPT RESPONSE " DMAGENTA "]" RESET " %s\n", cmd);
        }
        // Unknown/invalid syntax
        else {
            printf(DRED "[" RED " SYNTAX ERROR " DRED "]" RESET " Line %d: %s\n", line_num, trimmed);
            printf("         Expected: PUT <file> IN <path>\n");
            printf("                   EXECUTE \"<command>\"\n");
            printf("                   ECHO \"<message>\"\n");
            printf("                   END FETCH\n");
            errors++;
        }
    }
    
    fclose(fp);
    
    if (!end_fetch_found) {
        printf(DYELLOW "[" YELLOW " WARNING " DYELLOW "]" RESET " No 'END FETCH' found in dotfile\n");
    }
    
    if (errors > 0) {
        printf(DRED "[" RED " ERROR " DRED "]" RESET " %d syntax error(s) found\n", errors);
        return 1;
    }
    
    return 0;
}
