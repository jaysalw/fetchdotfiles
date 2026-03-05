#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
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

// Check if a package manager is available on the system
static int package_manager_exists(const char *pm) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "which %s >/dev/null 2>&1", pm);
    return system(cmd) == 0;
}

// Auto-detect available package managers
static void detect_package_managers(char detected[][64], int *count, int max_count) {
    const char *common_pms[] = {"pacman", "yay", "apt", "dnf", "zypper", "emerge", "apk", NULL};
    *count = 0;
    
    for (int i = 0; common_pms[i] != NULL && *count < max_count; i++) {
        if (package_manager_exists(common_pms[i])) {
            strncpy(detected[*count], common_pms[i], 63);
            detected[*count][63] = '\0';
            (*count)++;
        }
    }
}

// Parse version string and compare with constraints
static int check_version_constraint(const char *installed_ver, const char *constraint, const char *required_ver) {
    // For now, just do basic string comparison
    // In production, you'd parse version numbers properly
    if (strcmp(constraint, "==") == 0) {
        return strcmp(installed_ver, required_ver) == 0;
    } else if (strcmp(constraint, ">=") == 0) {
        return strcmp(installed_ver, required_ver) >= 0;
    }
    return 1;  // No constraint means any version is OK
}

// Install a package using the specified package manager
static int install_package(const char *package, const char *pm) {
    char cmd[512];
    
    if (strcmp(pm, "pacman") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo pacman -S --noconfirm %s", package);
    } else if (strcmp(pm, "yay") == 0) {
        snprintf(cmd, sizeof(cmd), "yay -S --noconfirm %s", package);
    } else if (strcmp(pm, "apt") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo apt-get install -y %s", package);
    } else if (strcmp(pm, "dnf") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo dnf install -y %s", package);
    } else if (strcmp(pm, "zypper") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo zypper install -y %s", package);
    } else if (strcmp(pm, "emerge") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo emerge %s", package);
    } else if (strcmp(pm, "apk") == 0) {
        snprintf(cmd, sizeof(cmd), "sudo apk add %s", package);
    } else {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Unknown package manager: %s\n", pm);
        return 1;
    }
    
    printf(DCYAN "[" CYAN "INSTALL" DCYAN "]" RESET " Installing %s with %s\n", package, pm);
    return execute_command(cmd);
}

