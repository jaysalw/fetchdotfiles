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
    char src[256], dest[256], cmd[512];
    int line_num = 0;
    int errors = 0;
    int end_fetch_found = 0;
    
    // Config options
    char default_dir[256] = "dotfiles";  // Default subdirectory
    int select_from_root = 0;            // If true, ignore default_dir
    
    // Always use repo_tmp as base directory
    const char *repo_dir = "repo_tmp";
    
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
        
        // Parse CONFIG commands
        char config_key[64], config_val[256];
        if (sscanf(trimmed, "CONFIG %s = %s", config_key, config_val) == 2) {
            if (strcmp(config_key, "default_dir") == 0) {
                strncpy(default_dir, config_val, sizeof(default_dir) - 1);
                default_dir[sizeof(default_dir) - 1] = '\0';
                printf(DYELLOW "[" YELLOW " CONFIG " DYELLOW "]" RESET " default_dir = %s\n", default_dir);
            } else if (strcmp(config_key, "select_from_root") == 0) {
                if (strcmp(config_val, "True") == 0 || strcmp(config_val, "true") == 0 || strcmp(config_val, "1") == 0) {
                    select_from_root = 1;
                    printf(DYELLOW "[" YELLOW " CONFIG " DYELLOW "]" RESET " select_from_root = True\n");
                } else {
                    select_from_root = 0;
                    printf(DYELLOW "[" YELLOW " CONFIG " DYELLOW "]" RESET " select_from_root = False\n");
                }
            } else {
                printf(DYELLOW "[" YELLOW " WARNING " DYELLOW "]" RESET " Unknown config key: %s\n", config_key);
            }
            continue;
        }
        
        // Parse PUT command
        if (sscanf(trimmed, "PUT %s IN %s", src, dest) == 2) {
            // Build full source path based on config
            char full_src[600];
            if (select_from_root) {
                snprintf(full_src, sizeof(full_src), "%s/%s", repo_dir, src);
            } else {
                snprintf(full_src, sizeof(full_src), "%s/%s/%s", repo_dir, default_dir, src);
            }
            
            printf(DBLUE "[" BLUE " TASK " DBLUE "]" RESET " Placing %s -> %s\n", src, dest);
            if (place_dotfile(full_src, dest, force) != 0) {
                printf(DRED "[" RED " FATAL " DRED "]" RESET " Stopping due to error\n");
                fclose(fp);
                return 1;
            }
        }
        // Parse EXECUTE command
        else if (sscanf(trimmed, "EXECUTE \"%[^\"]\"", cmd) == 1) {
            printf(DCYAN "[" CYAN " EXEC " DCYAN "]" RESET " Running: %s\n", cmd);
            if (execute_command(cmd) != 0) {
                printf(DRED "[" RED " FATAL " DRED "]" RESET " Command failed, stopping\n");
                fclose(fp);
                return 1;
            }
        }
        // Parse ECHO command
        else if (sscanf(trimmed, "ECHO \"%[^\"]\"", cmd) == 1) {
            printf(DMAGENTA "[" MAGENTA " SCRIPT RESPONSE " DMAGENTA "]" RESET " %s\n", cmd);
        }
        // Unknown/invalid syntax
        else {
            printf(DRED "[" RED " SYNTAX ERROR " DRED "]" RESET " Line %d: %s\n", line_num, trimmed);
            printf("         Expected: CONFIG <key> = <value>\n");
            printf("                   PUT <file> IN <path>\n");
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