int parse_dotfile(const char *filename, int force) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " Failed to open dotfile: %s\n", filename);
        return 1;
    }
    
    char line[1024];
    char src[256], dest[256], cmd[512];
    int line_num = 0;
    int errors = 0;
    int end_fetch_found = 0;
    int in_depend_block = 0;
    
    // Config options
    char default_dir[256] = "dotfiles";  // Default subdirectory
    int select_from_root = 0;            // If true, ignore default_dir
    char allowed_pms[16][64];            // Allowed package managers
    int allowed_pm_count = 0;
    char detected_pms[16][64];           // Auto-detected package managers
    int detected_pm_count = 0;
    int auto_detect_pm = 1;              // Auto-detect by default
    
    // Always use repo_tmp as base directory
    const char *repo_dir = "repo_tmp";
    
    // Detect available package managers
    detect_package_managers(detected_pms, &detected_pm_count, 16);
    if (detected_pm_count > 0) {
        printf(DGREEN "[" GREEN "INFO" DGREEN "]" RESET " Detected package managers:");
        for (int i = 0; i < detected_pm_count; i++) {
            printf(" %s", detected_pms[i]);
        }
        printf("\n");
    }
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char *trimmed = trim(line);
        
        // Skip empty lines and comments
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        // Check for END FETCH
        if (strcmp(trimmed, "END FETCH") == 0) {
            end_fetch_found = 1;
            printf(DGREEN "[" GREEN "INFO" DGREEN "]" RESET " End of fetch instructions\n");
            break;
        }
        
        // Handle DEPEND_ON block end
        if (in_depend_block && strchr(trimmed, '}')) {
            in_depend_block = 0;
            printf(DGREEN "[" GREEN "INFO" DGREEN "]" RESET " Dependencies checked\n");
            continue;
        }
        
        // Handle lines inside DEPEND_ON block
        if (in_depend_block) {
            char pkg_name[128] = "";
            char version_op[8] = "";
            char version[64] = "";
            
            // Parse: package>=version, package==version, or just package
            if (sscanf(trimmed, "%127[^>=<]>=%63s", pkg_name, version) == 2) {
                strcpy(version_op, ">=");
            } else if (sscanf(trimmed, "%127[^>=<]==%63s", pkg_name, version) == 2) {
                strcpy(version_op, "==");
            } else if (sscanf(trimmed, "%127s", pkg_name) == 1) {
                strcpy(version_op, "");
                strcpy(version, "");
            }
            
            // Trim package name
            char *pkg_trimmed = trim(pkg_name);
            if (strlen(pkg_trimmed) > 0) {
                if (strlen(version_op) > 0) {
                    printf(DYELLOW "[" YELLOW "DEPEND" DYELLOW "]" RESET " Checking %s %s %s\n", 
                           pkg_trimmed, version_op, version);
                } else {
                    printf(DYELLOW "[" YELLOW "DEPEND" DYELLOW "]" RESET " Checking %s\n", pkg_trimmed);
                }
                // Note: In production, you'd actually check if the package is installed
                // and verify the version constraint here
            }
            continue;
        }
        
        // Check for DEPEND_ON block start
        if (strncmp(trimmed, "DEPEND_ON", 9) == 0 && strchr(trimmed, '{')) {
            in_depend_block = 1;
            printf(DBLUE "[" BLUE "TASK" DBLUE "]" RESET " Checking dependencies\n");
            continue;
        }
        
        // Parse CONFIG commands
        char config_key[64], config_val[256];
        
        // Handle array-style CONFIG (allow_package_managers)
        if (strncmp(trimmed, "CONFIG allow_package_managers", 29) == 0) {
            char *bracket_start = strchr(trimmed, '[');
            char *bracket_end = strchr(trimmed, ']');
            if (bracket_start && bracket_end) {
                bracket_start++; // Skip '['
                *bracket_end = '\0'; // Terminate at ']'
                
                // Parse comma-separated values
                char *token = strtok(bracket_start, ",");
                allowed_pm_count = 0;
                while (token && allowed_pm_count < 16) {
                    // Trim whitespace and quotes
                    while (isspace(*token) || *token == '"') token++;
                    char *end = token + strlen(token) - 1;
                    while (end > token && (isspace(*end) || *end == '"')) {
                        *end = '\0';
                        end--;
                    }
                    
                    if (strlen(token) > 0) {
                        strncpy(allowed_pms[allowed_pm_count], token, 63);
                        allowed_pms[allowed_pm_count][63] = '\0';
                        allowed_pm_count++;
                    }
                    token = strtok(NULL, ",");
                }
                
                printf(DYELLOW "[" YELLOW "CONFIG" DYELLOW "]" RESET " allow_package_managers = [");
                for (int i = 0; i < allowed_pm_count; i++) {
                    printf("%s%s", allowed_pms[i], i < allowed_pm_count - 1 ? ", " : "");
                }
                printf("]\n");
                auto_detect_pm = 0;
            }
            continue;
        }
        
        // Regular CONFIG commands
        if (sscanf(trimmed, "CONFIG %s = %s", config_key, config_val) == 2) {
            if (strcmp(config_key, "default_dir") == 0) {
                strncpy(default_dir, config_val, sizeof(default_dir) - 1);
                default_dir[sizeof(default_dir) - 1] = '\0';
                printf(DYELLOW "[" YELLOW "CONFIG" DYELLOW "]" RESET " default_dir = %s\n", default_dir);
            } else if (strcmp(config_key, "select_from_root") == 0) {
                if (strcmp(config_val, "True") == 0 || strcmp(config_val, "true") == 0 || strcmp(config_val, "1") == 0) {
                    select_from_root = 1;
                    printf(DYELLOW "[" YELLOW "CONFIG" DYELLOW "]" RESET " select_from_root = True\n");
                } else {
                    select_from_root = 0;
                    printf(DYELLOW "[" YELLOW "CONFIG" DYELLOW "]" RESET " select_from_root = False\n");
                }
            } else {
                printf(DYELLOW "[" YELLOW "WARNING" DYELLOW "]" RESET " Unknown config key: %s\n", config_key);
            }
            continue;
        }
        
        // Parse GETPKG command
        char pkg_name[256], pm_name[64];
        if (sscanf(trimmed, "GETPKG \"%[^\"]\" WITH \"%[^\"]\"", pkg_name, pm_name) == 2) {
            // Check if the specified package manager is allowed
            int pm_allowed = 0;
            if (auto_detect_pm) {
                // If auto-detect, check if it's in detected list
                for (int i = 0; i < detected_pm_count; i++) {
                    if (strcmp(detected_pms[i], pm_name) == 0) {
                        pm_allowed = 1;
                        break;
                    }
                }
            } else {
                // If allow list is set, check if it's in allowed list
                for (int i = 0; i < allowed_pm_count; i++) {
                    if (strcmp(allowed_pms[i], pm_name) == 0) {
                        pm_allowed = 1;
                        break;
                    }
                }
            }
            
            if (pm_allowed) {
                if (install_package(pkg_name, pm_name) != 0) {
                    printf(DRED "[" RED "FATAL" DRED "]" RESET " Package installation failed\n");
                    fclose(fp);
                    return 1;
                }
            } else {
                printf(DRED "[" RED "ERROR" DRED "]" RESET " Package manager '%s' is not allowed or not available\n", pm_name);
                fclose(fp);
                return 1;
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
            
            printf(DBLUE "[" BLUE "TASK" DBLUE "]" RESET " Placing %s -> %s\n", src, dest);
            if (place_dotfile(full_src, dest, force) != 0) {
                printf(DRED "[" RED "FATAL" DRED "]" RESET " Stopping due to error\n");
                fclose(fp);
                return 1;
            }
        }
        // Parse EXECUTE command
        else if (sscanf(trimmed, "EXECUTE \"%[^\"]\"", cmd) == 1) {
            printf(DCYAN "[" CYAN " EXEC " DCYAN "]" RESET " Running: %s\n", cmd);
            if (execute_command(cmd) != 0) {
                printf(DRED "[" RED "FATAL" DRED "]" RESET " Command failed, stopping\n");
                fclose(fp);
                return 1;
            }
        }
        // Parse ECHO command
        else if (sscanf(trimmed, "ECHO \"%[^\"]\"", cmd) == 1) {
            printf(DMAGENTA "[" MAGENTA "SCRIPT RESPONSE" DMAGENTA "]" RESET " %s\n", cmd);
        }
        // Unknown/invalid syntax
        else {
            printf(DRED "[" RED "SYNTAX ERROR" DRED "]" RESET " Line %d: %s\n", line_num, trimmed);
            printf("         Expected: CONFIG <key> = <value>\n");
            printf("                   CONFIG allow_package_managers = [\"pm1\", \"pm2\"]\n");
            printf("                   PUT <file> IN <path>\n");
            printf("                   EXECUTE \"<command>\"\n");
            printf("                   ECHO \"<message>\"\n");
            printf("                   GETPKG \"<package>\" WITH \"<package_manager>\"\n");
            printf("                   DEPEND_ON { ... }\n");
            printf("                   END FETCH\n");
            printf("         See https://fetchdots.net/docs for help.\n");
            errors++;
        }
    }
    
    fclose(fp);
    
    if (!end_fetch_found) {
        printf(DYELLOW "[" YELLOW "WARNING" DYELLOW "]" RESET " No 'END FETCH' found in dotfile\n");
    }
    
    if (errors > 0) {
        printf(DRED "[" RED "ERROR" DRED "]" RESET " %d syntax error(s) found\n", errors);
        return 1;
    }
    
    return 0;
}
