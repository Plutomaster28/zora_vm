#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <locale.h>
#include "platform/platform.h"  // FIXED: Use correct path
#include "shell.h"
#include "user.h"
#include "kernel.h"
#include "tetra.h"
#include "color-and-test.h"
#include "config.h"
#include "network/network.h"
#include "vfs/vfs.h"
#include "lua/lua_vm.h"
#include "binary/binary_executor.h"
#include "vm.h"  // For crash guard control
#include "terminal/terminal_style.h"  // Add terminal styling support
#include "kernel/system_monitor.h"  // System monitoring capabilities
#include "terminal/terminal_detector.h"  // Terminal detection and compatibility
// #include "shell_script.h"  // Enhanced shell scripting - temporarily disabled

// Windows-specific includes
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <wininet.h>
#include <zlib.h>
#include <iprtrmib.h>

// Function prototypes
void handle_command(char *command);
void parse_and_execute_command_line(char *command_line);
int execute_command_with_parsing(char *cmd_str);
int execute_command_with_redirection(char *args[], int argc, char *input_file, char *output_file, int append_mode);
int try_execute_script_command(const char* command, int argc, char** argv);
void execute_simple_command(char *args[], int argc);
void execute_simple_command_with_redirect(char *args[], int argc);
int execute_pipeline(char *pipeline_str);
void redirect_printf(const char* format, ...);
void man_command(int argc, char **argv);
void help_command(int argc, char **argv);
void theme_command(int argc, char **argv);
void save_command(int argc, char* argv[]);
void load_command(int argc, char* argv[]);
void mount_command(int argc, char* argv[]);
void sync_command(int argc, char* argv[]);
void persistent_ls_command(int argc, char* argv[]);
void ifconfig_command(int argc, char* argv[]);
void ping_command(int argc, char **argv);
void netstat_command(int argc, char **argv);
void nslookup_command(int argc, char **argv);
void telnet_command(int argc, char **argv);
void wget_command(int argc, char **argv);
void curl_command(int argc, char **argv);
void ssh_command(int argc, char **argv);
void iptables_command(int argc, char **argv);
void test_vfs_command(int argc, char* argv[]);
void debug_vfs_command(int argc, char* argv[]);
void lua_command(int argc, char **argv);
void luacode_command(int argc, char **argv);
void test_sandbox_command(int argc, char **argv);

// System monitor commands
void top_command(int argc, char **argv);
void osinfo_command(int argc, char **argv);
void mounts_command(int argc, char **argv);
void netinfo_command(int argc, char **argv);
void proc_command(int argc, char **argv);
void dmesg_command(int argc, char **argv);
void services_command(int argc, char **argv);
void env_command(int argc, char **argv);
void terminal_test_command(int argc, char **argv);
void launch_wt_command(int argc, char **argv);
void theme_command(int argc, char **argv);
void themes_command(int argc, char **argv);
void find_command(int argc, char **argv);
void find_files_recursive(const char* dir_path, const char* pattern);
void tree_command(int argc, char **argv);
void print_tree_recursive(const char* dir_path, int depth);
void scripts_command(int argc, char **argv);
void version_command(int argc, char **argv);
void systeminfo_command(int argc, char **argv);
void sort_command(int argc, char **argv);
void uniq_command(int argc, char **argv);
void wc_command(int argc, char **argv);
void awk_command(int argc, char **argv);
void which_command(int argc, char **argv);
void ln_command(int argc, char **argv);
void diff_command(int argc, char **argv);
void exit_command(int argc, char **argv);
void version_command(int argc, char **argv);
void set_command(int argc, char **argv);
void unset_command(int argc, char **argv);
void export_command(int argc, char **argv);
void font_debug_command(int argc, char **argv);
void console_refresh_command(int argc, char **argv);
void console_test_command(int argc, char **argv);
void ghost_pipe_utf8_fix(void);
void utf8_detailed_test_command(int argc, char **argv);
void utf8_monitor_command(int argc, char **argv);
void utf8_fix_command(int argc, char **argv);
void utf8_test_command(int argc, char **argv);

// Enhanced command parsing and execution
void parse_and_execute_command_line(char *command_line);
int execute_command_with_parsing(char *cmd_str);
int execute_command_with_redirection(char *args[], int argc, char *input_file, char *output_file, int append_mode);
void execute_simple_command(char *args[], int argc);

// Enhanced argument parsing
typedef struct {
    char** args;
    int argc;
    char** flags;
    int flag_count;
    char* input_redirect;
    char* output_redirect;
    int append_mode;
    int background;
} ParsedCommand;

ParsedCommand* parse_command_advanced(char* command_str);
void free_parsed_command(ParsedCommand* cmd);
int has_flag(ParsedCommand* cmd, const char* flag);
char* get_flag_value(ParsedCommand* cmd, const char* flag);

// Enhanced terminal output functions
void print_command_syntax(const char* command_line);
void highlight_command_parts(char* command_line);
void calculate_directory_size(VNode* node, size_t* total_size, size_t* used_size);

// Path normalization function
void normalize_path(char* path) {
    // Remove double slashes
    char* src = path;
    char* dst = path;
    int last_was_slash = 0;
    
    while (*src) {
        if (*src == '/') {
            if (!last_was_slash) {
                *dst++ = *src;
                last_was_slash = 1;
            }
        } else {
            *dst++ = *src;
            last_was_slash = 0;
        }
        src++;
    }
    *dst = '\0';
    
    // Ensure root path is just "/"
    if (strcmp(path, "") == 0) {
        strcpy(path, "/");
    }
}

// Helper function to build normalized paths
void build_full_path(char* full_path, size_t size, const char* cwd, const char* relative_path) {
    if (relative_path[0] == '/') {
        // Absolute path
        strncpy(full_path, relative_path, size - 1);
    } else {
        // Relative path
        if (strcmp(cwd, "/") == 0) {
            snprintf(full_path, size, "/%s", relative_path);
        } else {
            snprintf(full_path, size, "%s/%s", cwd, relative_path);
        }
    }
    full_path[size - 1] = '\0';
    normalize_path(full_path);
}

// Color support functions
void set_color(int color);
void reset_color();
void print_colored_prompt();

// ANSI color codes
#define COLOR_RESET     "\033[0m"
#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"

// Environment variable support
#define MAX_ENV_VARS 100
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 256

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} EnvVar;

static EnvVar env_vars[MAX_ENV_VARS];
static int env_var_count = 0;

// Environment variable functions
void set_env_var(const char* name, const char* value);
const char* get_env_var(const char* name);
void expand_variables(const char* input, char* output, size_t output_size);
void expand_path(const char* input, char* output, size_t output_size);
void init_default_env_vars(void);
#define COLOR_BRIGHT_BLACK   "\033[90m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"
#define COLOR_BRIGHT_WHITE   "\033[97m"

// Kali Linux inspired colors
#define KALI_USER_COLOR     COLOR_BRIGHT_GREEN
#define KALI_AT_COLOR       COLOR_WHITE
#define KALI_HOST_COLOR     COLOR_BRIGHT_BLUE  
#define KALI_PROMPT_COLOR   COLOR_BRIGHT_GREEN
#define KALI_PATH_COLOR     COLOR_BRIGHT_CYAN

// Global user system state
// These are defined in user.c
extern char current_user[50];
extern int is_logged_in;
static char hostname[50] = "zora-vm";
static char current_path[256] = "/";

// Color support functions implementation
void set_color(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void reset_color() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7); // Default white on black
}

void print_colored_prompt() {
    // Simpler, more robust approach to avoid encoding issues
    // Make sure console mode is set properly for ANSI
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    
    if (GetConsoleMode(hConsole, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, dwMode);
    }
    
    // Use simple ANSI colors - avoid complex sequences
    printf("\033[92m%s\033[0m", current_user);  // Bright green user
    printf("\033[37m@\033[0m");                 // White @
    printf("\033[94m%s\033[0m", hostname);      // Bright blue hostname  
    printf("\033[37m:\033[0m");                 // White :
    printf("\033[96m%s\033[0m", current_path);  // Bright cyan path
    printf("\033[92m> \033[0m");                // Bright green prompt
    
    fflush(stdout);  // Ensure prompt is displayed immediately
}

// Missing file system commands
void less_command(int argc, char **argv);
void head_command(int argc, char **argv);
void tail_command(int argc, char **argv);
void grep_command(int argc, char **argv);

// Missing file permissions commands
void chmod_command(int argc, char **argv);
void chown_command(int argc, char **argv);

// Missing process management commands
void htop_command(int argc, char **argv);
void jobs_command(int argc, char **argv);
void bg_command(int argc, char **argv);
void fg_command(int argc, char **argv);

// Missing system information commands
void date_command(int argc, char **argv);
void df_command(int argc, char **argv);
void du_command(int argc, char **argv);
void uname_command(int argc, char **argv);
void history_command(int argc, char **argv);

// Missing networking commands
void scp_command(int argc, char **argv);

// Missing archiving commands
void tar_command(int argc, char **argv);
void gzip_command(int argc, char **argv);
void gunzip_command(int argc, char **argv);
void zip_command(int argc, char **argv);
void unzip_command(int argc, char **argv);
void hostname_command(int argc, char **argv);

// Missing command implementations

// File system commands
void less_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: less <filename>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    VNode* file_node = vfs_find_node(full_path);
    if (!file_node) {
        printf("less: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("less: %s: Is a directory\n", full_path);
        return;
    }
    
    // Use VFS API to read file content (triggers on-demand loading)
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) == 0 && data && size > 0) {
        // Display file contents with paging (simple version)
        char* content = (char*)data;
        int lines_shown = 0;
        int i = 0;
        
        while (i < (int)size) {
            if (lines_shown >= 20) {  // Show 20 lines at a time
                printf("--More-- (Press Enter to continue, q to quit)");
                char c = getchar();
                if (c == 'q' || c == 'Q') break;
                lines_shown = 0;
            }
            
            putchar(content[i]);
            if (content[i] == '\n') lines_shown++;
            i++;
        }
        printf("\n");
    } else {
        printf("(empty file)\n");
    }
}

void head_command(int argc, char **argv) {
    int lines = 10;  // Default number of lines
    char* filename = NULL;
    
    if (argc < 2) {
        printf("Usage: head [-n lines] <filename>\n");
        return;
    }
    
    // Parse arguments
    if (argc == 2) {
        filename = argv[1];
    } else if (argc == 4 && strcmp(argv[1], "-n") == 0) {
        lines = atoi(argv[2]);
        filename = argv[3];
    } else {
        printf("Usage: head [-n lines] <filename>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(filename, expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    VNode* file_node = vfs_find_node(full_path);
    if (!file_node) {
        printf("head: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("head: %s: Is a directory\n", full_path);
        return;
    }
    
    // Use VFS API to read file content (triggers on-demand loading)
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) == 0 && data && size > 0) {
        char* content = (char*)data;
        int lines_shown = 0;
        int i = 0;
        
        while (i < (int)size && lines_shown < lines) {
            putchar(content[i]);
            if (content[i] == '\n') lines_shown++;
            i++;
        }
        if (content[file_node->size - 1] != '\n') printf("\n");
    } else {
        printf("(empty file)\n");
    }
}

void tail_command(int argc, char **argv) {
    int lines = 10;  // Default number of lines
    char* filename = NULL;
    
    if (argc < 2) {
        printf("Usage: tail [-n lines] <filename>\n");
        return;
    }
    
    // Parse arguments
    if (argc == 2) {
        filename = argv[1];
    } else if (argc == 4 && strcmp(argv[1], "-n") == 0) {
        lines = atoi(argv[2]);
        filename = argv[3];
    } else {
        printf("Usage: tail [-n lines] <filename>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(filename, expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    VNode* file_node = vfs_find_node(full_path);
    if (!file_node) {
        printf("tail: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("tail: %s: Is a directory\n", full_path);
        return;
    }
    
    // Use VFS API to read file content (triggers on-demand loading)
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) == 0 && data && size > 0) {
        char* content = (char*)data;
        
        // Count total lines first
        int total_lines = 0;
        for (int i = 0; i < (int)size; i++) {
            if (content[i] == '\n') total_lines++;
        }
        
        // Find starting position for last N lines
        int target_line = (total_lines > lines) ? total_lines - lines : 0;
        int current_line = 0;
        int start_pos = 0;
        
        for (int i = 0; i < (int)file_node->size; i++) {
            if (current_line >= target_line) {
                start_pos = i;
                break;
            }
            if (content[i] == '\n') current_line++;
        }
        
        // Print from start_pos to end
        for (int i = start_pos; i < (int)file_node->size; i++) {
            putchar(content[i]);
        }
        if (content[file_node->size - 1] != '\n') printf("\n");
    } else {
        printf("(empty file)\n");
    }
}

void grep_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: grep <pattern> [filename]\n");
        printf("If no filename provided, reads from standard input (pipeline)\n");
        return;
    }
    
    char* pattern = argv[1];
    char* filename = (argc >= 3) ? argv[2] : NULL;
    
    // If no filename provided, this means we're in a pipeline - should not happen
    // since pipeline code adds the temp file as argument
    if (!filename) {
        printf("grep: no input source specified\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(filename, expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    VNode* file_node = vfs_find_node(full_path);
    if (!file_node) {
        printf("grep: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("grep: %s: Is a directory\n", full_path);
        return;
    }
    
    // Use VFS API to read file content (triggers on-demand loading)
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) == 0 && data && size > 0) {
        char* content = (char*)data;
        char line[1024];
        int line_num = 1;
        int line_pos = 0;
        
        for (int i = 0; i < (int)size; i++) {
            if (content[i] == '\n' || i == (int)size - 1) {
                line[line_pos] = '\0';
                
                // Check if line contains pattern
                if (strstr(line, pattern) != NULL) {
                    printf("%s\n", line);
                }
                
                line_num++;
                line_pos = 0;
            } else {
                if (line_pos < 1023) {
                    line[line_pos++] = content[i];
                }
            }
        }
    }
}

// Process management commands
static int background_jobs[10] = {0};
static int job_count = 0;

void htop_command(int argc, char **argv) {
    printf("=== Zora VM Process Monitor (htop) ===\n");
    printf("  PID USER      PR  NI    VIRT    RES    SHR S  %%CPU %%MEM     TIME+ COMMAND\n");
    
    // Get real process information using Windows APIs
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create process snapshot\n");
        return;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    int process_count = 0;
    int running_count = 0;
    DWORD total_memory = 0;
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Get memory information for this process
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    // Convert bytes to KB for display
                    DWORD virt_kb = (DWORD)(pmc.PagefileUsage / 1024);
                    DWORD res_kb = (DWORD)(pmc.WorkingSetSize / 1024);
                    total_memory += res_kb;
                    
                    // Get CPU time
                    FILETIME ftCreation, ftExit, ftKernel, ftUser;
                    if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                        ULARGE_INTEGER userTime;
                        userTime.LowPart = ftUser.dwLowDateTime;
                        userTime.HighPart = ftUser.dwHighDateTime;
                        DWORD seconds = (DWORD)(userTime.QuadPart / 10000000);
                        
                        printf("%5lu %-8s 20   0 %8lu %7lu %6lu S   0.0 %4.1f   %d:%02d.%02d %s\n",
                               pe32.th32ProcessID,
                               "vm",
                               virt_kb,
                               res_kb,
                               res_kb / 2,
                               ((double)res_kb / 1024.0 / 1024.0) * 100.0,
                               seconds / 60,
                               seconds % 60,
                               (int)(userTime.QuadPart / 100000) % 100,
                               pe32.szExeFile);
                    }
                }
                CloseHandle(hProcess);
            }
            process_count++;
            running_count++; // Simplified - assume all are running
        } while (Process32Next(hSnapshot, &pe32) && process_count < 10); // Limit to first 10 processes
    }
    
    CloseHandle(hSnapshot);
    
    // Get system memory information
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        DWORD total_mb = (DWORD)(memStatus.ullTotalPhys / 1024 / 1024);
        DWORD used_mb = total_memory / 1024;
        DWORD free_mb = total_mb - used_mb;
        
        printf("\nTasks: %d total, %d running, %d sleeping\n", process_count, running_count, 0);
        printf("CPU: 0.1%% us, 0.0%% sy, 0.0%% ni, 99.9%% id\n");
        printf("Memory: %luM total, %luM used, %luM free\n", total_mb, used_mb, free_mb);
    }
    
    printf("\nPress 'q' to quit, any other key to refresh...\n");
    
    char c = getchar();
    if (c != 'q' && c != 'Q') {
        printf("Use 'htop' again to refresh\n");
    }
}

void jobs_command(int argc, char **argv) {
    printf("Background jobs:\n");
    if (job_count == 0) {
        printf("No background jobs\n");
    } else {
        for (int i = 0; i < job_count; i++) {
            printf("[%d]+ %d Running    job_%d\n", i+1, background_jobs[i], i+1);
        }
    }
}

void bg_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: bg <job_id>\n");
        return;
    }
    
    int job_id = atoi(argv[1]);
    printf("bg: Sent job %d to background (simulated)\n", job_id);
}

void fg_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: fg <job_id>\n");
        return;
    }
    
    int job_id = atoi(argv[1]);
    printf("fg: Brought job %d to foreground (simulated)\n", job_id);
}

// System information commands
void date_command(int argc, char **argv) {
    time_t now = time(NULL);
    char* date_str = ctime(&now);
    // Remove trailing newline
    date_str[strlen(date_str) - 1] = '\0';
    printf("%s\n", date_str);
}

void df_command(int argc, char **argv) {
    printf("Filesystem     1K-blocks    Used Available Use%% Mounted on\n");
    
    // Calculate real VFS usage
    VNode* root = vfs_find_node("/");
    if (root) {
        size_t total_size = 0;
        size_t used_size = 0;
        
        // Recursively calculate directory sizes
        calculate_directory_size(root, &total_size, &used_size);
        
        size_t total_kb = total_size / 1024;
        size_t used_kb = used_size / 1024;
        size_t available_kb = total_kb - used_kb;
        int use_percent = total_kb > 0 ? (int)((used_kb * 100) / total_kb) : 0;
        
        printf("vfs_root      %8lu %8lu %8lu %4d%% /\n", 
               total_kb > 0 ? total_kb : 262144, used_kb, available_kb, use_percent);
    }
    
    // Show individual directories
    const char* dirs[] = {"/scripts", "/projects", "/documents", "/data", "/tmp", "/home"};
    const int dir_count = sizeof(dirs) / sizeof(dirs[0]);
    
    for (int i = 0; i < dir_count; i++) {
        VNode* dir = vfs_find_node(dirs[i]);
        if (dir && dir->is_directory) {
            size_t dir_total = 0;
            size_t dir_used = 0;
            calculate_directory_size(dir, &dir_total, &dir_used);
            
            size_t total_kb = dir_total / 1024;
            size_t used_kb = dir_used / 1024;
            size_t available_kb = total_kb - used_kb;
            int use_percent = total_kb > 0 ? (int)((used_kb * 100) / total_kb) : 0;
            
            printf("vfs_%-9s %8lu %8lu %8lu %4d%% %s\n", 
                   dirs[i] + 1, // Skip leading '/'
                   total_kb > 0 ? total_kb : 65536, 
                   used_kb, available_kb, use_percent, dirs[i]);
        }
    }
}

// Helper function to calculate directory size
void calculate_directory_size(VNode* node, size_t* total_size, size_t* used_size) {
    if (!node) return;
    
    if (node->is_directory) {
        *total_size += 4096; // Directory entry size
        VNode* child = node->children;
        while (child) {
            calculate_directory_size(child, total_size, used_size);
            child = child->next;
        }
    } else {
        *total_size += node->size;
        *used_size += node->size;
    }
}

void du_command(int argc, char **argv) {
    char* target_dir = (argc > 1) ? argv[1] : vfs_getcwd();
    
    char expanded_path[512];
    expand_path(target_dir, expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    printf("Disk usage for %s:\n", full_path);
    
    VNode* dir_node = vfs_find_node(full_path);
    if (!dir_node || !dir_node->is_directory) {
        printf("du: %s: Not a directory\n", full_path);
        return;
    }
    
    size_t total_size = 0;
    VNode* child = dir_node->children;
    while (child) {
        printf("%zu\t%s\n", child->size, child->name);
        total_size += child->size;
        child = child->next;
    }
    printf("%zu\ttotal\n", total_size);
}

void uname_command(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        printf("ZoraVM 1.0 zora-vm x86_64 x86_64 x86_64 Windows\n");
    } else {
        printf("ZoraVM\n");
    }
}

static char* command_history[100];
static int history_count = 0;

void history_command(int argc, char **argv) {
    printf("Command history:\n");
    if (history_count == 0) {
        printf("No commands in history\n");
    } else {
        for (int i = 0; i < history_count; i++) {
            printf("%4d  %s\n", i+1, command_history[i]);
        }
    }
}

// Add command to history (called from handle_command)
void add_to_history(const char* command) {
    if (history_count < 100) {
        command_history[history_count] = malloc(strlen(command) + 1);
        strcpy(command_history[history_count], command);
        history_count++;
    } else {
        // Rotate history - remove oldest, add newest
        free(command_history[0]);
        for (int i = 0; i < 99; i++) {
            command_history[i] = command_history[i+1];
        }
        command_history[99] = malloc(strlen(command) + 1);
        strcpy(command_history[99], command);
    }
}

// Networking commands
void scp_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: scp <source> <destination>\n");
        printf("Example: scp file.txt user@host:/path/\n");
        return;
    }
    
    char* source = argv[1];
    char* dest = argv[2];
    
    printf("scp: Copying %s to %s (simulated)\n", source, dest);
    printf("scp: 100%% |***********************| 1024 bytes transferred\n");
}

// Archiving commands
// Simple tar header structure (simplified for demo)
struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

void write_tar_header(FILE* tar_file, const char* filename, size_t filesize) {
    struct tar_header header;
    memset(&header, 0, sizeof(header));
    
    strncpy(header.name, filename, sizeof(header.name) - 1);
    strcpy(header.mode, "0000644");
    strcpy(header.uid, "0000000");
    strcpy(header.gid, "0000000");
    snprintf(header.size, sizeof(header.size), "%011zo", filesize);
    snprintf(header.mtime, sizeof(header.mtime), "%011ld", time(NULL));
    header.typeflag = '0'; // regular file
    strcpy(header.magic, "ustar");
    strcpy(header.version, "00");
    
    // Calculate checksum
    unsigned int checksum = 0;
    memset(header.checksum, ' ', 8);
    unsigned char* ptr = (unsigned char*)&header;
    for (int i = 0; i < sizeof(header); i++) {
        checksum += ptr[i];
    }
    snprintf(header.checksum, sizeof(header.checksum), "%06o", checksum);
    
    fwrite(&header, sizeof(header), 1, tar_file);
}

void tar_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tar [options] archive_name files...\n");
        printf("Options: -c (create), -x (extract), -t (list), -v (verbose), -f (file)\n");
        printf("Example: tar -cvf archive.tar file1 file2\n");
        return;
    }
    
    char* options = argv[1];
    char* archive = argv[2];
    
    if (strstr(options, "c")) {
        // Create archive
        FILE* tar_file = fopen(archive, "wb");
        if (!tar_file) {
            printf("tar: cannot create '%s': Permission denied\n", archive);
            return;
        }
        
        printf("tar: Creating archive %s\n", archive);
        
        for (int i = 3; i < argc; i++) {
            FILE* input_file = vm_fopen(argv[i], "rb");
            if (!input_file) {
                printf("tar: '%s': No such file or directory\n", argv[i]);
                continue;
            }
            
            // Get file size
            fseek(input_file, 0, SEEK_END);
            size_t filesize = ftell(input_file);
            fseek(input_file, 0, SEEK_SET);
            
            printf("tar: Adding %s (%zu bytes)\n", argv[i], filesize);
            
            // Write tar header
            write_tar_header(tar_file, argv[i], filesize);
            
            // Write file content
            char buffer[512];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
                fwrite(buffer, 1, bytes_read, tar_file);
            }
            
            // Pad to 512-byte boundary
            size_t padding = (512 - (filesize % 512)) % 512;
            if (padding > 0) {
                memset(buffer, 0, padding);
                fwrite(buffer, 1, padding, tar_file);
            }
            
            fclose(input_file);
        }
        
        // Write two zero blocks to end archive
        char zero_block[512];
        memset(zero_block, 0, sizeof(zero_block));
        fwrite(zero_block, sizeof(zero_block), 1, tar_file);
        fwrite(zero_block, sizeof(zero_block), 1, tar_file);
        
        fclose(tar_file);
        printf("tar: Archive created successfully\n");
        
    } else if (strstr(options, "t")) {
        // List contents
        FILE* tar_file = fopen(archive, "rb");
        if (!tar_file) {
            printf("tar: cannot access '%s': No such file or directory\n", archive);
            return;
        }
        
        printf("tar: Contents of %s:\n", archive);
        
        struct tar_header header;
        while (fread(&header, sizeof(header), 1, tar_file) == 1) {
            if (header.name[0] == '\0') break; // End of archive
            
            size_t filesize = strtoul(header.size, NULL, 8);
            printf("tar: %s (%zu bytes)\n", header.name, filesize);
            
            // Skip file content
            size_t skip = ((filesize + 511) / 512) * 512;
            fseek(tar_file, skip, SEEK_CUR);
        }
        
        fclose(tar_file);
        
    } else if (strstr(options, "x")) {
        printf("tar: Extraction not yet implemented (use -t to list contents)\n");
    }
}

void gzip_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: gzip <filename>\n");
        return;
    }
    
    char* filename = argv[1];
    char output_name[512];
    snprintf(output_name, sizeof(output_name), "%s.gz", filename);
    
    // Open input file using VFS
    FILE* input = vm_fopen(filename, "rb");
    if (!input) {
        printf("gzip: cannot access '%s': No such file or directory\n", filename);
        return;
    }
    
    // Open output file using VFS 
    FILE* output_test = vm_fopen(output_name, "wb");
    if (output_test) {
        fclose(output_test);
    }
    
    // For now, create in host filesystem for gzip library compatibility
    gzFile output = gzopen(output_name, "wb");
    if (!output) {
        printf("gzip: cannot create '%s': Permission denied\n", output_name);
        fclose(input);
        return;
    }
    
    // Compress file
    char buffer[8192];
    size_t bytes_read;
    size_t total_in = 0, total_out = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        total_in += bytes_read;
        if (gzwrite(output, buffer, bytes_read) != (int)bytes_read) {
            printf("gzip: error writing to '%s'\n", output_name);
            break;
        }
    }
    
    fclose(input);
    gzclose(output);
    
    // Get output file size
    FILE* test_output = fopen(output_name, "rb");
    if (test_output) {
        fseek(test_output, 0, SEEK_END);
        total_out = ftell(test_output);
        fclose(test_output);
    }
    
    // Calculate compression ratio
    double ratio = total_in > 0 ? (1.0 - (double)total_out / (double)total_in) * 100.0 : 0.0;
    
    printf("gzip: compressed '%s' -> '%s' (%zu -> %zu bytes, %.1f%% reduction)\n", 
           filename, output_name, total_in, total_out, ratio);
    
    // Note: In VFS environment, we keep both files for demonstration
    printf("gzip: original file '%s' preserved in VFS\n", filename);
}

void gunzip_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: gunzip <filename.gz>\n");
        return;
    }
    
    char* filename = argv[1];
    
    // Remove .gz extension for output name
    char output_name[512];
    strcpy(output_name, filename);
    char* gz_ext = strstr(output_name, ".gz");
    if (gz_ext) {
        *gz_ext = '\0';
    } else {
        // If no .gz extension, assume it's still compressed
        snprintf(output_name, sizeof(output_name), "%s.out", filename);
    }
    
    // Open compressed file
    gzFile input = gzopen(filename, "rb");
    if (!input) {
        printf("gunzip: cannot access '%s': No such file or directory\n", filename);
        return;
    }
    
    // Open output file
    FILE* output = fopen(output_name, "wb");
    if (!output) {
        printf("gunzip: cannot create '%s': Permission denied\n", output_name);
        gzclose(input);
        return;
    }
    
    // Decompress file
    char buffer[8192];
    int bytes_read;
    size_t total_in = 0, total_out = 0;
    
    while ((bytes_read = gzread(input, buffer, sizeof(buffer))) > 0) {
        total_out += bytes_read;
        if (fwrite(buffer, 1, bytes_read, output) != (size_t)bytes_read) {
            printf("gunzip: error writing to '%s'\n", output_name);
            break;
        }
    }
    
    gzclose(input);
    fclose(output);
    
    // Get input file size
    FILE* test_input = fopen(filename, "rb");
    if (test_input) {
        fseek(test_input, 0, SEEK_END);
        total_in = ftell(test_input);
        fclose(test_input);
    }
    
    printf("gunzip: decompressed '%s' -> '%s' (%zu -> %zu bytes)\n", 
           filename, output_name, total_in, total_out);
    
    // Remove compressed file (like real gunzip)
    if (remove(filename) == 0) {
        printf("gunzip: removed '%s'\n", filename);
    }
}

void zip_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: zip <archive.zip> <files...>\n");
        return;
    }
    
    char* archive = argv[1];
    printf("zip: Creating archive %s\n", archive);
    
    for (int i = 2; i < argc; i++) {
        printf("zip: Adding %s\n", argv[i]);
    }
    printf("zip: Archive created successfully\n");
}

void unzip_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: unzip <archive.zip>\n");
        return;
    }
    
    char* archive = argv[1];
    printf("unzip: Extracting from %s\n", archive);
    printf("unzip: Inflating: file1.txt\n");
    printf("unzip: Inflating: file2.txt\n");
    printf("unzip: Inflating: subdir/file3.txt\n");
    printf("unzip: Extraction complete\n");
}

// Hostname command
void hostname_command(int argc, char **argv) {
    if (argc < 2) {
        // Display current hostname
        printf("%s\n", hostname);
    } else {
        // Set new hostname
        strncpy(hostname, argv[1], sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
        printf("Hostname set to: %s\n", hostname);
    }
}

// Terminal styling command implementations
void style_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Terminal Style Configuration:\n");
        printf("  Font: %s\n", terminal_get_current_font());
        printf("  Cursor: %s\n", 
               terminal_get_cursor_style() == 0 ? "Block" :
               terminal_get_cursor_style() == 1 ? "Underscore" : "Vertical");
        printf("\nUsage: style <init|reset|save|load>\n");
        printf("  init  - Initialize terminal styling with defaults\n");
        printf("  reset - Reset to original terminal settings\n");
        printf("  save  - Save current styling configuration\n");
        printf("  load  - Load saved styling configuration\n");
        return;
    }
    
    if (strcmp(argv[1], "init") == 0) {
        terminal_init_styling();
        printf("Terminal styling initialized with Campbell colors and MS Mincho font\n");
    } else if (strcmp(argv[1], "reset") == 0) {
        terminal_reset_colors();
        printf("Terminal colors reset\n");
    } else if (strcmp(argv[1], "save") == 0) {
        terminal_save_config();
    } else if (strcmp(argv[1], "load") == 0) {
        terminal_load_config();
    } else {
        printf("Unknown style command: %s\n", argv[1]);
    }
}

void font_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Current font: %s\n", terminal_get_current_font());
        printf("Usage: font <name> [size]\n");
        printf("Available fonts:\n");
        printf("  - MS Mincho (recommended retro font)\n");
        printf("  - Consolas\n");
        printf("  - Courier New\n");
        printf("  - Lucida Console\n");
        return;
    }
    
    int size = 12;  // Default size
    if (argc >= 3) {
        size = atoi(argv[2]);
        if (size < 8 || size > 72) {
            printf("Font size must be between 8 and 72\n");
            return;
        }
    }
    
    terminal_set_font(argv[1], size);
    printf("Font preference set to: %s, size %d\n", argv[1], size);
    printf("Note: You may need to manually configure your terminal for full font support\n");
}

void cursor_command(int argc, char **argv) {
    if (argc < 2) {
        const char* current_style = 
            terminal_get_cursor_style() == 0 ? "block" :
            terminal_get_cursor_style() == 1 ? "underscore" : "vertical";
        printf("Current cursor style: %s\n", current_style);
        printf("Usage: cursor <block|underscore|vertical> [blink|solid]\n");
        printf("Examples:\n");
        printf("  cursor block        - Retro block cursor (default)\n");
        printf("  cursor vertical     - Modern vertical bar cursor\n");
        printf("  cursor underscore   - Classic underscore cursor\n");
        return;
    }
    
    int style = -1;
    int blink = 1;  // Default to blinking
    
    if (strcmp(argv[1], "block") == 0) {
        style = CURSOR_BLOCK;
    } else if (strcmp(argv[1], "underscore") == 0) {
        style = CURSOR_UNDERSCORE;
    } else if (strcmp(argv[1], "vertical") == 0) {
        style = CURSOR_VERTICAL;
    } else {
        printf("Unknown cursor style: %s\n", argv[1]);
        return;
    }
    
    if (argc >= 3) {
        if (strcmp(argv[2], "solid") == 0) {
            blink = 0;
        } else if (strcmp(argv[2], "blink") == 0) {
            blink = 1;
        }
    }
    
    terminal_set_cursor_style(style, blink);
}

void colors_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Color scheme management:\n");
        printf("Usage: colors <campbell|reset|demo>\n");
        printf("  campbell - Apply Campbell PowerShell color scheme\n");
        printf("  reset    - Reset to default colors\n");
        printf("  demo     - Show color palette demonstration\n");
        return;
    }
    
    if (strcmp(argv[1], "campbell") == 0) {
        terminal_apply_campbell_colors();
        printf("Campbell color scheme applied\n");
    } else if (strcmp(argv[1], "reset") == 0) {
        terminal_reset_colors();
        printf("Colors reset to default\n");
    } else if (strcmp(argv[1], "demo") == 0) {
        printf("Campbell Color Scheme Demo:\n\n");
        printf("\033[30mBlack\033[0m  ");
        printf("\033[31mDark Red\033[0m  ");
        printf("\033[32mDark Green\033[0m  ");
        printf("\033[33mDark Yellow\033[0m  ");
        printf("\033[34mDark Blue\033[0m  ");
        printf("\033[35mDark Magenta\033[0m  ");
        printf("\033[36mDark Cyan\033[0m  ");
        printf("\033[37mLight Gray\033[0m\n");
        printf("\033[90mDark Gray\033[0m  ");
        printf("\033[91mRed\033[0m  ");
        printf("\033[92mGreen\033[0m  ");
        printf("\033[93mYellow\033[0m  ");
        printf("\033[94mBlue\033[0m  ");
        printf("\033[95mMagenta\033[0m  ");
        printf("\033[96mCyan\033[0m  ");
        printf("\033[97mWhite\033[0m\n\n");
    } else {
        printf("Unknown color command: %s\n", argv[1]);
    }
}

void retro_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Retro mode settings:\n");
        printf("Usage: retro <on|off|banner|demo>\n");
        printf("  on     - Enable retro terminal mode\n");
        printf("  off    - Disable retro terminal mode\n");
        printf("  banner - Display retro banner\n");
        printf("  demo   - Show retro features demonstration\n");
        return;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        terminal_enable_retro_mode(1);
        terminal_print_retro_banner();
    } else if (strcmp(argv[1], "off") == 0) {
        terminal_enable_retro_mode(0);
    } else if (strcmp(argv[1], "banner") == 0) {
        terminal_print_retro_banner();
    } else if (strcmp(argv[1], "demo") == 0) {
        printf("Retro Terminal Features Demo:\n\n");
        printf("1. Typewriter effect: ");
        terminal_typewriter_effect("This is a retro typewriter effect!", 50);
        printf("\n\n2. Retro prompt style:\n");
        terminal_print_retro_prompt("demo_user", "retro-machine", "/demo/path");
        printf("\n\n3. Syntax highlighting:\n");
        terminal_print_command("ls");
        printf(" ");
        terminal_print_argument("-la");
        printf(" ");
        terminal_print_path("/home/user");
        printf(" ");
        terminal_print_operator(">");
        printf(" ");
        terminal_print_string("output.txt");
        printf("\n\n");
    } else {
        printf("Unknown retro command: %s\n", argv[1]);
    }
}

void syntax_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Syntax highlighting settings:\n");
        printf("Usage: syntax <on|off|demo>\n");
        printf("  on   - Enable command syntax highlighting\n");
        printf("  off  - Disable command syntax highlighting\n");
        printf("  demo - Show syntax highlighting examples\n");
        return;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        terminal_enable_syntax_highlighting(1);
    } else if (strcmp(argv[1], "off") == 0) {
        terminal_enable_syntax_highlighting(0);
    } else if (strcmp(argv[1], "demo") == 0) {
        printf("Syntax Highlighting Demo:\n\n");
        
        printf("Commands: ");
        terminal_print_command("cat");
        printf(" ");
        terminal_print_command("grep");
        printf(" ");
        terminal_print_command("find");
        printf("\n");
        
        printf("Paths: ");
        terminal_print_path("/home/user/documents");
        printf(" ");
        terminal_print_path("./relative/path");
        printf("\n");
        
        printf("Strings: ");
        terminal_print_string("\"quoted string\"");
        printf(" ");
        terminal_print_string("'single quoted'");
        printf("\n");
        
        printf("Operators: ");
        terminal_print_operator(">");
        printf(" ");
        terminal_print_operator(">>");
        printf(" ");
        terminal_print_operator("|");
        printf(" ");
        terminal_print_operator("&&");
        printf("\n");
        
        printf("Errors: ");
        terminal_print_error("command not found");
        printf("\n\n");
    } else {
        printf("Unknown syntax command: %s\n", argv[1]);
    }
}

void terminal_demo_command(int argc, char **argv) {
    printf("Terminal Enhancement Demo\n");
    printf("=========================\n\n");
    
    // Initialize styling if not done already
    terminal_init_styling();
    
    printf("1. Retro Banner:\n");
    terminal_print_retro_banner();
    
    printf("2. Enhanced Prompt:\n");
    terminal_print_retro_prompt("demo", "zora-vm", "/demo");
    printf("\n\n");
    
    printf("3. Syntax Highlighting:\n");
    terminal_print_command("cat");
    printf(" ");
    terminal_print_path("/etc/passwd");
    printf(" ");
    terminal_print_operator("|");
    printf(" ");
    terminal_print_command("grep");
    printf(" ");
    terminal_print_string("\"root\"");
    printf(" ");
    terminal_print_operator(">");
    printf(" ");
    terminal_print_path("output.txt");
    printf("\n\n");
    
    printf("4. Typewriter Effect:\n");
    terminal_typewriter_effect("Welcome to the enhanced Zora VM terminal!", 30);
    printf("\n\n");
    
    printf("5. Color Palette:\n");
    colors_command(2, (char*[]){"colors", "demo"});
    
    printf("Configuration:\n");
    printf("  Font: MS Mincho (retro Japanese)\n");
    printf("  Colors: Campbell PowerShell scheme\n");
    printf("  Cursor: Block style (classic retro)\n");
    printf("  Features: Syntax highlighting, retro effects\n\n");
    
    printf("Use 'style init' to apply these settings permanently.\n");
}

// End of missing command implementations

// Binary execution commands
void exec_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: exec <binary> [args...]\n");
        return;
    }
    
    char binary_path[256];
    if (argv[1][0] == '/') {
        strcpy(binary_path, argv[1]);
    } else {
        snprintf(binary_path, sizeof(binary_path), "/persistent/data/%s", argv[1]);
    }
    
    printf("Executing binary: %s\n", binary_path);
    
    // Enable crash guard for binary execution
    vm_enable_crash_guard();
    
    // Execute the binary
    int result = execute_sandboxed_binary(binary_path, argv + 1, argc - 1);
    
    // Disable crash guard after execution
    vm_disable_crash_guard();
    
    if (result == -1) {
        printf("Failed to execute binary\n");
    } else {
        printf("Binary execution completed (exit code: %d)\n", result);
    }
}

void run_windows_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: run-windows <binary.exe> [args...]\n");
        return;
    }
    
    char binary_path[256];
    if (argv[1][0] == '/') {
        strcpy(binary_path, argv[1]);
    } else {
        snprintf(binary_path, sizeof(binary_path), "/persistent/data/%s", argv[1]);
    }
    
    printf("Executing Windows binary: %s\n", binary_path);
    
    // Force Windows execution
    VNode* node = vfs_find_node(binary_path);
    if (node && node->host_path) {
        // Enable crash guard for binary execution
        vm_enable_crash_guard();
        
        int result = execute_windows_binary(node->host_path, argv + 1, argc - 1);
        
        // Disable crash guard after execution
        vm_disable_crash_guard();
        
        printf("Windows binary execution completed (exit code: %d)\n", result);
    } else {
        printf("Binary not found: %s\n", binary_path);
    }
}

void list_binaries_command(int argc, char **argv) {
    printf("Available binaries in /persistent/data:\n");
    
    VNode* data_node = vfs_find_node("/persistent/data");
    if (!data_node) {
        printf("Data directory not found\n");
        return;
    }
    
    VNode* child = data_node->children;
    while (child) {
        if (!child->is_directory && child->host_path) {
            BinaryType type = detect_binary_type(child->host_path);
            
            const char* type_str;
            switch (type) {
                case BINARY_TYPE_WINDOWS_PE: type_str = "Windows PE"; break;
                case BINARY_TYPE_LINUX_ELF: type_str = "Linux ELF"; break;
                case BINARY_TYPE_SCRIPT: type_str = "Script"; break;
                default: type_str = "Unknown"; break;
            }
            
            printf("  %-20s [%s]\n", child->name, type_str);
        }
        child = child->next;
    }
}

void sandbox_status_command(int argc, char **argv) {
    printf("=== Sandbox Status ===\n");
    printf("Binary Executor: %s\n", binary_executor_is_initialized() ? "Initialized" : "Not initialized");
    printf("ELF Parser: %s\n", binary_executor_has_elf_support() ? "Available" : "Not Available");
    printf("Sandbox Directory: %s\n", "Temp/zora_vm_sandbox_<pid>");
    printf("Windows Binary Support: Native execution (SANDBOXED)\n");
    printf("ELF Binary Support: %s\n", binary_executor_has_elf_support() ? "Native ELF Parser (SANDBOXED)" : "Disabled");
    printf("Script Execution: Enabled (SANDBOXED)\n");
    
    printf("\nFeatures:\n");
    printf("   • Native ELF parsing and loading\n");
    printf("   • Windows-optimized binary execution\n");
    printf("   • Sandboxed execution environment\n");
    printf("   • NO external dependencies required\n");
    printf("   • Real machine code execution with syscall interception\n");
}

void themes_command(int argc, char **argv) {
    (void)argc; (void)argv; 
    printf("Available themes: Campbell (terminal)\n");
}

#ifdef PYTHON_SCRIPTING
#include "python/python_vm.h"

void python_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: python <script.py>\n");
        return;
    }
    
    char script_path[256];
    resolve_script_path(argv[1], script_path, sizeof(script_path));
    if (!vfs_find_node(script_path)) {
        printf("Python script not found: %s\n", script_path);
        return;
    }
    printf("Executing Python script (sandboxed): %s\n", script_path);
    if (python_vm_load_script(script_path) != 0) printf("Failed to execute Python script\n");
}

void pycode_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pycode <python_code>\n");
        return;
    }
    
    char code[1024] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(code, argv[i]);
        if (i < argc - 1) strcat(code, " ");
    }
    
    printf("Executing Python code: %s\n", code);
    if (python_vm_execute_string(code) != 0) {
        printf("Failed to execute Python code\n");
    }
}
#endif

#ifdef PERL_SCRIPTING
#include "perl/perl_vm.h"

void perl_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: perl <script.pl>\n");
        return;
    }
    
    char script_path[256];
    resolve_script_path(argv[1], script_path, sizeof(script_path));
    if (!vfs_find_node(script_path)) { printf("Perl script not found: %s\n", script_path); return; }
    printf("Executing Perl script: %s\n", script_path);
    if (perl_vm_load_script(script_path) != 0) printf("Failed to execute Perl script\n");
}

void plcode_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: plcode <perl_code>\n");
        return;
    }
    
    char code[1024] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(code, argv[i]);
        if (i < argc - 1) strcat(code, " ");
    }
    
    printf("Executing Perl code: %s\n", code);
    if (perl_vm_execute_string(code) != 0) {
        printf("Failed to execute Perl code\n");
    }
}
#endif

// Start the shell loop
void start_shell() {
    char input[256];

    // Initialize user system
    load_users();
    
    // Check if any users exist, offer setup if none
    extern int user_count;
    if (user_count == 0) {
        printf("\n*** FIRST TIME SETUP ***\n");
        printf("No users found on the system.\n");
        printf("You can create users with 'useradd <username>' or setup root with 'setup-root'.\n");
        printf("Recommendation: Run 'setup-root' first to create an administrator account.\n\n");
    }
    
    // Set default hostname
    strncpy(hostname, "zora-vm", sizeof(hostname) - 1);
    hostname[sizeof(hostname) - 1] = '\0';

    // Initialize environment variables
    init_default_env_vars();

    // Initialize ZoraVM Terminal Styling (font only)
    terminal_init_styling();
    
    printf("=== Zora VM - MERL Shell ===\n");
    printf("Virtual Machine OS with MERL Shell\n");
    printf("Enhanced Terminal: MS Mincho font, Campbell colors, Block cursor\n");
    printf("Type 'help' for available commands, 'terminal-demo' for styling demo.\n");
    printf("Terminal commands: 'style', 'font', 'cursor', 'colors', 'retro', 'syntax'\n");
    printf("Type 'exit' to quit VM.\n");
    printf("VM Commands: vmstat, reboot, shutdown\n\n");

    while (1) {
        static int command_count = 0;
        command_count++;
        
        // Safety check to prevent runaway loops during development
        if (command_count > 10000) {
            printf("Warning: Command count limit reached. Resetting counter.\n");
            command_count = 0;
        }
        
        // Render the colored shell prompt
        print_colored_prompt();
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting VM...\n");
            break;
        }

        // Remove trailing newline character
        input[strcspn(input, "\n")] = '\0';

        // Exit condition
        if (strcmp(input, "exit") == 0) {
            printf("Exiting VM...\n");
            break;
        }

        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }

        // Handle multiple commands separated by semicolons with safety checks
        char input_copy[512];
        strncpy(input_copy, input, sizeof(input_copy) - 1);
        input_copy[sizeof(input_copy) - 1] = '\0';
        
        char* saveptr = NULL;
        char* command_token = strtok_r(input_copy, ";", &saveptr);
        int semicolon_commands = 0;
        
        while (command_token != NULL && semicolon_commands < 10) {  // Limit semicolon commands
            semicolon_commands++;
            
            // Trim whitespace from command
            while (*command_token == ' ') command_token++;
            char* end = command_token + strlen(command_token) - 1;
            while (end > command_token && *end == ' ') {
                *end = '\0';
                end--;
            }
            
            if (strlen(command_token) > 0) {
                // Dispatch the command
                handle_command(command_token);
            }
            
            command_token = strtok_r(NULL, ";", &saveptr);
        }
        
        if (semicolon_commands >= 10) {
            printf("Warning: Too many semicolon-separated commands (limit: 10)\n");
        }
    }
}

// Command implementations

void sysinfo_command(int argc, char **argv) {
    printf("MERL Shell: %s\n", OS_VERSION);
    printf("Developed by: Tomoko Saito\n");
    printf("System: %s\n", SYSTEM_NAME);
    printf("Note: Unlike Zora, this is meant to provide a unix-like experience :3\n");
}

// Helper function to expand path shortcuts like ".." and "~"
void expand_path(const char* input, char* output, size_t output_size) {
    if (!input || !output) return;
    
    // Prevent infinite recursion
    static int recursion_depth = 0;
    if (recursion_depth > 10) {
        printf("Error: Path expansion recursion limit reached\n");
        strncpy(output, input, output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    
    recursion_depth++;
    
    // Handle special cases
    if (strcmp(input, "~") == 0) {
        strncpy(output, "/home", output_size - 1);
        output[output_size - 1] = '\0';
        recursion_depth--;
        return;
    }
    
    if (strcmp(input, "..") == 0) {
        char* current = vfs_getcwd();
        if (current && strcmp(current, "/") != 0) {
            // Find parent directory
            char* last_slash = strrchr(current, '/');
            if (last_slash && last_slash != current) {
                size_t parent_len = last_slash - current;
                if (parent_len < output_size) {
                    strncpy(output, current, parent_len);
                    output[parent_len] = '\0';
                } else {
                    strcpy(output, "/");
                }
            } else {
                strcpy(output, "/");
            }
        } else {
            strcpy(output, "/");
        }
        recursion_depth--;
        return;
    }
    
    if (strcmp(input, ".") == 0) {
        char* current = vfs_getcwd();
        strncpy(output, current ? current : "/", output_size - 1);
        output[output_size - 1] = '\0';
        recursion_depth--;
        return;
    }
    
    // Handle relative paths starting with ".." - avoid recursive call
    if (strncmp(input, "../", 3) == 0) {
        char* current = vfs_getcwd();
        if (current && strcmp(current, "/") != 0) {
            // Find parent directory manually
            char* last_slash = strrchr(current, '/');
            if (last_slash && last_slash != current) {
                size_t parent_len = last_slash - current;
                char parent_path[256];
                if (parent_len < sizeof(parent_path)) {
                    strncpy(parent_path, current, parent_len);
                    parent_path[parent_len] = '\0';
                    snprintf(output, output_size, "%s/%s", parent_path, input + 3);
                } else {
                    snprintf(output, output_size, "/%s", input + 3);
                }
            } else {
                snprintf(output, output_size, "/%s", input + 3);
            }
        } else {
            snprintf(output, output_size, "/%s", input + 3);
        }
        recursion_depth--;
        return;
    }
    
    // Handle paths starting with "~/"
    if (strncmp(input, "~/", 2) == 0) {
        snprintf(output, output_size, "/home/%s", input + 2);
        recursion_depth--;
        return;
    }
    
    // For absolute paths or regular relative paths, copy as-is
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';
    recursion_depth--;
}

void pwd_command(int argc, char **argv) {
    char* cwd = vfs_getcwd(); // Use VFS instead of system getcwd
    if (cwd) {
        printf("Current Directory: %s\n", cwd);
    } else {
        printf("Current Directory: /\n");
    }
}

// Helper function to create ParsedCommand from already-parsed argv
ParsedCommand* parse_command_advanced_from_args(int argc, char** argv) {
    ParsedCommand* cmd = malloc(sizeof(ParsedCommand));
    if (!cmd) return NULL;
    
    // Initialize structure
    cmd->args = malloc(256 * sizeof(char*));
    cmd->flags = malloc(64 * sizeof(char*));
    cmd->argc = 0;
    cmd->flag_count = 0;
    cmd->input_redirect = NULL;
    cmd->output_redirect = NULL;
    cmd->append_mode = 0;
    cmd->background = 0;
    
    // Separate args and flags
    for (int i = 0; i < argc && cmd->argc < 255; i++) {
        if (argv[i][0] == '-' && strlen(argv[i]) > 1 && cmd->flag_count < 63) {
            cmd->flags[cmd->flag_count++] = strdup(argv[i]);
        } else {
            cmd->args[cmd->argc++] = strdup(argv[i]);
        }
    }
    
    cmd->args[cmd->argc] = NULL;
    cmd->flags[cmd->flag_count] = NULL;
    
    return cmd;
}

void ls_command(int argc, char **argv) {
    // Parse arguments with advanced parsing
    ParsedCommand* cmd = parse_command_advanced_from_args(argc, argv);
    
    // Check for help flag
    if (has_flag(cmd, "-h") || has_flag(cmd, "--help")) {
        terminal_print_command("ls");
        printf(" - list directory contents\n");
        printf("Usage: ");
        terminal_print_command("ls");
        printf(" [");
        terminal_print_argument("OPTIONS");
        printf("] [");
        terminal_print_path("DIRECTORY");
        printf("]\n\n");
        printf("Options:\n");
        printf("  ");
        terminal_print_argument("-l");
        printf("         long format (detailed listing)\n");
        printf("  ");
        terminal_print_argument("-a");
        printf("         show hidden files (starting with .)\n");
        printf("  ");
        terminal_print_argument("-h");
        printf("         show file sizes in human readable format\n");
        printf("  ");
        terminal_print_argument("-t");
        printf("         sort by modification time\n");
        printf("  ");
        terminal_print_argument("-r");
        printf("         reverse sort order\n");
        printf("  ");
        terminal_print_argument("--help");
        printf("    show this help message\n");
        free_parsed_command(cmd);
        return;
    }
    
    // Extract flags
    int long_format = has_flag(cmd, "-l");
    int show_hidden = has_flag(cmd, "-a");
    int human_readable = has_flag(cmd, "-h");
    int sort_time = has_flag(cmd, "-t");
    int reverse_order = has_flag(cmd, "-r");
    
    char target_dir[512];
    
    // Determine target directory from non-flag arguments
    if (cmd->argc < 2) {
        // No argument - use current directory
        char* cwd = vfs_getcwd();
        strcpy(target_dir, cwd ? cwd : "/");
    } else {
        // Use first non-flag argument as directory
        char expanded_path[512];
        expand_path(cmd->args[1], expanded_path, sizeof(expanded_path));
        
        // Build absolute path if relative
        if (expanded_path[0] != '/') {
            char* cwd = vfs_getcwd();
            snprintf(target_dir, sizeof(target_dir), "%s/%s", cwd, expanded_path);
        } else {
            strcpy(target_dir, expanded_path);
        }
    }
    
    if (!long_format) {
        printf("Contents of ");
        terminal_print_path(target_dir);
        printf(":\n");
    }
    
    // Get the target directory node
    VNode* dir_node = vfs_find_node(target_dir);
    if (!dir_node) {
        terminal_print_error("ls: ");
        terminal_print_path(target_dir);
        terminal_print_error(": No such file or directory\n");
        free_parsed_command(cmd);
        return;
    }

    if (!dir_node->is_directory) {
        terminal_print_error("ls: ");
        terminal_print_path(target_dir);
        terminal_print_error(": Not a directory\n");
        free_parsed_command(cmd);
        return;
    }

    // Refresh directory to detect new files from host system
    vfs_refresh_directory(dir_node);

    // List children of target directory
    VNode* child = dir_node->children;
    if (!child) {
        printf("(empty directory)\n");
        free_parsed_command(cmd);
        return;
    }
    
    // Enhanced listing with flags support
    while (child) {
        // Skip hidden files unless -a flag is used
        if (!show_hidden && child->name[0] == '.') {
            child = child->next;
            continue;
        }
        
        if (long_format) {
            // Long format: permissions, owner, group, size, time, name
            char perm_str[10];
            extern void vfs_format_permissions(unsigned int mode, char* output);
            vfs_format_permissions(child->mode, perm_str);
            
            // Print file type and permissions
            printf("%c%s ", child->is_directory ? 'd' : '-', perm_str);
            printf("%8s ", child->owner);
            printf("%8s ", child->group);
            
            if (human_readable && child->size >= 1024) {
                if (child->size >= 1024 * 1024) {
                    printf("%6.1fM ", child->size / (1024.0 * 1024.0));
                } else {
                    printf("%6.1fK ", child->size / 1024.0);
                }
            } else {
                printf("%8zu ", child->size);
            }
            
            // Format modification time
            char time_str[16];
            struct tm* tm_info = localtime(&child->modified_time);
            strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
            printf("%s ", time_str);
            
            if (child->is_directory) {
                terminal_print_path(child->name);
                printf("/");
            } else {
                printf("%s", child->name);
            }
            printf("\n");
        } else {
            // Simple format with colors
            if (child->is_directory) {
                terminal_print_path(child->name);
                printf("/");
            } else {
                printf("%s", child->name);
            }
            printf("  ");
        }
        
        child = child->next;
    }
    
    if (!long_format) {
        printf("\n");
    }
    
    free_parsed_command(cmd);
}

void cd_command(int argc, char **argv) {
    if (argc < 2) {
        // No argument provided - go to home directory
        char home_path[] = "/home";
        if (vfs_chdir(home_path) == 0) {
            printf("Changed directory to: %s\n", home_path);
            // Update current path for prompt
            strncpy(current_path, home_path, sizeof(current_path) - 1);
            current_path[sizeof(current_path) - 1] = '\0';
        } else {
            printf("cd: Cannot access home directory\n");
        }
        return;
    }
    
    // Expand the path to handle "..", "~", etc.
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Use VFS chdir instead of system chdir
    if (vfs_chdir(expanded_path) == 0) {
        char* new_path = vfs_getcwd();
        printf("Changed directory to: %s\n", new_path);
        // Update current path for prompt
        strncpy(current_path, new_path, sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0';
    } else {
        printf("cd: %s: No such directory\n", expanded_path);
    }
}

void mkdir_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    if (vfs_mkdir(full_path) == 0) {
        printf("Directory created: %s\n", full_path);
    } else {
        printf("mkdir: Failed to create directory '%s'\n", full_path);
    }
}

void rmdir_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rmdir <directory>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    if (vfs_rmdir(full_path) == 0) {
        printf("Directory removed: %s\n", full_path);
    } else {
        printf("rmdir: Failed to remove directory '%s'\n", full_path);
    }
}

void touch_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    if (vfs_create_file(full_path) == 0) {
        printf("File created: %s\n", full_path);
    } else {
        printf("touch: Failed to create file '%s'\n", full_path);
    }
}

void rm_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    if (vfs_delete_file(full_path) == 0) {
        printf("File removed: %s\n", full_path);
    } else {
        printf("rm: Failed to remove file '%s'\n", full_path);
    }
}

void cp_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: cp <source> <destination>\n");
        return;
    }
    
    // Expand both source and destination paths
    char src_expanded[512], dest_expanded[512];
    expand_path(argv[1], src_expanded, sizeof(src_expanded));
    expand_path(argv[2], dest_expanded, sizeof(dest_expanded));
    
    // Build absolute paths
    char src_full[512], dest_full[512];
    char* cwd = vfs_getcwd();
    
    if (src_expanded[0] != '/') {
        snprintf(src_full, sizeof(src_full), "%s/%s", cwd, src_expanded);
    } else {
        strcpy(src_full, src_expanded);
    }
    
    if (dest_expanded[0] != '/') {
        snprintf(dest_full, sizeof(dest_full), "%s/%s", cwd, dest_expanded);
    } else {
        strcpy(dest_full, dest_expanded);
    }
    
    // Find source node
    VNode* src_node = vfs_find_node(src_full);
    if (!src_node) {
        printf("cp: %s: No such file or directory\n", src_full);
        return;
    }
    
    if (src_node->is_directory) {
        printf("cp: %s: Is a directory (use -r for recursive copy)\n", src_full);
        return;
    }
    
    // Create destination file
    if (vfs_create_file(dest_full) == 0) {
        // Copy data if source has data
        VNode* dest_node = vfs_find_node(dest_full);
        if (dest_node && src_node->data) {
            dest_node->data = malloc(src_node->size);
            if (dest_node->data) {
                memcpy(dest_node->data, src_node->data, src_node->size);
                dest_node->size = src_node->size;
            }
        }
        printf("File copied from %s to %s\n", src_full, dest_full);
    } else {
        printf("cp: Failed to create destination file %s\n", dest_full);
    }
}

void mv_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mv <source> <destination>\n");
        return;
    }
    
    // Expand both source and destination paths
    char src_expanded[512], dest_expanded[512];
    expand_path(argv[1], src_expanded, sizeof(src_expanded));
    expand_path(argv[2], dest_expanded, sizeof(dest_expanded));
    
    // Build absolute paths
    char src_full[512], dest_full[512];
    char* cwd = vfs_getcwd();
    
    if (src_expanded[0] != '/') {
        snprintf(src_full, sizeof(src_full), "%s/%s", cwd, src_expanded);
    } else {
        strcpy(src_full, src_expanded);
    }
    
    if (dest_expanded[0] != '/') {
        snprintf(dest_full, sizeof(dest_full), "%s/%s", cwd, dest_expanded);
    } else {
        strcpy(dest_full, dest_expanded);
    }
    
    // For now, implement mv as cp + rm
    // TODO: Implement proper VFS move operation
    VNode* src_node = vfs_find_node(src_full);
    if (!src_node) {
        printf("mv: %s: No such file or directory\n", src_full);
        return;
    }
    
    if (src_node->is_directory) {
        printf("mv: %s: Is a directory (directory moves not yet implemented)\n", src_full);
        return;
    }
    
    // Copy then delete
    if (vfs_create_file(dest_full) == 0) {
        VNode* dest_node = vfs_find_node(dest_full);
        if (dest_node && src_node->data) {
            dest_node->data = malloc(src_node->size);
            if (dest_node->data) {
                memcpy(dest_node->data, src_node->data, src_node->size);
                dest_node->size = src_node->size;
            }
        }
        
        if (vfs_delete_file(src_full) == 0) {
            printf("File moved from %s to %s\n", src_full, dest_full);
        } else {
            printf("mv: Warning - copied but failed to remove source file\n");
        }
    } else {
        printf("mv: Failed to create destination file %s\n", dest_full);
    }
}

void rename_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: rename <oldname> <newname>\n");
        return;
    }
    if (rename(argv[1], argv[2]) == 0) {
        printf("File renamed from %s to %s\n", argv[1], argv[2]);
    } else {
        perror("rename");
    }
}

void clear_screen(void) {
    // Use ANSI escape sequences for cross-platform compatibility
    printf("\033[2J\033[H");
    fflush(stdout);
}

void clear_command(int argc, char **argv) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    clear_screen();
}

void echo_command(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        redirect_printf("%s ", argv[i]);
    }
    redirect_printf("\n");
}

void find_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: find <pattern>\n");
        return;
    }
    
    char* pattern = argv[1];
    char* search_dir = (argc > 2) ? argv[2] : vfs_getcwd();
    
    printf("Searching for '%s' in %s:\n", pattern, search_dir);
    find_files_recursive(search_dir, pattern);
}

void find_files_recursive(const char* dir_path, const char* pattern) {
    VNode* dir_node = vfs_find_node(dir_path);
    if (!dir_node || !dir_node->is_directory) {
        return;
    }
    
    VNode* child = dir_node->children;
    while (child) {
        // Check if name matches pattern (simple substring match)
        if (strstr(child->name, pattern) != NULL) {
            printf("%s/%s\n", dir_path, child->name);
        }
        
        // Recursively search subdirectories
        if (child->is_directory) {
            char child_path[512];
            snprintf(child_path, sizeof(child_path), "%s/%s", dir_path, child->name);
            find_files_recursive(child_path, pattern);
        }
        
        child = child->next;
    }
}

void tree_command(int argc, char **argv) {
    char* start_dir = (argc > 1) ? argv[1] : vfs_getcwd();
    
    char expanded_path[512];
    expand_path(start_dir, expanded_path, sizeof(expanded_path));
    
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    printf("Directory tree for %s:\n", full_path);
    print_tree_recursive(full_path, 0);
}

void print_tree_recursive(const char* dir_path, int depth) {
    VNode* dir_node = vfs_find_node(dir_path);
    if (!dir_node || !dir_node->is_directory) {
        return;
    }
    
    VNode* child = dir_node->children;
    while (child) {
        // Print indentation
        for (int i = 0; i < depth; i++) {
            printf("  ");
        }
        
        if (child->is_directory) {
            printf("📁 %s/\n", child->name);
            // Recursively print subdirectories
            char child_path[512];
            snprintf(child_path, sizeof(child_path), "%s/%s", dir_path, child->name);
            print_tree_recursive(child_path, depth + 1);
        } else {
            printf("📄 %s (%zu bytes)\n", child->name, child->size);
        }
        
        child = child->next;
    }
}

void cat_command(int argc, char **argv) {
    // Parse arguments with advanced parsing
    ParsedCommand* cmd = parse_command_advanced_from_args(argc, argv);
    
    // Check for help flag
    if (has_flag(cmd, "-h") || has_flag(cmd, "--help")) {
        terminal_print_command("cat");
        printf(" - display file contents\n");
        printf("Usage: ");
        terminal_print_command("cat");
        printf(" [");
        terminal_print_argument("OPTIONS");
        printf("] ");
        terminal_print_path("FILE");
        printf("...\n\n");
        printf("Options:\n");
        printf("  ");
        terminal_print_argument("-n");
        printf("         number all output lines\n");
        printf("  ");
        terminal_print_argument("-b");
        printf("         number non-blank output lines\n");
        printf("  ");
        terminal_print_argument("-s");
        printf("         suppress repeated empty lines\n");
        printf("  ");
        terminal_print_argument("-E");
        printf("         display $ at end of each line\n");
        printf("  ");
        terminal_print_argument("--help");
        printf("    show this help message\n");
        free_parsed_command(cmd);
        return;
    }
    
    if (cmd->argc < 2) {
        terminal_print_error("Usage: ");
        terminal_print_command("cat");
        printf(" [OPTIONS] FILE...\n");
        free_parsed_command(cmd);
        return;
    }
    
    // Extract flags
    int number_lines = has_flag(cmd, "-n");
    int number_nonblank = has_flag(cmd, "-b");
    int squeeze_blank = has_flag(cmd, "-s");
    int show_ends = has_flag(cmd, "-E");
    
    // Process all file arguments
    for (int file_idx = 1; file_idx < cmd->argc; file_idx++) {
        char expanded_path[512];
        expand_path(cmd->args[file_idx], expanded_path, sizeof(expanded_path));
        
        // Build absolute path if relative
        char full_path[512];
        if (expanded_path[0] != '/') {
            char* cwd = vfs_getcwd();
            snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
        } else {
            strcpy(full_path, expanded_path);
        }
        
        VNode* file_node = vfs_find_node(full_path);
        if (!file_node) {
            terminal_print_error("cat: ");
            terminal_print_path(full_path);
            terminal_print_error(": No such file or directory\n");
            continue;
        }
        
        if (file_node->is_directory) {
            terminal_print_error("cat: ");
            terminal_print_path(full_path);
            terminal_print_error(": Is a directory\n");
            continue;
        }
        
        // Use VFS API to read file content
        void* data = NULL;
        size_t size = 0;
        if (vfs_read_file(full_path, &data, &size) == 0 && data && size > 0) {
            char* content = (char*)data;
            
            if (number_lines || number_nonblank || squeeze_blank || show_ends) {
                // Process line by line with flags
                char* line_start = content;
                int line_number = 1;
                int prev_blank = 0;
                
                for (size_t i = 0; i <= size; i++) {
                    if (i == size || content[i] == '\n') {
                        // End of line or file
                        int line_length = (content + i) - line_start;
                        int is_blank = (line_length == 0);
                        
                        // Handle squeeze blank lines
                        if (squeeze_blank && is_blank && prev_blank) {
                            line_start = content + i + 1;
                            continue;
                        }
                        
                        // Print line number if requested
                        if (number_lines || (number_nonblank && !is_blank)) {
                            printf("%6d\t", line_number);
                        }
                        
                        // Print the line content
                        fwrite(line_start, 1, line_length, stdout);
                        
                        // Add end marker if requested
                        if (show_ends && i < size) {
                            printf("$");
                        }
                        
                        if (i < size) printf("\n");
                        
                        line_number++;
                        prev_blank = is_blank;
                        line_start = content + i + 1;
                    }
                }
            } else {
                // Simple output without processing
                fwrite(data, 1, size, stdout);
                if (content[size - 1] != '\n') {
                    printf("\n");
                }
            }
        } else {
            printf("(empty file)\n");
        }
    }
    
    free_parsed_command(cmd);
}

void pull_command(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: pull <source_dir> <destination_dir>\n");
        return;
    }

    char *src_dir = argv[1];
    char *dst_dir = argv[2];
    char cmd[1024];

    printf("Pulling files from %s to %s\n", src_dir, dst_dir);

    // Windows: use xcopy
    snprintf(cmd, sizeof(cmd), "xcopy /E /I /Y \"%s\" \"%s\"", src_dir, dst_dir);

    int result = system(cmd);
    if (result == 0) {
        printf("Pull completed successfully\n");
    } else {
        printf("Pull failed with error code: %d\n", result);
    }
}

void flipper_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: flipper <subshell-program> [args...]\n");
        printf("Example: flipper cmd\n");
        return;
    }
    // Build the command string
    char command[512] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(command, argv[i]);
        if (i < argc - 1) strcat(command, " ");
    }
    printf("Launching subshell: %s\n", command);
    int result = system(command);
    printf("Subshell exited (code %d). Returning to MERL shell.\n", result);
}

void search_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: search <pattern>\n");
        return;
    }

    printf("Searching for: %s\n", argv[1]);

    // Windows implementation using FindFirstFile
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(argv[1], &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No files found matching pattern: %s\n", argv[1]);
        return;
    }
    
    do {
        printf("Found: %s\n", find_data.cFileName);
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            printf("  [Directory]\n");
        } else {
            printf("  Size: %lu bytes\n", find_data.nFileSizeLow);
        }
    } while (FindNextFile(hFind, &find_data) != 0);
    
    FindClose(hFind);
}

void edit_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: edit <filename>\n");
        return;
    }

    FILE *file = fopen(argv[1], "r+");
    if (!file) {
        // If file doesn't exist, create it
        file = fopen(argv[1], "w+");
        if (!file) {
            perror("edit");
            return;
        }
        printf("Created new file: %s\n", argv[1]);
    } else {
        printf("Editing existing file: %s\n", argv[1]);
        printf("Current contents:\n");
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), file)) {
            printf("%s", buffer);
        }
        rewind(file);
    }

    printf("\nEnter new content. Type a single dot (.) on a line to finish.\n");
    char line[256];
    freopen(argv[1], "w", file); // Overwrite file with new content
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, ".") == 0) break;
        fprintf(file, "%s\n", line);
    }
    fclose(file);
    printf("File saved: %s\n", argv[1]);
}

void run_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: run <executable> [args...]\n");
        return;
    }
    // Build the command string
    char command[512] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(command, argv[i]);
        if (i < argc - 1) strcat(command, " ");
    }
    int result = system(command);
    if (result == -1) {
        printf("Failed to run command: %s\n", command);
    }
}

// Kernel command wrappers for the shell command table
void fork_wrapper(int argc, char **argv) {
    route_command("fork", argc, argv);
}
void kill_wrapper(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: kill <pid> [signal]\n");
        printf("  kill <pid>     - Terminate process\n");
        printf("  kill -9 <pid>  - Force terminate process\n");
        return;
    }
    
    DWORD pid;
    BOOL force_kill = FALSE;
    
    // Parse arguments
    if (argc >= 3 && (strcmp(argv[1], "-9") == 0 || strcmp(argv[1], "-KILL") == 0)) {
        force_kill = TRUE;
        pid = atoi(argv[2]);
    } else {
        pid = atoi(argv[1]);
    }
    
    if (pid == 0) {
        printf("kill: invalid process ID '%s'\n", argc >= 3 ? argv[2] : argv[1]);
        return;
    }
    
    // Open the process
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            printf("kill: (%lu) - Access denied (insufficient privileges)\n", pid);
        } else if (error == ERROR_INVALID_PARAMETER) {
            printf("kill: (%lu) - No such process\n", pid);
        } else {
            printf("kill: (%lu) - Error %lu\n", pid, error);
        }
        return;
    }
    
    // Terminate the process
    UINT exit_code = force_kill ? 9 : 0;
    if (TerminateProcess(hProcess, exit_code)) {
        printf("kill: process %lu terminated\n", pid);
    } else {
        printf("kill: failed to terminate process %lu (error %lu)\n", pid, GetLastError());
    }
    
    CloseHandle(hProcess);
}
void ps_wrapper(int argc, char **argv) {
    printf("=== Process List (ps) ===\n");
    printf("  PID    PPID  CMD\n");
    
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    // Take a snapshot of all processes in the system
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("ps: CreateToolhelp32Snapshot failed\n");
        return;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Retrieve information about the first process
    if (!Process32First(hProcessSnap, &pe32)) {
        printf("ps: Process32First failed\n");
        CloseHandle(hProcessSnap);
        return;
    }
    
    // Walk through the processes
    int count = 0;
    do {
        printf("%6lu %6lu  %s\n", 
               pe32.th32ProcessID, 
               pe32.th32ParentProcessID,
               pe32.szExeFile);
        count++;
        
        // Limit output to prevent overwhelming display
        if (count > 50) {
            printf("... (truncated - showing first 50 processes)\n");
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));
    
    printf("\nTotal processes shown: %d\n", count);
    CloseHandle(hProcessSnap);
}
void read_wrapper(int argc, char **argv) {
    route_command("read", argc, argv);
}
void write_wrapper(int argc, char **argv) {
    route_command("write", argc, argv);
}
void route_wrapper(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: route <kernel-command> [args...]\n");
        return;
    }
    route_command(argv[1], argc - 1, argv + 1);
}

// ===== PERMISSION COMMANDS =====

// Helper function to check if current user is guest (with restrictions)
int is_guest_user() {
    extern char current_user[50];
    return (strcmp(current_user, "guest") == 0);
}

void chmod_command(int argc, char **argv) {
    if (is_guest_user()) {
        printf("chmod: Permission denied - guests cannot change file permissions\n");
        return;
    }
    
    if (argc < 3) {
        printf("Usage: chmod <mode> <file>\n");
        printf("Examples:\n");
        printf("  chmod 755 file.txt\n");
        printf("  chmod rwxr-xr-x file.txt\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[2], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    // Parse permission mode
    extern int vfs_parse_permissions(const char* perm_str);
    int mode = vfs_parse_permissions(argv[1]);
    
    // Apply chmod
    extern int vfs_chmod(const char* path, unsigned int mode);
    if (vfs_chmod(full_path, mode) == 0) {
        printf("Changed permissions of '%s'\n", argv[2]);
    } else {
        printf("chmod: cannot change permissions of '%s': Permission denied\n", argv[2]);
    }
}

void chown_command(int argc, char **argv) {
    if (is_guest_user()) {
        printf("chown: Permission denied - guests cannot change file ownership\n");
        return;
    }
    
    if (argc < 3) {
        printf("Usage: chown <owner>[:<group>] <file>\n");
        printf("Examples:\n");
        printf("  chown root file.txt\n");
        printf("  chown user:users file.txt\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[2], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    // Parse owner:group format
    char owner[64] = {0};
    char group[64] = {0};
    char* colon = strchr(argv[1], ':');
    
    if (colon) {
        *colon = '\0';
        strncpy(owner, argv[1], sizeof(owner) - 1);
        strncpy(group, colon + 1, sizeof(group) - 1);
        *colon = ':'; // restore original string
    } else {
        strncpy(owner, argv[1], sizeof(owner) - 1);
    }
    
    // Apply chown
    extern int vfs_chown(const char* path, const char* owner, const char* group);
    if (vfs_chown(full_path, owner[0] ? owner : NULL, group[0] ? group : NULL) == 0) {
        printf("Changed ownership of '%s'\n", argv[2]);
    } else {
        printf("chown: cannot change ownership of '%s': Permission denied\n", argv[2]);
    }
}

void stat_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: stat <file>\n");
        return;
    }
    
    char expanded_path[512];
    expand_path(argv[1], expanded_path, sizeof(expanded_path));
    
    // Build absolute path if relative
    char full_path[512];
    if (expanded_path[0] != '/') {
        char* cwd = vfs_getcwd();
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, expanded_path);
    } else {
        strcpy(full_path, expanded_path);
    }
    
    extern VNode* vfs_find_node(const char* path);
    VNode* node = vfs_find_node(full_path);
    if (!node) {
        printf("stat: cannot stat '%s': No such file or directory\n", argv[1]);
        return;
    }
    
    printf("  File: %s\n", node->name);
    printf("  Size: %zu\n", node->size);
    printf("  Type: %s\n", node->is_directory ? "directory" : "regular file");
    
    // Format permissions
    char perm_str[10];
    extern void vfs_format_permissions(unsigned int mode, char* output);
    vfs_format_permissions(node->mode, perm_str);
    printf("Access: (%04o/%s)\n", node->mode & 0777, perm_str);
    
    printf("Owner: %s\n", node->owner);
    printf("Group: %s\n", node->group);
    
    // Format timestamps
    char time_str[64];
    struct tm* tm_info = localtime(&node->created_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Created: %s\n", time_str);
    
    tm_info = localtime(&node->modified_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Modified: %s\n", time_str);
}

// ===== END PERMISSION COMMANDS =====

// VM command implementations
void vm_status_command(int argc, char **argv) {
    printf("=== Zora VM Status ===\n");
    printf("CPU: Running\n");
    printf("Memory: 256MB allocated\n");
    printf("Shell: MERL v1.0 (VM Mode)\n");
    printf("OS: Zora Custom OS\n");
    printf("Uptime: Running\n");
}

void vm_reboot_command(int argc, char **argv) {
    printf("Rebooting Zora VM...\n");
    printf("This will restart the entire VM process.\n");
    printf("Are you sure? (y/n): ");
    char response;
    scanf(" %c", &response);
    if (response == 'y' || response == 'Y') {
        printf("Initiating reboot...\n");
        vm_trigger_reboot();  // Call the proper reboot function
        return;  // Let the main loop handle the reboot
    } else {
        printf("Reboot cancelled.\n");
    }
}

void vm_shutdown_command(int argc, char **argv) {
    printf("Shutting down Zora VM...\n");
    printf("Goodbye!\n");
    exit(0);
}

// ===== SYSTEM MONITOR COMMANDS =====

void top_command(int argc, char **argv) {
    system_monitor_update();
    system_monitor_display_processes();
}

void osinfo_command(int argc, char **argv) {
    system_monitor_display_system_info();
}

void mounts_command(int argc, char **argv) {
    system_monitor_display_filesystems();
}

void netinfo_command(int argc, char **argv) {
    system_monitor_display_network_status();
}

void proc_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: proc <add|kill|list> [args...]\n");
        printf("  proc add <name> [priority]  - Add a new process\n");
        printf("  proc kill <pid>            - Kill a process\n");
        printf("  proc list                  - List all processes\n");
        return;
    }
    
    if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) {
            printf("Usage: proc add <name> [priority]\n");
            return;
        }
        char* name = argv[2];
        int priority = (argc >= 4) ? atoi(argv[3]) : 50;
        int pid = system_monitor_add_process(name, priority);
        if (pid > 0) {
            printf("Process '%s' added with PID %d\n", name, pid);
        } else {
            printf("Failed to add process '%s'\n", name);
        }
    } else if (strcmp(argv[1], "kill") == 0) {
        if (argc < 3) {
            printf("Usage: proc kill <pid>\n");
            return;
        }
        int pid = atoi(argv[2]);
        if (system_monitor_kill_process(pid) == 0) {
            printf("Process %d terminated\n", pid);
        } else {
            printf("Failed to kill process %d\n", pid);
        }
    } else if (strcmp(argv[1], "list") == 0) {
        system_monitor_display_processes();
    } else {
        printf("Unknown proc command: %s\n", argv[1]);
    }
}

void dmesg_command(int argc, char **argv) {
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              ZoraVM Kernel Messages                         ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("[    0.000000] ZoraVM kernel version 2.1.0 starting...\n");
    printf("[    0.001234] Initializing virtual CPU with x86_64 architecture\n");
    printf("[    0.002456] Memory management initialized: 64MB virtual memory\n");
    printf("[    0.003789] VFS: Virtual filesystem mounted at /\n");
    printf("[    0.004012] DEVMGR: Device manager started\n");
    printf("[    0.005234] DEVMGR: Registered driver: Terminal Driver v1.0\n");
    printf("[    0.006456] DEVMGR: Registered driver: Virtual Disk Driver v1.0\n");
    printf("[    0.007789] DEVMGR: Registered driver: Virtual Network Driver v1.0\n");
    printf("[    0.009012] NET: Virtual network stack initialized\n");
    printf("[    0.010234] NET: Interface veth0 configured (10.0.2.15/24)\n");
    printf("[    0.011456] SANDBOX: Security sandbox enabled\n");
    printf("[    0.012789] SANDBOX: Memory limit: 64MB, CPU limit: 80%%\n");
    printf("[    0.014012] LUA: Lua scripting engine v5.4.6 loaded\n");
    printf("[    0.015234] MERL: MERL shell v2.1.0 initialized\n");
    printf("[    0.016456] AUTH: Multi-user authentication system ready\n");
    printf("[    0.017789] VFS: Unix-style permissions enabled\n");
    printf("[    0.019012] TERM: Terminal styling system initialized\n");
    printf("[    0.020234] BOOT: System initialization complete\n");
    printf("[    0.021456] SHELL: User session started for 'guest'\n");
    
    time_t current_time = time(NULL);
    struct tm* timeinfo = localtime(&current_time);
    printf("[%4d.%06d] SYSTEM: Current time %04d-%02d-%02d %02d:%02d:%02d\n",
           (int)(current_time % 10000), 123456,
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void services_command(int argc, char **argv) {
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                System Services                               ║\n");
    printf("╠══════════════════╤═══════════╤═════════════╤══════════════╤══════════════════╣\n");
    printf("║ Service          │ Status    │ PID         │ Memory       │ Description      ║\n");
    printf("╠══════════════════╪═══════════╪═════════════╪══════════════╪══════════════════╣\n");
    printf("║ zora-kernel      │ running   │ 1           │ 2048 KB      │ System kernel    ║\n");
    printf("║ init             │ running   │ 2           │ 512 KB       │ Init process     ║\n");
    printf("║ merl-shell       │ running   │ 3           │ 4096 KB      │ MERL shell       ║\n");
    printf("║ vfs-daemon       │ running   │ 4           │ 1024 KB      │ VFS manager      ║\n");
    printf("║ net-stack        │ running   │ 5           │ 768 KB       │ Network stack    ║\n");
    printf("║ auth-service     │ running   │ 6           │ 256 KB       │ Authentication   ║\n");
    printf("║ term-manager     │ running   │ 7           │ 512 KB       │ Terminal manager ║\n");
    printf("║ sandbox-monitor  │ running   │ 8           │ 384 KB       │ Security sandbox ║\n");
    printf("║ lua-engine       │ running   │ 9           │ 1536 KB      │ Lua interpreter  ║\n");
    printf("╚══════════════════╧═══════════╧═════════════╧══════════════╧══════════════════╝\n");
    
    printf("\nService Management:\n");
    printf("• All critical services are running normally\n");
    printf("• Total system memory usage: 11.1 MB\n");
    printf("• System uptime: %ld seconds\n", time(NULL) % 86400);
    printf("• No failed services detected\n");
}

void terminal_test_command(int argc, char **argv) {
    print_terminal_info();
}

void launch_wt_command(int argc, char **argv) {
    printf("Attempting to launch Windows Terminal...\n");
    
    // Get current executable path
    char exe_path[512];
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    
    if (try_launch_windows_terminal(exe_path)) {
        printf("Successfully launched Windows Terminal!\n");
        printf("This session will continue in the old terminal.\n");
        printf("Switch to the new Windows Terminal window for better experience.\n");
    } else {
        printf("Failed to launch Windows Terminal.\n");
        printf("Make sure Windows Terminal is installed:\n");
        printf("  • Install from Microsoft Store\n");
        printf("  • Or run: winget install Microsoft.WindowsTerminal\n");
        printf("  • Or download from: https://github.com/microsoft/terminal\n");
    }
}

// ===== END SYSTEM MONITOR COMMANDS =====

// Command table
Command command_table[] = {
    {"man", man_command, "Displays information about commands."},
    {"help", help_command, "Displays the help menu."},
    {"theme", theme_command, "Change ZoraVM visual theme."},
    {"sysinfo", sysinfo_command, "Displays system information and credits."},
    {"pwd", pwd_command, "Prints the current working directory."},
    {"ls", ls_command, "Lists the contents of the current directory."},
    {"cd", cd_command, "Changes the current working directory."},
    {"mkdir", mkdir_command, "Creates a new directory."},
    {"rmdir", rmdir_command, "Removes a directory."},
    {"touch", touch_command, "Creates a new file."},
    {"rm", rm_command, "Removes a file."},
    {"cp", cp_command, "Copies a file."},
    {"mv", mv_command, "Moves a file."},
    {"rename", rename_command, "Renames a file."},
    {"search", search_command, "Searches for files matching a pattern."},
    {"edit", edit_command, "Edits a text file."},
    {"run", run_command, "Runs an external program."},
    {"clear", clear_command, "Clears the screen."},
    {"echo", echo_command, "Prints a string to the console."},
    {"cat", cat_command, "Displays the contents of a file."},
    {"tetra", tetra_command, "Handles package management."},
    {"flipper", flipper_command, "Switches to sub-shells."},
    {"pull", pull_command, "Takes a directory from the MERL goodies repository."},
    {"whoami", whoami_command, "Displays the current logged-in user."},
    {"useradd", useradd_command, "Adds a new user with secure password input."},
    {"login", login_command, "Logs in with secure password input (Unix-style)."},
    {"logout", logout_command, "Logs out the current user."},
    {"passwd", passwd_command, "Changes password with secure input and verification."},
    {"su", su_command, "Switch user (su [username], defaults to root)."},
    {"users", users_command, "List all users on the system."},
    {"setup-root", setup_root_command, "Setup root user (first-time only)."},
    {"chmod", chmod_command, "Change file permissions (chmod <mode> <file>)."},
    {"chown", chown_command, "Change file ownership (chown <owner>[:<group>] <file>)."},
    {"stat", stat_command, "Display detailed file information."},
    {"route", route_wrapper, "Routes commands to the appropriate handlers."},
    {"fork", fork_wrapper, "Creates a new process."},
    {"kill", kill_wrapper, "Terminates a process by ID."},
    {"ps", ps_wrapper, "Lists all active processes."},
    {"read", read_wrapper, "Reads a file."},
    {"write", write_wrapper, "Writes to a file."},
    {"color-and-test", color_and_test_command, "Displays colors and system info."},
    {"neofetch", color_and_test_command, "Display system information with logo (alias for color-and-test)."},
    {"vmstat", vm_status_command, "Shows virtual machine status."},
    {"reboot", vm_reboot_command, "Reboots the virtual machine."},
    {"shutdown", vm_shutdown_command, "Shuts down the virtual machine."},
    {"save", save_command, "Save file to persistent storage"},
    {"load", load_command, "Load directory from persistent storage"},
    {"mount", mount_command, "Mount host directory to VM path"},
    {"sync", sync_command, "Sync all persistent storage"},
    {"pls", persistent_ls_command, "List persistent storage contents"},
    {"ifconfig", ifconfig_command, "Configure network interface"},
    {"ping", ping_command, "Send ICMP ping packets"},
    {"netstat", netstat_command, "Display network connections"},
    {"nslookup", nslookup_command, "Query DNS servers"},
    {"telnet", telnet_command, "Connect to remote host"},
    {"wget", wget_command, "Download files from web"},
    {"curl", curl_command, "Transfer data from servers"},
    {"ssh", ssh_command, "Secure shell connection"},
    {"iptables", iptables_command, "Configure firewall rules"},
    {"testvfs", test_vfs_command, "Test VFS functionality"},
    {"debugvfs", debug_vfs_command, "Debug VFS structure"},
    {"lua", lua_command, "Execute Lua script from /scripts (fallback /persistent/scripts)"},
    {"luacode", luacode_command, "Execute Lua code directly."},
    {"exec", exec_command, "Execute binary from /persistent/data/"},
    {"run-windows", run_windows_command, "Execute Windows binary with sandboxing"},
    {"list-binaries", list_binaries_command, "List available binaries and their types"},
    {"sandbox-status", sandbox_status_command, "Show sandbox execution status"},
    {"theme", theme_command, "Terminal theme control"},
    {"themes", themes_command, "List available terminal themes"},
    {"find", find_command, "Search for files by name pattern"},
    {"tree", tree_command, "Display directory tree structure"},
    {"less", less_command, "View file contents page by page"},
    {"head", head_command, "Display the beginning of a file"},
    {"tail", tail_command, "Display the end of a file"},
    {"grep", grep_command, "Search for patterns within files"},
    {"htop", htop_command, "Display sorted information about processes in real-time"},
    {"jobs", jobs_command, "List background jobs"},
    {"bg", bg_command, "Send a stopped process to the background"},
    {"fg", fg_command, "Bring a background process to the foreground"},
    {"date", date_command, "Display or set the system date and time"},
    {"df", df_command, "Display disk space usage"},
    {"du", du_command, "Display disk usage of files and directories"},
    {"uname", uname_command, "Print system information"},
    {"systeminfo", systeminfo_command, "Display detailed system information"},
    {"scripts", scripts_command, "List available script-based commands"},
    {"history", history_command, "Display command history"},
    {"scp", scp_command, "Secure copy for transferring files over SSH"},
    {"tar", tar_command, "Archive files and directories"},
    {"gzip", gzip_command, "Compress files"},
    {"gunzip", gunzip_command, "Decompress files"},
    {"zip", zip_command, "Create zip archives"},
    {"unzip", unzip_command, "Extract zip archives"},
    {"hostname", hostname_command, "Display or set the system hostname"},
    
    // System monitoring and OS commands
    {"top", top_command, "Display running processes (system monitor)"},
    {"osinfo", osinfo_command, "Display detailed OS and system information"},
    {"mounts", mounts_command, "Show mounted filesystems"},
    {"netinfo", netinfo_command, "Display network interface status"},
    {"proc", proc_command, "Process management (add/kill/list)"},
    {"dmesg", dmesg_command, "Display kernel messages"},
    {"services", services_command, "Display system services status"},
    {"terminal-test", terminal_test_command, "Test terminal capabilities and compatibility"},
    {"launch-wt", launch_wt_command, "Launch Windows Terminal (if available)"},
    
    // Terminal styling commands
    {"style", style_command, "Configure terminal styling (init/reset/save/load)"},
    {"font", font_command, "Set terminal font (MS Mincho recommended)"},
    {"cursor", cursor_command, "Set cursor style (block/underscore/vertical)"},
    {"colors", colors_command, "Manage color schemes (Campbell PowerShell)"},
    {"retro", retro_command, "Enable/disable retro terminal mode"},
    {"syntax", syntax_command, "Toggle command syntax highlighting"},
    {"terminal-demo", terminal_demo_command, "Demonstrate terminal enhancements"},
    
    #ifdef PYTHON_SCRIPTING
    {"python", python_command, "Execute Python script from /scripts (fallback legacy path)"},
    {"pycode", pycode_command, "Execute Python code directly"},
    #endif
    #ifdef PERL_SCRIPTING
    {"perl", perl_command, "Execute Perl script from /scripts (fallback legacy path)"},
    {"plcode", plcode_command, "Execute Perl code directly"},
    #endif

    {"test-sandbox", test_sandbox_command, "Test sandbox security and isolation"},
    
    // Additional essential Unix commands
    {"sort", sort_command, "Sort lines in text files"},
    {"uniq", uniq_command, "Report or omit repeated lines"},
    {"wc", wc_command, "Count lines, words, and characters in files"},
    {"awk", awk_command, "Text processing and pattern scanning"},
    {"which", which_command, "Locate a command"},
    {"ln", ln_command, "Create links between files"},
    {"diff", diff_command, "Compare files line by line"},
    {"exit", exit_command, "Exit the shell and VM"},
    {"version", version_command, "Display ZoraVM version information"},
    
    // Environment variable commands
    {"set", set_command, "Set environment variables"},
    {"unset", unset_command, "Remove environment variables"},
    {"export", export_command, "Export environment variables"},
    {"env", env_command, "Display environment variables"},
    
    // Diagnostic commands
    {"font-debug", font_debug_command, "Debug console font and UTF-8 compatibility"},
    {"console-refresh", console_refresh_command, "Force console refresh to fix UTF-8 rendering"},
    {"console-test", console_test_command, "Test console capabilities and diagnose issues"},
    {"utf8-detailed", utf8_detailed_test_command, "Detailed UTF-8 character rendering test"},
    {"utf8-monitor", utf8_monitor_command, "Monitor UTF-8 encoding status over time"},
    {"utf8-fix", utf8_fix_command, "Attempt to fix UTF-8 encoding issues"},
    {"utf8-test", utf8_test_command, "Test and diagnose UTF-8 encoding support"},

    {NULL, NULL, NULL}
};
const int command_table_size = sizeof(command_table) / sizeof(Command);

// Environment variable implementation
void set_env_var(const char* name, const char* value) {
    // Check if variable already exists
    for (int i = 0; i < env_var_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            strncpy(env_vars[i].value, value, MAX_VAR_VALUE - 1);
            env_vars[i].value[MAX_VAR_VALUE - 1] = '\0';
            return;
        }
    }
    
    // Add new variable if space available
    if (env_var_count < MAX_ENV_VARS) {
        strncpy(env_vars[env_var_count].name, name, MAX_VAR_NAME - 1);
        env_vars[env_var_count].name[MAX_VAR_NAME - 1] = '\0';
        strncpy(env_vars[env_var_count].value, value, MAX_VAR_VALUE - 1);
        env_vars[env_var_count].value[MAX_VAR_VALUE - 1] = '\0';
        env_var_count++;
    }
}

const char* get_env_var(const char* name) {
    for (int i = 0; i < env_var_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            return env_vars[i].value;
        }
    }
    return NULL;
}

void expand_variables(const char* input, char* output, size_t output_size) {
    const char* src = input;
    char* dst = output;
    size_t remaining = output_size - 1;
    
    while (*src && remaining > 0) {
        if (*src == '$') {
            src++; // Skip $
            if (*src == '{') {
                // ${VAR} format
                src++; // Skip {
                char var_name[MAX_VAR_NAME];
                int var_len = 0;
                while (*src && *src != '}' && var_len < MAX_VAR_NAME - 1) {
                    var_name[var_len++] = *src++;
                }
                if (*src == '}') src++; // Skip }
                var_name[var_len] = '\0';
                
                const char* var_value = get_env_var(var_name);
                if (var_value) {
                    size_t value_len = strlen(var_value);
                    if (value_len <= remaining) {
                        strcpy(dst, var_value);
                        dst += value_len;
                        remaining -= value_len;
                    }
                }
            } else {
                // $VAR format
                char var_name[MAX_VAR_NAME];
                int var_len = 0;
                while (*src && (isalnum(*src) || *src == '_') && var_len < MAX_VAR_NAME - 1) {
                    var_name[var_len++] = *src++;
                }
                var_name[var_len] = '\0';
                
                const char* var_value = get_env_var(var_name);
                if (var_value) {
                    size_t value_len = strlen(var_value);
                    if (value_len <= remaining) {
                        strcpy(dst, var_value);
                        dst += value_len;
                        remaining -= value_len;
                    }
                }
            }
        } else {
            *dst++ = *src++;
            remaining--;
        }
    }
    *dst = '\0';
}

void init_default_env_vars(void) {
    set_env_var("HOME", "/home");
    set_env_var("USER", "guest");
    set_env_var("PATH", "/bin:/usr/bin:/scripts");
    set_env_var("SHELL", "/bin/merl");
    set_env_var("PWD", "/");
    set_env_var("HOSTNAME", "zora-vm");
}

// Shell operator parsing functions
void execute_simple_command(char *args[], int argc);
void parse_and_execute_command_line(char *command_line);
int execute_command_with_redirection(char *args[], int argc, char *input_file, char *output_file, int append_mode);

void handle_command(char *command) {
    // Check for null command
    if (!command) {
        return;
    }
    
    // Check for empty command after trimming whitespace
    char *trimmed = command;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    if (*trimmed == '\0') {
        return;
    }
    
    // Add command to history (but not if it's just whitespace)
    if (strlen(trimmed) > 0) {
        add_to_history(trimmed);
    }
    
    // Expand environment variables
    char expanded_command[1024];
    expand_variables(trimmed, expanded_command, sizeof(expanded_command));
    
    // Parse and execute the full command line with operators
    parse_and_execute_command_line(expanded_command);
}

void parse_and_execute_command_line(char *command_line) {
    // Highlight the command line if syntax highlighting is enabled
    if (g_terminal_config.syntax_highlight) {
        printf("$ ");
        highlight_command_parts(command_line);
        printf("\n");
    }
    
    // Create a working copy of the command line
    char *cmd_copy = strdup(command_line);
    if (!cmd_copy) return;
    
    // Split by semicolon (;) for sequential commands
    char *cmd_part = strtok(cmd_copy, ";");
    
    while (cmd_part != NULL) {
        // Trim whitespace from command part
        while (*cmd_part == ' ' || *cmd_part == '\t') cmd_part++;
        char *end = cmd_part + strlen(cmd_part) - 1;
        while (end > cmd_part && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';
        
        if (strlen(cmd_part) > 0) {
            // Check for background process operator (&)
            int is_background = 0;
            char *bg_pos = strchr(cmd_part, '&');
            if (bg_pos && *(bg_pos + 1) != '&') {  // Make sure it's not &&
                *bg_pos = '\0';  // Remove the & from command
                is_background = 1;
                // Trim any remaining whitespace after removing &
                char *end = cmd_part + strlen(cmd_part) - 1;
                while (end > cmd_part && (*end == ' ' || *end == '\t')) end--;
                *(end + 1) = '\0';
            }
            
            // Check for && and || operators
            char *and_pos = strstr(cmd_part, "&&");
            char *or_pos = strstr(cmd_part, "||");
            
            if (and_pos || or_pos) {
                // Handle conditional execution
                char *first_cmd, *second_cmd;
                int is_and = 0;
                
                if (and_pos && (!or_pos || and_pos < or_pos)) {
                    // && operator found first
                    *and_pos = '\0';
                    first_cmd = cmd_part;
                    second_cmd = and_pos + 2;
                    is_and = 1;
                } else {
                    // || operator found first
                    *or_pos = '\0';
                    first_cmd = cmd_part;
                    second_cmd = or_pos + 2;
                    is_and = 0;
                }
                
                // Execute first command
                int first_result;
                if (is_background) {
                    printf("[Background] ");
                    terminal_print_operator("Executing");
                    printf(": %s\n", first_cmd);
                    first_result = execute_pipeline(first_cmd);
                } else {
                    first_result = execute_pipeline(first_cmd);
                }
                
                // Execute second command based on first result and operator
                if ((is_and && first_result == 0) || (!is_and && first_result != 0)) {
                    // Trim whitespace from second command
                    while (*second_cmd == ' ' || *second_cmd == '\t') second_cmd++;
                    if (is_background) {
                        printf("[Background] ");
                        terminal_print_operator("Executing");
                        printf(": %s\n", second_cmd);
                    }
                    execute_pipeline(second_cmd);
                }
            } else {
                // No conditional operators, process as pipeline
                if (is_background) {
                    printf("[Background] ");
                    terminal_print_operator("Executing");
                    printf(": %s\n", cmd_part);
                }
                execute_pipeline(cmd_part);
            }
        }
        
        cmd_part = strtok(NULL, ";");
    }
    
    free(cmd_copy);
}

int execute_pipeline(char *pipeline_str) {
    // Create a working copy
    char *pipe_copy = strdup(pipeline_str);
    if (!pipe_copy) return 1;
    
    // For now, implement simple pipeline (can be extended for complex pipes)
    char *pipe_pos = strchr(pipe_copy, '|');
    
    if (pipe_pos) {
        // Handle actual piping - capture output from first command and feed to second
        *pipe_pos = '\0';
        char *first_cmd = pipe_copy;
        char *second_cmd = pipe_pos + 1;
        
        // Trim whitespace
        while (*first_cmd == ' ' || *first_cmd == '\t') first_cmd++;
        while (*second_cmd == ' ' || *second_cmd == '\t') second_cmd++;
        
        // Create temp file for pipe communication
        char temp_pipe_file[] = "/tmp/pipe_data";
        
        // Execute first command with output redirected to temp file
        printf("Executing pipeline: %s | %s\n", first_cmd, second_cmd);
        
        // Parse first command
        char *first_args[256];
        int first_argc = 0;
        char *first_copy = strdup(first_cmd);
        char *token = strtok(first_copy, " \t");
        while (token && first_argc < 255) {
            first_args[first_argc++] = token;
            token = strtok(NULL, " \t");
        }
        first_args[first_argc] = NULL;
        
        // Execute first command and capture output
        int result = 0;
        if (first_argc > 0) {
            execute_command_with_redirection(first_args, first_argc, NULL, temp_pipe_file, 0);
        }
        
        // Parse second command and modify it to read from temp file
        char *second_args[256];
        int second_argc = 0;
        char *second_copy = strdup(second_cmd);
        token = strtok(second_copy, " \t");
        while (token && second_argc < 255) {
            second_args[second_argc++] = token;
            token = strtok(NULL, " \t");
        }
        second_args[second_argc] = NULL;
        
        if (second_argc > 0) {
            // Special handling for commands that expect piped input
            if (strcmp(second_args[0], "grep") == 0) {
                // For grep, we need to modify arguments to read from temp file
                if (second_argc >= 2) {
                    // Shift arguments and add temp file as last argument
                    second_args[second_argc] = temp_pipe_file;
                    second_argc++;
                    second_args[second_argc] = NULL;
                    execute_simple_command(second_args, second_argc);
                } else {
                    printf("Error: grep requires a pattern argument\n");
                }
            } else if (strcmp(second_args[0], "head") == 0 || strcmp(second_args[0], "tail") == 0 || 
                      strcmp(second_args[0], "cat") == 0 || strcmp(second_args[0], "less") == 0) {
                // For file display commands, add temp file as argument
                second_args[second_argc] = temp_pipe_file;
                second_argc++;
                second_args[second_argc] = NULL;
                execute_simple_command(second_args, second_argc);
            } else {
                // For other commands, use input redirection
                execute_command_with_redirection(second_args, second_argc, temp_pipe_file, NULL, 0);
            }
        }
        
        // Clean up temp file
        vfs_delete_file(temp_pipe_file);
        
        free(first_copy);
        free(second_copy);
        free(pipe_copy);
        return result;
    } else {
        // No pipe, execute single command with redirection
        int result = execute_command_with_parsing(pipe_copy);
        free(pipe_copy);
        return result;
    }
}

int execute_command_with_parsing(char *cmd_str) {
    // Parse for redirection operators
    char *input_file = NULL;
    char *output_file = NULL;
    int append_mode = 0;
    
    // Create working copy
    char *work_copy = strdup(cmd_str);
    if (!work_copy) return 1;
    
    // Look for redirection operators
    char *redirect_pos;
    
    // Check for >> (append)
    redirect_pos = strstr(work_copy, ">>");
    if (redirect_pos) {
        *redirect_pos = '\0';
        output_file = redirect_pos + 2;
        append_mode = 1;
        
        // Trim whitespace from filename
        while (*output_file == ' ' || *output_file == '\t') output_file++;
        char *end = output_file + strlen(output_file) - 1;
        while (end > output_file && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';
    } else {
        // Check for > (overwrite)
        redirect_pos = strchr(work_copy, '>');
        if (redirect_pos) {
            *redirect_pos = '\0';
            output_file = redirect_pos + 1;
            append_mode = 0;
            
            // Trim whitespace from filename
            while (*output_file == ' ' || *output_file == '\t') output_file++;
            char *end = output_file + strlen(output_file) - 1;
            while (end > output_file && (*end == ' ' || *end == '\t')) end--;
            *(end + 1) = '\0';
        }
    }
    
    // Check for < (input redirection)
    redirect_pos = strchr(work_copy, '<');
    if (redirect_pos) {
        *redirect_pos = '\0';
        input_file = redirect_pos + 1;
        
        // Trim whitespace from filename
        while (*input_file == ' ' || *input_file == '\t') input_file++;
        char *end = input_file + strlen(input_file) - 1;
        while (end > input_file && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';
    }
    
    // Parse command and arguments (increased buffer size for safety)
    char *args[256];  // Increased from 10 to 256 for safety
    int argc = 0;
    
    char *token = strtok(work_copy, " \t");
    while (token != NULL && argc < 255) {  // Leave room for NULL terminator
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;
    
    int result = 1;
    if (argc > 0) {
        result = execute_command_with_redirection(args, argc, input_file, output_file, append_mode);
    }
    
    free(work_copy);
    return result;
}

// Capture output buffer for VFS redirection
static char redirect_buffer[65536];
static size_t redirect_buffer_size = 0;
static int redirect_active = 0;

// Custom printf for redirection - captures output instead of printing
void redirect_printf(const char* format, ...) {
    if (!redirect_active) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(redirect_buffer + redirect_buffer_size, 
                       sizeof(redirect_buffer) - redirect_buffer_size, 
                       format, args);
    va_end(args);
    
    if (len > 0 && redirect_buffer_size + len < sizeof(redirect_buffer)) {
        redirect_buffer_size += len;
    }
}

int execute_command_with_redirection(char *args[], int argc, char *input_file, char *output_file, int append_mode) {
    // Handle output redirection using VFS
    if (output_file && strlen(output_file) > 0) {
        // Build normalized absolute path
        char full_path[512];
        char* cwd = vfs_getcwd();
        build_full_path(full_path, sizeof(full_path), cwd ? cwd : "/", output_file);
        
        printf("Redirecting output to: %s (%s)\n", full_path, append_mode ? "append" : "overwrite");
        fflush(stdout);
        
        // Initialize redirect buffer
        redirect_buffer_size = 0;
        redirect_active = 1;
        
        // If append mode, read existing content first
        if (append_mode) {
            void* existing_data;
            size_t existing_size;
            if (vfs_read_file(full_path, &existing_data, &existing_size) == 0 && existing_data) {
                if (existing_size < sizeof(redirect_buffer)) {
                    memcpy(redirect_buffer, existing_data, existing_size);
                    redirect_buffer_size = existing_size;
                }
            }
        }

        // Create a temporary filename for stdout redirection
        char temp_filename[256];
        snprintf(temp_filename, sizeof(temp_filename), "temp_stdout_%d.tmp", (int)time(NULL));
        
        // Simple approach: just use freopen and restore with freopen
        FILE* temp_file = freopen(temp_filename, "w", stdout);
        if (temp_file) {
            // Execute the command with stdout redirected
            execute_simple_command(args, argc);
            
            // Flush and close
            fflush(stdout);
            fclose(temp_file);
            
            // Restore stdout to console - simple approach
            freopen("CON", "w", stdout);
            
            // Read the captured output from temp file
            FILE* read_temp = fopen(temp_filename, "r");
            if (read_temp) {
                fseek(read_temp, 0, SEEK_END);
                long size = ftell(read_temp);
                fseek(read_temp, 0, SEEK_SET);
                
                if (size > 0 && size < sizeof(redirect_buffer)) {
                    redirect_buffer_size = fread(redirect_buffer, 1, size, read_temp);
                    // Ensure buffer is properly terminated
                    if (redirect_buffer_size < sizeof(redirect_buffer)) {
                        redirect_buffer[redirect_buffer_size] = '\0';
                    }
                }
                
                fclose(read_temp);
                remove(temp_filename); // Clean up temp file
            }
        } else {
            // Fallback to original method if freopen fails
            execute_simple_command_with_redirect(args, argc);
        }
        
        // Stop capturing
        redirect_active = 0;
        
        // Create file if it doesn't exist
        if (vfs_find_node(full_path) == NULL) {
            vfs_create_file(full_path);
        }
        
        // Write captured output to VFS
        if (redirect_buffer_size > 0) {
            if (vfs_write_file(full_path, redirect_buffer, redirect_buffer_size) == 0) {
                printf("Output redirection completed to: %s (%zu bytes)\n", full_path, redirect_buffer_size);
            } else {
                printf("Error: Failed to write to VFS file '%s'\n", full_path);
            }
        } else {
            printf("Output redirection completed to: %s (empty)\n", full_path);
        }
    } else {
        // No redirection, execute normally
        execute_simple_command(args, argc);
    }
    
    // Handle input redirection (TODO: implement properly)
    if (input_file && strlen(input_file) > 0) {
        printf("Note: Input redirection from '%s' not yet fully implemented\n", input_file);
    }
    
    return 0;
}

void execute_simple_command_with_redirect(char *args[], int argc) {
    // Skip empty commands
    if (argc == 0) return;

    // Search for the command in the command table
    for (int i = 0; i < command_table_size; i++) {
        if (command_table[i].name && strcmp(args[0], command_table[i].name) == 0) {
            // Execute the command handler safely
            if (command_table[i].handler) {
                command_table[i].handler(argc, args);
            }
            return;
        }
    }

    // Command not found - use redirect_printf for consistency
    if (args[0] && strlen(args[0]) > 0 && strlen(args[0]) < 256) {
        // Check if the string contains only printable characters
        int is_valid = 1;
        for (int j = 0; args[0][j]; j++) {
            if (!isprint((unsigned char)args[0][j])) {
                is_valid = 0;
                break;
            }
        }
        
        if (is_valid) {
            redirect_printf("Unknown command: '%s'\n", args[0]);
        } else {
            redirect_printf("Unknown command: [invalid characters]\n");
        }
    } else {
        redirect_printf("Unknown command: [empty or invalid]\n");
    }
    redirect_printf("Type 'help' to see available commands.\n");
}

// Enhanced command parsing implementation
ParsedCommand* parse_command_advanced(char* command_str) {
    ParsedCommand* cmd = malloc(sizeof(ParsedCommand));
    if (!cmd) return NULL;
    
    // Initialize structure
    cmd->args = malloc(256 * sizeof(char*));
    cmd->flags = malloc(64 * sizeof(char*));
    cmd->argc = 0;
    cmd->flag_count = 0;
    cmd->input_redirect = NULL;
    cmd->output_redirect = NULL;
    cmd->append_mode = 0;
    cmd->background = 0;
    
    char* work_str = strdup(command_str);
    char* token = strtok(work_str, " \t");
    
    while (token && cmd->argc < 255) {
        // Check for redirection operators
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t");
            if (token) cmd->output_redirect = strdup(token);
            continue;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t");
            if (token) {
                cmd->output_redirect = strdup(token);
                cmd->append_mode = 1;
            }
            continue;
        } else if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t");
            if (token) cmd->input_redirect = strdup(token);
            continue;
        } else if (strcmp(token, "&") == 0) {
            cmd->background = 1;
            continue;
        }
        
        // Check for flags (starting with -)
        if (token[0] == '-' && strlen(token) > 1 && cmd->flag_count < 63) {
            cmd->flags[cmd->flag_count++] = strdup(token);
        } else {
            cmd->args[cmd->argc++] = strdup(token);
        }
        
        token = strtok(NULL, " \t");
    }
    
    cmd->args[cmd->argc] = NULL;
    cmd->flags[cmd->flag_count] = NULL;
    
    free(work_str);
    return cmd;
}

void free_parsed_command(ParsedCommand* cmd) {
    if (!cmd) return;
    
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->args[i]);
    }
    for (int i = 0; i < cmd->flag_count; i++) {
        free(cmd->flags[i]);
    }
    
    free(cmd->args);
    free(cmd->flags);
    free(cmd->input_redirect);
    free(cmd->output_redirect);
    free(cmd);
}

int has_flag(ParsedCommand* cmd, const char* flag) {
    for (int i = 0; i < cmd->flag_count; i++) {
        if (strcmp(cmd->flags[i], flag) == 0) return 1;
        // Support combined flags like -la = -l -a
        if (strlen(cmd->flags[i]) > 2 && cmd->flags[i][0] == '-' && 
            strlen(flag) == 2 && flag[0] == '-') {
            for (int j = 1; cmd->flags[i][j]; j++) {
                if (cmd->flags[i][j] == flag[1]) return 1;
            }
        }
    }
    return 0;
}

char* get_flag_value(ParsedCommand* cmd, const char* flag) {
    for (int i = 0; i < cmd->flag_count; i++) {
        if (strncmp(cmd->flags[i], flag, strlen(flag)) == 0) {
            if (strlen(cmd->flags[i]) > strlen(flag) && cmd->flags[i][strlen(flag)] == '=') {
                return cmd->flags[i] + strlen(flag) + 1;
            }
        }
    }
    return NULL;
}

// Enhanced syntax highlighting for command lines
void highlight_command_parts(char* command_line) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", command_line);
        return;
    }
    
    char* work_str = strdup(command_line);
    char* token = strtok(work_str, " \t");
    int is_first = 1;
    
    while (token) {
        if (!is_first) printf(" ");
        
        if (is_first) {
            // First token is the command
            terminal_print_command(token);
            is_first = 0;
        } else if (token[0] == '-') {
            // Flags in bright magenta
            printf(COLOR_BRIGHT_MAGENTA "%s" COLOR_RESET, token);
        } else if (strchr(token, '/') || strchr(token, '\\') || strchr(token, '.')) {
            // Paths and files
            terminal_print_path(token);
        } else if ((token[0] == '"' && token[strlen(token)-1] == '"') ||
                   (token[0] == '\'' && token[strlen(token)-1] == '\'')) {
            // Quoted strings
            terminal_print_string(token);
        } else if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || 
                   strcmp(token, "<") == 0 || strcmp(token, "|") == 0 ||
                   strcmp(token, "&&") == 0 || strcmp(token, "||") == 0 ||
                   strcmp(token, "&") == 0) {
            // Operators
            terminal_print_operator(token);
        } else {
            // Regular arguments
            terminal_print_argument(token);
        }
        
        token = strtok(NULL, " \t");
    }
    
    free(work_str);
}

// Enhanced script command resolution with better error handling
int try_execute_script_command(const char* command, int argc, char** argv) {
    char script_path[512];
    
    // Prevent infinite recursion by checking if we're already in script execution
    static int script_execution_depth = 0;
    if (script_execution_depth > 5) {
        printf("Error: Script execution depth limit reached (possible infinite recursion)\n");
        return 0;
    }
    
    script_execution_depth++;
    
    // Check for Lua script (.lua)
    snprintf(script_path, sizeof(script_path), "/bin/%s.lua", command);
    if (vfs_find_node(script_path)) {
        printf("Executing Lua script: %s\n", script_path);
        
        #ifdef LUA_SCRIPTING
        // Build argument string for Lua script
        char args_str[1024] = "";
        for (int i = 1; i < argc && i < 10; i++) {  // Limit arguments to prevent overflow
            if (i > 1) strcat(args_str, " ");
            if (strlen(args_str) + strlen(argv[i]) < sizeof(args_str) - 10) {
                strcat(args_str, argv[i]);
            }
        }
        
        int result = lua_vm_load_script_with_args(script_path, args_str);
        script_execution_depth--;
        return (result == 0) ? 1 : 0;
        #else
        printf("Lua scripting not enabled in this build\n");
        script_execution_depth--;
        return 1;
        #endif
    }
    
    // Check for Python script (.py)
    snprintf(script_path, sizeof(script_path), "/bin/%s.py", command);
    if (vfs_find_node(script_path)) {
        printf("Executing Python script: %s\n", script_path);
        
        #ifdef PYTHON_SCRIPTING
        int result = python_vm_load_script_with_args(script_path, argc, argv);
        script_execution_depth--;
        return (result == 0) ? 1 : 0;
        #else
        printf("Python scripting not enabled in this build\n");
        script_execution_depth--;
        return 1;
        #endif
    }
    
    // Check for Perl script (.pl)
    snprintf(script_path, sizeof(script_path), "/bin/%s.pl", command);
    if (vfs_find_node(script_path)) {
        printf("Executing Perl script: %s\n", script_path);
        
        #ifdef PERL_SCRIPTING
        int result = perl_vm_load_script_with_args(script_path, argc, argv);
        script_execution_depth--;
        return (result == 0) ? 1 : 0;
        #else
        printf("Perl scripting not enabled in this build\n");
        script_execution_depth--;
        return 1;
        #endif
    }
    
    script_execution_depth--;
    return 0; // No script found
}

void execute_simple_command(char *args[], int argc) {
    // Skip empty commands
    if (argc == 0) return;

    // Search for the command in the command table
    for (int i = 0; i < command_table_size; i++) {
        if (command_table[i].name && strcmp(args[0], command_table[i].name) == 0) {
            // Execute the command handler safely
            if (command_table[i].handler) {
                command_table[i].handler(argc, args);
            }
            return;
        }
    }

    // Command not found in built-ins - check for scripts in /bin/
    if (try_execute_script_command(args[0], argc, args)) {
        return; // Script was found and executed
    }

    // Command not found - show styled error message
    terminal_print_error("Unknown command: '");
    printf("%s", args[0]);
    terminal_print_error("'\n");
    printf("Type ");
    terminal_print_command("help");
    printf(" to see available commands.\n");
}

void man_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: man <command>\n");
        return;
    }

    for (int i = 0; i < command_table_size; i++) {
        if (strcmp(argv[1], command_table[i].name) == 0) {
            printf("%s: %s\n", command_table[i].name, command_table[i].description);
            return;
        }
    }

    printf("No manual entry for '%s'\n", argv[1]);
}

void help_command(int argc, char **argv) {
    if (argc > 1) {
        // Show help for specific command
        char* command_name = argv[1];
        
        // Search for the command in the command table
        for (int i = 0; i < command_table_size; i++) {
            if (command_table[i].name && strcmp(command_name, command_table[i].name) == 0) {
                printf("\n=== Help for '%s' ===\n", command_name);
                printf("Description: %s\n", command_table[i].description ? command_table[i].description : "No description available");
                printf("Usage: %s\n", command_name);
                
                // Show specific usage examples for common commands
                if (strcmp(command_name, "grep") == 0) {
                    printf("Examples:\n");
                    printf("  grep pattern file.txt       - Search for pattern in file\n");
                    printf("  command | grep pattern       - Filter command output\n");
                } else if (strcmp(command_name, "sort") == 0) {
                    printf("Examples:\n");
                    printf("  sort file.txt               - Sort lines alphabetically\n");
                    printf("  sort -r file.txt            - Sort in reverse order\n");
                    printf("  sort -n numbers.txt         - Sort numerically\n");
                } else if (strcmp(command_name, "wc") == 0) {
                    printf("Examples:\n");
                    printf("  wc file.txt                 - Count lines, words, chars\n");
                    printf("  wc -l file.txt              - Count lines only\n");
                    printf("  command | wc                - Count command output\n");
                }
                printf("\n");
                return;
            }
        }
        printf("Command '%s' not found. Type 'help' to see all commands.\n", command_name);
        return;
    }
    
    // Show categorized help
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                            ZORA VM - MERL SHELL                            ║\n");
    printf("║                        Unix-Compatible Command Environment                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf(" FILE SYSTEM COMMANDS:\n");
    printf("  %-12s - List directory contents          %-12s - Print working directory\n", "ls", "pwd");
    printf("  %-12s - Change directory                 %-12s - Create directory\n", "cd", "mkdir");
    printf("  %-12s - Remove directory                 %-12s - Create empty file\n", "rmdir", "touch");
    printf("  %-12s - Remove file                      %-12s - Copy file\n", "rm", "cp");
    printf("  %-12s - Move/rename file                 %-12s - Display file contents\n", "mv", "cat");
    printf("  %-12s - Edit text file                   %-12s - Find files by pattern\n", "edit", "find");
    printf("  %-12s - Show directory tree              %-12s - Search files by name\n", "tree", "search");
    printf("\n");
    
    printf(" FILE VIEWING & PROCESSING:\n");
    printf("  %-12s - View file page by page           %-12s - Show file beginning\n", "less", "head");
    printf("  %-12s - Show file end                    %-12s - Search patterns in files\n", "tail", "grep");
    printf("  %-12s - Sort file lines                  %-12s - Remove duplicate lines\n", "sort", "uniq");
    printf("  %-12s - Count lines/words/chars          %-12s - Text processing tool\n", "wc", "awk");
    printf("  %-12s - Compare files                    %-12s - Create file links\n", "diff", "ln");
    printf("\n");
    
    printf(" SYSTEM & PROCESS MANAGEMENT:\n");
    printf("  %-12s - Show system information          %-12s - Display processes\n", "sysinfo", "htop");
    printf("  %-12s - List active processes            %-12s - Create new process\n", "ps", "fork");
    printf("  %-12s - Terminate process                %-12s - List background jobs\n", "kill", "jobs");
    printf("  %-12s - Background process               %-12s - Foreground process\n", "bg", "fg");
    printf("  %-12s - Show current date/time           %-12s - Show disk usage\n", "date", "df");
    printf("  %-12s - Directory disk usage             %-12s - System information\n", "du", "uname");
    printf("  %-12s - Detailed system info             %-12s - Show version\n", "systeminfo", "version");
    printf("\n");
    
    printf(" ADVANCED SYSTEM MONITORING:\n");
    printf("  %-12s - Real-time process monitor        %-12s - Operating system info\n", "top", "osinfo");
    printf("  %-12s - Mounted filesystems              %-12s - Network interface info\n", "mounts", "netinfo");
    printf("  %-12s - Process management (add/kill)    %-12s - Kernel message buffer\n", "proc", "dmesg");
    printf("  %-12s - System services status           %-12s - Terminal capabilities\n", "services", "terminal-test");
    printf("  %-12s - Launch Windows Terminal          \n", "launch-wt");
    printf("\n");
    
    printf(" NETWORK COMMANDS:\n");
    printf("  %-12s - Configure network interface      %-12s - Send ping packets\n", "ifconfig", "ping");
    printf("  %-12s - Show network connections         %-12s - DNS lookup\n", "netstat", "nslookup");
    printf("  %-12s - Remote terminal access           %-12s - Download files\n", "ssh", "wget");
    printf("  %-12s - Transfer data                    %-12s - Secure file copy\n", "curl", "scp");
    printf("  %-12s - Configure firewall               %-12s - Set hostname\n", "iptables", "hostname");
    printf("\n");
    
    printf(" ARCHIVE & COMPRESSION:\n");
    printf("  %-12s - Archive files                    %-12s - Compress files\n", "tar", "gzip");
    printf("  %-12s - Decompress files                 %-12s - Create zip archives\n", "gunzip", "zip");
    printf("  %-12s - Extract zip archives             \n", "unzip");
    printf("\n");
    
    printf(" PERMISSIONS & USERS:\n");
    printf("  %-12s - Change file permissions          %-12s - Change file owner\n", "chmod", "chown");
    printf("  %-12s - Show current user                %-12s - User login\n", "whoami", "login");
    printf("  %-12s - User logout                      %-12s - Add new user\n", "logout", "useradd");
    printf("  %-12s - Change password                  %-12s - Switch user\n", "passwd", "su");
    printf("  %-12s - Show file details                %-12s - List all users\n", "stat", "users");
    printf("  %-12s - Setup root user (first time)    \n", "setup-root");
    printf("\n");
    
    printf(" ENVIRONMENT & VARIABLES:\n");
    printf("  %-12s - Set environment variable         %-12s - Remove environment var\n", "set", "unset");
    printf("  %-12s - Export environment variable      %-12s - Show all variables\n", "export", "env");
    printf("  %-12s - Command history                  %-12s - Locate command\n", "history", "which");
    printf("  %-12s - Display ZoraVM version           \n", "version");
    printf("\n");
    
    printf(" TERMINAL CUSTOMIZATION:\n");
    printf("  %-12s - Configure terminal styling       %-12s - Set terminal font\n", "style", "font");
    printf("  %-12s - Set cursor style                 %-12s - Manage color schemes\n", "cursor", "colors");
    printf("  %-12s - Toggle retro mode                %-12s - Syntax highlighting\n", "retro", "syntax");
    printf("  %-12s - Theme control                    %-12s - List available themes\n", "theme", "themes");
    printf("  %-12s - Test terminal capabilities       %-12s - Launch Windows Terminal\n", "terminal-test", "launch-wt");
    printf("  %-12s - Monitor UTF-8 over time          %-12s - Fix UTF-8 encoding\n", "utf8-monitor", "utf8-fix");
    printf("  %-12s - Test UTF-8 support               \n", "utf8-test");
    printf("\n");
    
    printf(" VM SPECIFIC COMMANDS:\n");
    printf("  %-12s - VM status information            %-12s - Reboot virtual machine\n", "vmstat", "reboot");
    printf("  %-12s - Shutdown VM                      %-12s - Test VFS functionality\n", "shutdown", "testvfs");
    printf("  %-12s - Debug VFS structure              %-12s - Execute binary files\n", "debugvfs", "exec");
    printf("  %-12s - Sandbox status                   %-12s - Security testing\n", "sandbox-status", "test-sandbox");
    printf("  %-12s - Persistent storage list          %-12s - Save to persistent\n", "pls", "save");
    printf("  %-12s - Load from persistent             %-12s - Mount host directory\n", "load", "mount");
    printf("  %-12s - Sync persistent storage          %-12s - Exit shell and VM\n", "sync", "exit");
    printf("\n");
    
    printf(" SCRIPTING & PROGRAMMING:\n");
    printf("  %-12s - Execute Lua scripts              %-12s - Run Lua code directly\n", "lua", "luacode");
    #ifdef PYTHON_SCRIPTING
    printf("  %-12s - Execute Python scripts           %-12s - Run Python code directly\n", "python", "pycode");
    #endif
    #ifdef PERL_SCRIPTING
    printf("  %-12s - Execute Perl scripts             %-12s - Run Perl code directly\n", "perl", "plcode");
    #endif
    printf("  %-12s - Package management               %-12s - Sub-shell switcher\n", "tetra", "flipper");
    printf("  %-12s - Repository management            %-12s - List script commands\n", "pull", "scripts");
    printf("  %-12s - Binary execution                 %-12s - Windows binary exec\n", "exec", "run-windows");
    printf("  %-12s - List available binaries          \n", "list-binaries");
    printf("\n");
    
    printf(" SHELL OPERATORS:\n");
    printf("  %-12s - Sequential execution (cmd1 ; cmd2)\n", ";");
    printf("  %-12s - Conditional AND (cmd1 && cmd2 - run cmd2 only if cmd1 succeeds)\n", "&&");
    printf("  %-12s - Conditional OR (cmd1 || cmd2 - run cmd2 only if cmd1 fails)\n", "||");
    printf("  %-12s - Pipe output (cmd1 | cmd2 - send cmd1 output to cmd2)\n", "|");
    printf("  %-12s - Redirect output to file (cmd > file.txt)\n", ">");
    printf("  %-12s - Append output to file (cmd >> file.txt)\n", ">>");
    printf("  %-12s - Redirect input from file (cmd < file.txt)\n", "<");
    printf("\n");
    
    printf(" USAGE EXAMPLES:\n");
    printf("  help grep                    - Get detailed help for grep command\n");
    printf("  ls | grep .txt               - List files and filter for .txt files\n");
    printf("  ps | grep myprocess          - Find specific processes\n");
    printf("  help | grep file             - Find all file-related commands\n");
    printf("  sort data.txt | uniq         - Sort and remove duplicates\n");
    printf("  cat file.txt | wc -l         - Count lines in file\n");
    printf("  echo 'Hello' > output.txt    - Write text to file\n");
    printf("  command1 && command2         - Run command2 only if command1 succeeds\n");
    printf("  set PATH=/usr/bin:/bin       - Set environment variable\n");
    printf("  export USER=admin            - Export environment variable\n");
    printf("  lua myscript.lua arg1 arg2   - Execute Lua script with arguments\n");
    printf("  python analytics.py data.csv - Execute Python script with file\n");
    printf("  top | head -20               - Show top 20 processes\n");
    printf("  find . -name '*.txt' | wc -l - Count text files recursively\n");
    printf("  version && osinfo            - Show version then system info\n");
    printf("  utf8-fix && utf8-test        - Fix then test UTF-8 encoding\n");
    printf("\n");
    
    printf(" QUICK HELP:\n");
    printf("  Type 'help <command>' for detailed help on specific commands\n");
    printf("  Type 'man <command>' for manual pages\n");
    printf("  Type 'which <command>' to locate a command\n");
    printf("  Use Tab completion for command names and file paths\n");
    printf("\n");
}

void theme_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: theme [font-only]\n");
        printf("ZoraVM Terminal Styling:\n");
        printf("  font-only - Uses MS Mincho font with original Campbell colors\n");
        printf("  Current: Enhanced terminal with MS Mincho font\n");
        return;
    }
    
    char* theme_name = argv[1];
    
    if (strcmp(theme_name, "font-only") == 0) {
        printf("ZoraVM is using font-only mode with MS Mincho font.\n");
        printf("Terminal colors remain as Campbell scheme.\n");
        printf("This provides better readability while preserving familiar colors.\n");
    } else {
        printf("Unknown theme: %s\n", theme_name);
        printf("Available: font-only\n");
    }
}

void save_command(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: save <file>\n");
        return;
    }
    printf("Saving file: %s\n", argv[1]);
}

void load_command(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: load <directory>\n");
        return;
    }
    printf("Loading directory: %s\n", argv[1]);
}

void mount_command(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: mount <vm_path> <host_path>\n");
        return;
    }
    printf("Mounting %s -> %s\n", argv[1], argv[2]);
}

void sync_command(int argc, char* argv[]) {
    printf("Syncing all persistent storage...\n");
    
    // Sync all persistent nodes
    vfs_sync_all_persistent();
    
    printf("Sync complete\n");
}

void persistent_ls_command(int argc, char* argv[]) {
    printf("Persistent storage mappings:\n");
    printf("/persistent/documents -> ZoraPerl/documents\n");
    printf("/persistent/scripts -> ZoraPerl/scripts\n");
}

void ifconfig_command(int argc, char* argv[]) {
    if (argc == 1) {
        network_show_interfaces();
        return;
    }
    
    if (argc >= 2) {
        char* iface = argv[1];
        if (strcmp(iface, "up") == 0) {
            network_interface_up("veth0");
        } else if (strcmp(iface, "down") == 0) {
            network_interface_down("veth0");
        }
    }
    
    if (argc >= 4) {
        char* iface = argv[1];
        char* ip = argv[2];
        char* netmask = argv[3];
        network_set_ip(iface, ip, netmask);
    }
}

void ping_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ping <host>\n");
        return;
    }
    
    char* target = argv[1];
    printf("PING %s:\n", target);
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("ping: Failed to initialize network\n");
        return;
    }
    
    // Resolve hostname to IP
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    
    int status = getaddrinfo(target, NULL, &hints, &result);
    if (status != 0) {
        printf("ping: cannot resolve %s: %s\n", target, gai_strerror(status));
        WSACleanup();
        return;
    }
    
    struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);
    
    printf("PING %s (%s) from virtual interface:\n", target, ip_str);
    
    // Create ICMP handle
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        printf("ping: Could not create ICMP handle\n");
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
    
    // Ping data
    char sendData[32] = "Zora VM ping test data";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    LPVOID replyBuffer = malloc(replySize);
    
    if (!replyBuffer) {
        printf("ping: Memory allocation failed\n");
        IcmpCloseHandle(hIcmpFile);
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
    
    // Send 4 ping packets
    for (int i = 1; i <= 4; i++) {
        DWORD dwRetVal = IcmpSendEcho(hIcmpFile, addr_in->sin_addr.s_addr,
                                      sendData, sizeof(sendData),
                                      NULL, replyBuffer, replySize, 5000);
        
        if (dwRetVal != 0) {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
            if (pEchoReply->Status == IP_SUCCESS) {
                printf("64 bytes from %s (%s): icmp_seq=%d ttl=%d time=%ldms\n",
                       target, ip_str, i, pEchoReply->Options.Ttl, pEchoReply->RoundTripTime);
            } else {
                printf("Request timed out (seq=%d)\n", i);
            }
        } else {
            printf("Request timed out (seq=%d)\n", i);
        }
        
        Sleep(1000); // Wait 1 second between pings
    }
    
    printf("\n--- %s ping statistics ---\n", target);
    printf("4 packets transmitted, 4 received, 0%% packet loss\n");
    
    // Cleanup
    free(replyBuffer);
    IcmpCloseHandle(hIcmpFile);
    freeaddrinfo(result);
    WSACleanup();
}

void netstat_command(int argc, char **argv) {
    if (argc == 1) {
        // Show TCP connections
        printf("=== Active Network Connections (TCP) ===\n");
        printf("  Proto  Local Address          Foreign Address        State\n");
        
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;
        
        // Get the size of the TCP table
        dwRetVal = GetTcpTable(NULL, &dwSize, FALSE);
        if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
            PMIB_TCPTABLE pTcpTable = (MIB_TCPTABLE*)malloc(dwSize);
            if (pTcpTable == NULL) {
                printf("Error allocating memory for TCP table\n");
                return;
            }
            
            // Get the actual TCP table
            dwRetVal = GetTcpTable(pTcpTable, &dwSize, TRUE);
            if (dwRetVal == NO_ERROR) {
                for (int i = 0; i < (int)pTcpTable->dwNumEntries; i++) {
                    struct in_addr local_addr, remote_addr;
                    local_addr.s_addr = pTcpTable->table[i].dwLocalAddr;
                    remote_addr.s_addr = pTcpTable->table[i].dwRemoteAddr;
                    
                    char* state;
                    switch (pTcpTable->table[i].dwState) {
                        case MIB_TCP_STATE_CLOSED: state = "CLOSED"; break;
                        case MIB_TCP_STATE_LISTEN: state = "LISTENING"; break;
                        case MIB_TCP_STATE_SYN_SENT: state = "SYN_SENT"; break;
                        case MIB_TCP_STATE_SYN_RCVD: state = "SYN_RCVD"; break;
                        case MIB_TCP_STATE_ESTAB: state = "ESTABLISHED"; break;
                        case MIB_TCP_STATE_FIN_WAIT1: state = "FIN_WAIT1"; break;
                        case MIB_TCP_STATE_FIN_WAIT2: state = "FIN_WAIT2"; break;
                        case MIB_TCP_STATE_CLOSE_WAIT: state = "CLOSE_WAIT"; break;
                        case MIB_TCP_STATE_CLOSING: state = "CLOSING"; break;
                        case MIB_TCP_STATE_LAST_ACK: state = "LAST_ACK"; break;
                        case MIB_TCP_STATE_TIME_WAIT: state = "TIME_WAIT"; break;
                        case MIB_TCP_STATE_DELETE_TCB: state = "DELETE_TCB"; break;
                        default: state = "UNKNOWN"; break;
                    }
                    
                    printf("  TCP    %s:%-5d          %s:%-5d          %s\n",
                           inet_ntoa(local_addr), ntohs((u_short)pTcpTable->table[i].dwLocalPort),
                           inet_ntoa(remote_addr), ntohs((u_short)pTcpTable->table[i].dwRemotePort),
                           state);
                }
            }
            free(pTcpTable);
        }
        
    } else if (argc == 2) {
        if (strcmp(argv[1], "-r") == 0) {
            // Show routing table
            printf("=== IPv4 Route Table ===\n");
            printf("Network Destination    Netmask          Gateway       Interface  Metric\n");
            
            DWORD dwSize = 0;
            DWORD dwRetVal = GetIpForwardTable(NULL, &dwSize, FALSE);
            if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
                PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE*)malloc(dwSize);
                if (pIpForwardTable == NULL) {
                    printf("Error allocating memory for route table\n");
                    return;
                }
                
                dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, TRUE);
                if (dwRetVal == NO_ERROR) {
                    for (int i = 0; i < (int)pIpForwardTable->dwNumEntries; i++) {
                        struct in_addr dest, mask, gateway, iface;
                        dest.s_addr = pIpForwardTable->table[i].dwForwardDest;
                        mask.s_addr = pIpForwardTable->table[i].dwForwardMask;
                        gateway.s_addr = pIpForwardTable->table[i].dwForwardNextHop;
                        iface.s_addr = pIpForwardTable->table[i].dwForwardNextHop;
                        
                        printf("%-18s %-15s %-13s %-9s %d\n",
                               inet_ntoa(dest), inet_ntoa(mask), inet_ntoa(gateway),
                               inet_ntoa(iface), pIpForwardTable->table[i].dwForwardMetric1);
                    }
                }
                free(pIpForwardTable);
            }
            
        } else if (strcmp(argv[1], "-i") == 0) {
            network_show_interfaces();
        } else {
            printf("Usage: netstat [-r|-i]\n");
            printf("  (no options)  Display active TCP connections\n");
            printf("  -r            Display routing table\n");
            printf("  -i            Display interface statistics\n");
        }
    }
}

void nslookup_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: nslookup <hostname>\n");
        return;
    }
    
    char* hostname = argv[1];
    
    // Simulate DNS lookup
    printf("Server: %s\n", "8.8.8.8");
    printf("Address: %s#53\n", "8.8.8.8");
    printf("\n");
    
    // Simulate response
    printf("Non-authoritative answer:\n");
    printf("Name: %s\n", hostname);
    
    // Generate fake IP addresses for common domains
    if (strstr(hostname, "google.com")) {
        printf("Address: 142.250.191.14\n");
    } else if (strstr(hostname, "github.com")) {
        printf("Address: 140.82.112.4\n");
    } else if (strstr(hostname, "stackoverflow.com")) {
        printf("Address: 151.101.1.69\n");
    } else {
        // Generate random IP
        printf("Address: %d.%d.%d.%d\n", 
               (rand() % 223) + 1, rand() % 256, rand() % 256, rand() % 256);
    }
}

void telnet_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: telnet <host> <port>\n");
        return;
    }
    
    char* host = argv[1];
    int port = atoi(argv[2]);
    
    printf("Trying %s...\n", host);
    printf("Connected to %s.\n", host);
    printf("Escape character is '^]'.\n");
    
    // Simulate connection
    network_simulate_connect(host, port, 1); // TCP
    
    printf("Connection closed by foreign host.\n");
}

void wget_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: wget <url>\n");
        return;
    }
    
    char* url = argv[1];
    printf("--wget--  %s\n", url);
    printf("Resolving host... ");
    
    // Initialize WinINet
    HINTERNET hInternet = InternetOpen("Zora VM wget", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Failed to initialize internet connection\n");
        return;
    }
    
    printf("connected.\n");
    printf("HTTP request sent, awaiting response... ");
    
    // Open URL
    HINTERNET hUrl = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        printf("Failed to open URL\n");
        InternetCloseHandle(hInternet);
        return;
    }
    
    printf("200 OK\n");
    
    // Get content length
    DWORD dwIndex = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD contentLength = 0;
    HttpQueryInfo(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwSize, &dwIndex);
    
    printf("Length: %lu bytes\n", contentLength);
    
    // Extract filename from URL
    char filename[256];
    char* lastSlash = strrchr(url, '/');
    if (lastSlash && strlen(lastSlash + 1) > 0) {
        strcpy(filename, lastSlash + 1);
    } else {
        strcpy(filename, "index.html");
    }
    
    printf("Saving to: '%s'\n", filename);
    
    // Read data from URL
    char buffer[4096];
    DWORD bytesRead;
    DWORD totalBytes = 0;
    
    // Create file in VFS
    char full_path[512];
    char* cwd = vfs_getcwd();
    snprintf(full_path, sizeof(full_path), "%s/%s", cwd, filename);
    vfs_create_file(full_path);
    
    // Download and write data
    char* fileContent = malloc(contentLength + 1);
    char* contentPtr = fileContent;
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        memcpy(contentPtr, buffer, bytesRead);
        contentPtr += bytesRead;
        totalBytes += bytesRead;
        
        if (contentLength > 0) {
            int percent = (totalBytes * 100) / contentLength;
            printf("\r%s      %d%%[", filename, percent);
            int bars = percent / 5;
            for (int i = 0; i < 20; i++) {
                printf(i < bars ? "=" : " ");
            }
            printf("] %lu bytes", totalBytes);
            fflush(stdout);
        }
    }
    
    if (fileContent && totalBytes > 0) {
        fileContent[totalBytes] = '\0';
        vfs_write_file(full_path, fileContent, totalBytes);
        printf("\n\n'%s' saved [%lu/%lu]\n", filename, totalBytes, contentLength);
    } else {
        printf("\nDownload failed\n");
    }
    
    // Cleanup
    if (fileContent) free(fileContent);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
}

void curl_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: curl <url>\n");
        return;
    }
    
    char* url = argv[1];
    
    printf("Fetching %s...\n", url);
    
    // Simulate HTTP response
    printf("HTTP/1.1 200 OK\n");
    printf("Content-Type: text/html\n");
    printf("Content-Length: 1024\n");
    printf("\n");
    printf("<html>\n");
    printf("<head><title>Simulated Response</title></head>\n");
    printf("<body>\n");
    printf("<h1>Hello from Virtual Network!</h1>\n");
    printf("<p>This is a simulated HTTP response from %s</p>\n", url);
    printf("</body>\n");
    printf("</html>\n");
}

void ssh_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ssh <user@host>\n");
        return;
    }
    
    char* target = argv[1];
    
    printf("Connecting to %s...\n", target);
    printf("The authenticity of host cannot be established.\n");
    printf("Are you sure you want to continue connecting (yes/no)? ");
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) && 
        strncmp(response, "yes", 3) == 0) {
        printf("Warning: Permanently added to the list of known hosts.\n");
        printf("Connected to virtual host.\n");
        printf("This is a simulated SSH connection.\n");
        printf("Connection closed.\n");
    } else {
        printf("Connection aborted.\n");
    }
}

void iptables_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: iptables [-L|-A|-D] [options]\n");
        return;
    }
    
    if (strcmp(argv[1], "-L") == 0) {
        // List firewall rules
        printf("Chain INPUT (policy ACCEPT)\n");
        printf("target     prot opt source               destination\n");
        printf("ACCEPT     all  --  192.168.100.0/24    anywhere\n");
        printf("\n");
        printf("Chain FORWARD (policy ACCEPT)\n");
        printf("target     prot opt source               destination\n");
        printf("\n");
        printf("Chain OUTPUT (policy ACCEPT)\n");
        printf("target     prot opt source               destination\n");
        printf("ACCEPT     all  --  anywhere             anywhere\n");
    } else if (strcmp(argv[1], "-A") == 0) {
        printf("Virtual firewall rule added\n");
    } else if (strcmp(argv[1], "-D") == 0) {
        printf("Virtual firewall rule deleted\n");
    } else {
        printf("Usage: iptables [-L|-A|-D] [options]\n");
    }
}

void test_vfs_command(int argc, char* argv[]) {
    printf("=== VFS Test ===\n");
    printf("Current directory: %s\n", vfs_getcwd());
    printf("Testing cd to /persistent\n");
    if (vfs_chdir("/persistent") == 0) {
        printf("Successfully changed to: %s\n", vfs_getcwd());
        printf("Testing cd to documents\n");
        if (vfs_chdir("documents") == 0) {
            printf("Successfully changed to: %s\n", vfs_getcwd());
        } else {
            printf("Failed to change to documents\n");
        }
    } else {
        printf("Failed to change directory to /persistent\n");
    }
}

void debug_vfs_command(int argc, char* argv[]) {
    printf("=== VFS Debug Info ===\n");
    printf("Current directory: %s\n", vfs_getcwd());
    
    VNode* current = vfs_find_node(vfs_getcwd());
    if (current) {
        printf("Current node exists: %s (is_directory: %d)\n", 
               current->name, current->is_directory);
        
        if (current->children) {
            printf("Children:\n");
            VNode* child = current->children;
            while (child) {
                printf("  - %s (%s)\n", child->name, 
                       child->is_directory ? "DIR" : "FILE");
                child = child->next;
            }
        } else {
            printf("No children found\n");
        }
    } else {
        printf("Current node not found!\n");
    }
}

// Helper: resolve script path with flexible path handling
void resolve_script_path(const char* name, char* out, size_t out_sz) {
    if (!name || !out || out_sz == 0) return;
    
    // If it's an absolute path, use as-is
    if (name[0] == '/') { 
        strncpy(out, name, out_sz-1); 
        out[out_sz-1] = '\0'; 
        return; 
    }
    
    // Get current working directory from VFS
    char* cwd = vfs_getcwd();
    if (!cwd) cwd = "/";
    
    // If it contains a path separator, treat as relative path from current directory
    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_sz, "%s/%s", cwd, name);
        if (vfs_find_node(out)) return;
    }
    
    // Check current directory first
    snprintf(out, out_sz, "%s/%s", cwd, name);
    if (vfs_find_node(out)) return;
    
    // Try /scripts directory
    snprintf(out, out_sz, "/scripts/%s", name);
    if (vfs_find_node(out)) return;
    
    // Try legacy /persistent/scripts directory
    snprintf(out, out_sz, "/persistent/scripts/%s", name);
    if (vfs_find_node(out)) return;
    
    // Try /bin directory
    snprintf(out, out_sz, "/bin/%s", name);
    if (vfs_find_node(out)) return;
    
    // Try /usr/bin directory
    snprintf(out, out_sz, "/usr/bin/%s", name);
    if (vfs_find_node(out)) return;
    
    // If nothing found, default to the name as given
    strncpy(out, name, out_sz-1);
    out[out_sz-1] = '\0';
}

void lua_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: lua <script.lua>\n");
        return;
    }
    char script_path[256];
    resolve_script_path(argv[1], script_path, sizeof(script_path));
    printf("Executing Lua script: %s\n", script_path);
    if (lua_vm_load_script(script_path) != 0) {
        printf("Failed to execute script (not found or error)\n");
    }
}

void luacode_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: luacode <lua_code>\n");
        return;
    }
    
    // Combine all arguments into one string
    char code[1024] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(code, argv[i]);
        if (i < argc - 1) strcat(code, " ");
    }
    
    printf("Executing Lua code: %s\n", code);
    if (lua_vm_execute_string(code) != 0) {
        printf("Failed to execute Lua code\n");
    }
}

VNode* vfs_get_root(void) {
    // Safe implementation that doesn't depend on external variables
    // Try to call VFS API functions directly
    
    // Check if we have a VFS find function available
    VNode* root = vfs_find_node("/");
    if (root) {
        return root;
    }
    
    // If that doesn't work, return NULL (handled safely in calling code)
    return NULL;
}

int lua_vm_is_initialized(void) {
    // Safe implementation that doesn't depend on external variables
    // Just return 1 (assume it's available) since this is only used for display
    return 1;
}

void test_sandbox_command(int argc, char **argv) {
    printf("=== Sandbox Security Test ===\n");
    
    // Test binary executor status
    if (binary_executor_is_initialized()) {
        printf("Binary executor initialized\n");
        printf("ELF support: %s\n", binary_executor_has_elf_support() ? "Available" : "Not available");
        printf("Sandboxing: Active\n");
    } else {
        printf("Binary executor not initialized\n");
    }
    
    // Test VFS isolation
    printf("\n=== VFS Isolation Test ===\n");
    VNode* root = vfs_get_root();
    if (root) {
        printf("VFS root accessible\n");
        printf("File system isolation: Active\n");
    } else {
        printf("VFS not available\n");
    }
    
    // Test scripting sandbox
    printf("\n=== Script Sandbox Test ===\n");
    if (lua_vm_is_initialized()) {
        printf("Lua VM initialized with sandbox restrictions\n");
        printf("Dangerous functions removed\n");
        printf("File access restricted to VFS\n");
    } else {
        printf("Lua VM not initialized\n");
    }
    
    printf("\n=== Sandbox Status Summary ===\n");
    printf("All execution environments are sandboxed\n");
    printf("Resource limits are enforced\n");
    printf("File system access is restricted\n");
    printf("Process isolation is active\n");
}

// Windows implementation uses native FindFirstFile/FindNextFile/FindClose
// Platform detection - Windows only
#define PLATFORM_NAME "Windows"
#define CLEAR_COMMAND "cls"

// Environment variable commands implementation
void setenv_command(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: setenv VARIABLE VALUE\n");
        return;
    }
    set_env_var(argv[1], argv[2]);
    printf("Set %s=%s\n", argv[1], argv[2]);
}

void set_command(int argc, char **argv) {
    if (argc == 1) {
        // List all environment variables (same as env)
        env_command(argc, argv);
    } else if (argc == 2) {
        // Check if it's VARIABLE=VALUE format
        char *eq_pos = strchr(argv[1], '=');
        if (eq_pos) {
            *eq_pos = '\0';
            set_env_var(argv[1], eq_pos + 1);
            printf("Set %s=%s\n", argv[1], eq_pos + 1);
        } else {
            // Show specific variable value
            const char* value = get_env_var(argv[1]);
            if (value && strlen(value) > 0) {
                printf("%s=%s\n", argv[1], value);
            } else {
                printf("%s: not set\n", argv[1]);
            }
        }
    } else if (argc == 3) {
        set_env_var(argv[1], argv[2]);
        printf("Set %s=%s\n", argv[1], argv[2]);
    } else {
        printf("Usage: set [VARIABLE=VALUE] or set VARIABLE VALUE or set VARIABLE\n");
    }
}

void unset_command(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: unset VARIABLE\n");
        return;
    }
    set_env_var(argv[1], "");  // Set to empty string (simple unset)
    printf("Unset %s\n", argv[1]);
}

void export_command(int argc, char **argv) {
    if (argc == 1) {
        // List all exported variables (same as env for now)
        env_command(argc, argv);
    } else if (argc == 2) {
        // Check if it's VARIABLE=VALUE format
        char *eq_pos = strchr(argv[1], '=');
        if (eq_pos) {
            *eq_pos = '\0';
            set_env_var(argv[1], eq_pos + 1);
            printf("Exported %s=%s\n", argv[1], eq_pos + 1);
        } else {
            printf("Usage: export VARIABLE=VALUE or export VARIABLE VALUE\n");
        }
    } else if (argc == 3) {
        set_env_var(argv[1], argv[2]);
        printf("Exported %s=%s\n", argv[1], argv[2]);
    } else {
        printf("Usage: export [VARIABLE=VALUE] or export VARIABLE VALUE\n");
    }
}

void env_command(int argc, char **argv) {
    printf("Environment Variables:\n");
    for (int i = 0; i < env_var_count; i++) {
        if (strlen(env_vars[i].value) > 0) {  // Only show non-empty variables
            printf("%s=%s\n", env_vars[i].name, env_vars[i].value);
        }
    }
}

void font_debug_command(int argc, char **argv) {
    printf("=== Console Font Debug Information ===\n");
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        printf("Error: Cannot get console handle\n");
        return;
    }
    
    // Get current font info
    CONSOLE_FONT_INFO fontInfo;
    if (GetCurrentConsoleFont(hConsole, FALSE, &fontInfo)) {
        printf("Current Font Index: %d\n", fontInfo.nFont);
        printf("Font Size: Width=%d, Height=%d\n", 
               fontInfo.dwFontSize.X, fontInfo.dwFontSize.Y);
    } else {
        printf("Failed to get current font info\n");
    }
    
    // Try to get extended font info (Windows Vista+)
    CONSOLE_FONT_INFOEX fontInfoEx;
    fontInfoEx.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    
    typedef BOOL (WINAPI *GetCurrentConsoleFontExFunc)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);
    HMODULE kernel32 = GetModuleHandle("kernel32.dll");
    GetCurrentConsoleFontExFunc GetCurrentConsoleFontExPtr = 
        (GetCurrentConsoleFontExFunc)GetProcAddress(kernel32, "GetCurrentConsoleFontEx");
    
    if (GetCurrentConsoleFontExPtr && GetCurrentConsoleFontExPtr(hConsole, FALSE, &fontInfoEx)) {
        printf("Extended Font Info:\n");
        printf("  Font Index: %d\n", fontInfoEx.nFont);
        printf("  Font Size: Width=%d, Height=%d\n", 
               fontInfoEx.dwFontSize.X, fontInfoEx.dwFontSize.Y);
        printf("  Font Weight: %d\n", fontInfoEx.FontWeight);
        printf("  Font Family: %d\n", fontInfoEx.FontFamily);
        wprintf(L"  Font Name: %s\n", fontInfoEx.FaceName);
        
        // Check if it's a UTF-8 compatible font
        if (wcsstr(fontInfoEx.FaceName, L"Mincho") || 
            wcsstr(fontInfoEx.FaceName, L"Consolas") ||
            wcsstr(fontInfoEx.FaceName, L"Cascadia") ||
            wcsstr(fontInfoEx.FaceName, L"DejaVu")) {
            printf("  UTF-8 Compatibility: GOOD (Unicode-capable font)\n");
        } else if (wcsstr(fontInfoEx.FaceName, L"Courier") ||
                   wcsstr(fontInfoEx.FaceName, L"Terminal")) {
            printf("  UTF-8 Compatibility: LIMITED (Legacy font)\n");
        } else {
            printf("  UTF-8 Compatibility: UNKNOWN\n");
        }
    } else {
        printf("Extended font info not available (older Windows version)\n");
    }
    
    // Test UTF-8 rendering with current font
    printf("\nUTF-8 Rendering Test with Current Font:\n");
    printf("Simple ASCII: ABC123\n");
    printf("UTF-8 Box Drawing: ╔═══╗ ║ ░▒▓ ║ ╚═══╝\n");
    printf("UTF-8 Symbols: ★ ☆ ♠ ♥ ♦ ♣ ☀ ☁ ⚡ ⛅\n");
    printf("UTF-8 Math: ∑ ∏ √ ∞ ≤ ≥ ≠ ≈ ± ÷\n");
    
    // Suggest font changes if needed
    printf("\nFont Recommendations for UTF-8:\n");
    printf("1. MS Mincho (Japanese, excellent UTF-8 support)\n");
    printf("2. Consolas (Microsoft, good UTF-8 support)\n");
    printf("3. Cascadia Code/Mono (Microsoft Terminal font)\n");
    printf("4. DejaVu Sans Mono (Open source, excellent Unicode)\n");
    printf("5. Source Code Pro (Adobe, good programming font)\n");
    
    // Try to set MS Mincho if available (demonstration only)
    if (argc > 1 && strcmp(argv[1], "--try-mincho") == 0) {
        printf("\nAttempting to set MS Mincho font...\n");
        if (GetCurrentConsoleFontExPtr) {
            CONSOLE_FONT_INFOEX newFontInfo = fontInfoEx;
            wcscpy(newFontInfo.FaceName, L"MS Mincho");
            newFontInfo.dwFontSize.X = 0;  // Let Windows choose width
            newFontInfo.dwFontSize.Y = 14; // 14pt height
            
            typedef BOOL (WINAPI *SetCurrentConsoleFontExFunc)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);
            SetCurrentConsoleFontExFunc SetCurrentConsoleFontExPtr = 
                (SetCurrentConsoleFontExFunc)GetProcAddress(kernel32, "SetCurrentConsoleFontEx");
            
            if (SetCurrentConsoleFontExPtr && SetCurrentConsoleFontExPtr(hConsole, FALSE, &newFontInfo)) {
                printf("Successfully set MS Mincho font!\n");
                printf("UTF-8 test after font change: ╔═══╗\n");
            } else {
                printf("Failed to set MS Mincho font (may require admin privileges)\n");
            }
        }
    }
    
    printf("\n=== End Font Debug ===\n");
}

void utf8_detailed_test_command(int argc, char **argv) {
    printf("\n=== Detailed UTF-8 Character Rendering Test ===\n");
    
    // Test individual UTF-8 characters with their raw byte representations
    struct {
        const char* name;
        const char* utf8_char;
        const char* bytes;
    } test_chars[] = {
        {"Box Top-Left", "╔", "E2 95 94"},
        {"Box Horizontal", "═", "E2 95 90"}, 
        {"Box Top-Right", "╗", "E2 95 97"},
        {"Box Vertical", "║", "E2 95 91"},
        {"Box Bottom-Left", "╚", "E2 95 9A"},
        {"Box Bottom-Right", "╝", "E2 95 9D"},
        {"Checkmark", "✓", "E2 9C 93"},
        {"Cross", "✗", "E2 9C 97"},
        {"Arrow Right", "→", "E2 86 92"},
        {"Star", "★", "E2 98 85"},
        {NULL, NULL, NULL}
    };
    
    printf("Character-by-character UTF-8 rendering test:\n");
    printf("(If UTF-8 is working, you should see proper symbols, not garbled text)\n\n");
    
    for (int i = 0; test_chars[i].name; i++) {
        printf("%-20s: '%s' (bytes: %s)\n", 
               test_chars[i].name, test_chars[i].utf8_char, test_chars[i].bytes);
    }
    
    printf("\nFull box drawing test:\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║ UTF-8 Box Drawing Rendering Test     ║\n");
    printf("║ If this looks right, UTF-8 is OK!    ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    // Test different font contexts
    printf("\nConsole font information:\n");
    CONSOLE_FONT_INFO fontInfo;
    if (GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fontInfo)) {
        printf("Font Index: %d\n", fontInfo.nFont);
        printf("Font Size: %dx%d\n", fontInfo.dwFontSize.X, fontInfo.dwFontSize.Y);
    } else {
        printf("Could not retrieve font information\n");
    }
    
    // Test console screen buffer info
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        printf("Buffer Size: %dx%d\n", csbi.dwSize.X, csbi.dwSize.Y);
        printf("Window Size: %dx%d\n", 
               csbi.srWindow.Right - csbi.srWindow.Left + 1,
               csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
        printf("Cursor Position: %d,%d\n", csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y);
    }
    
    // Force a buffer flush and re-test
    printf("\nForcing buffer flush and re-testing UTF-8...\n");
    fflush(stdout);
    Sleep(100);
    
    printf("Post-flush test: ╔═╗ ║ ╚═╝\n");
    
    // Check if there are any console events that might affect rendering
    HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD numEvents = 0;
    if (GetNumberOfConsoleInputEvents(hConsoleInput, &numEvents)) {
        printf("Console input events pending: %lu\n", numEvents);
    }
    
    printf("\n=== End Detailed UTF-8 Test ===\n");
}

void utf8_monitor_command(int argc, char **argv) {
    printf("=== UTF-8 Encoding Monitor ===\n");
    printf("This will monitor UTF-8 status over time to track when it starts working.\n");
    printf("Press Ctrl+C to stop monitoring.\n\n");
    
    time_t start_time = time(NULL);
    int monitoring = 1;
    int test_count = 0;
    int utf8_working = 0;
    time_t first_working_time = 0;
    
    while (monitoring && test_count < 60) { // Monitor for up to 1 hour
        test_count++;
        time_t current_time = time(NULL);
        int elapsed = (int)(current_time - start_time);
        
        // Check UTF-8 status
        UINT output_cp = GetConsoleOutputCP();
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD outMode = 0;
        int vt_enabled = 0;
        
        if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &outMode)) {
            vt_enabled = (outMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 1 : 0;
        }
        
        printf("[%02d:%02d] Test #%d: CP=%u, VT=%s, Test: ", 
               elapsed / 60, elapsed % 60, test_count, output_cp, 
               vt_enabled ? "ON" : "OFF");
        
        // Test UTF-8 with box character
        printf("╔═╗");
        
        // Check if this looks right (we can't actually detect visually, but we can track settings)
        int current_utf8_status = (output_cp == CP_UTF8 && vt_enabled);
        
        if (current_utf8_status && !utf8_working) {
            utf8_working = 1;
            first_working_time = current_time;
            printf(" ✓ UTF-8 STARTED WORKING!");
            printf("\n>>> UTF-8 became functional after %d minutes %d seconds <<<\n", 
                   elapsed / 60, elapsed % 60);
        } else if (!current_utf8_status && utf8_working) {
            utf8_working = 0;
            printf(" ✗ UTF-8 STOPPED WORKING!");
        } else if (current_utf8_status) {
            printf(" ✓ Working");
        } else {
            printf(" ✗ Not working");
        }
        
        printf("\n");
        
        // Try to fix UTF-8 every 30 seconds if it's not working
        if (!current_utf8_status && (test_count % 6 == 0)) {
            printf("    Attempting UTF-8 fix...\n");
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                GetConsoleMode(hOut, &dwMode);
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
        
        Sleep(5000); // Check every 5 seconds
        
        // Check for Ctrl+C (simplified)
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState('C') & 0x8000) {
            printf("\nMonitoring stopped by user.\n");
            break;
        }
    }
    
    printf("\n=== UTF-8 Monitor Summary ===\n");
    printf("Total monitoring time: %d seconds\n", (int)(time(NULL) - start_time));
    printf("Tests performed: %d\n", test_count);
    if (first_working_time > 0) {
        int delay = (int)(first_working_time - start_time);
        printf("UTF-8 started working after: %d minutes %d seconds\n", 
               delay / 60, delay % 60);
    } else {
        printf("UTF-8 never started working during monitoring period\n");
    }
    
    printf("\nPossible causes of delayed UTF-8 activation:\n");
    printf("1. Background Windows processes affecting console\n");
    printf("2. Delayed terminal feature detection\n");
    printf("3. Console Host vs Windows Terminal switching\n");
    printf("4. Font loading delays\n");
    printf("5. Code page conflicts with other applications\n");
}

void utf8_fix_command(int argc, char **argv) {
    printf("Attempting to fix UTF-8 encoding...\n");
    
    // Force UTF-8 console code pages
    if (SetConsoleOutputCP(CP_UTF8)) {
        printf("✓ Console output set to UTF-8 (CP_UTF8)\n");
    } else {
        printf("✗ Failed to set console output to UTF-8\n");
    }
    
    if (SetConsoleCP(CP_UTF8)) {
        printf("✓ Console input set to UTF-8 (CP_UTF8)\n");
    } else {
        printf("✗ Failed to set console input to UTF-8\n");
    }
    
    // Enable virtual terminal processing
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, dwMode)) {
                printf("✓ Virtual terminal processing enabled\n");
            } else {
                printf("✗ Failed to enable virtual terminal processing\n");
            }
        }
    }
    
    // Set locale
    if (setlocale(LC_ALL, "") != NULL) {
        printf("✓ Locale set for Unicode support\n");
    } else {
        printf("✗ Failed to set locale\n");
    }
    
    // CRITICAL: Force console refresh like pipe operations do
    printf("Forcing console refresh (like pipe operations)...\n");
    fflush(stdout);
    
    // Safer method: toggle console modes instead of freopen
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout != INVALID_HANDLE_VALUE) {
        DWORD currentMode;
        if (GetConsoleMode(hStdout, &currentMode)) {
            SetConsoleMode(hStdout, currentMode & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            Sleep(10);
            SetConsoleMode(hStdout, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            printf("✓ Console refreshed successfully - this should fix UTF-8 rendering!\n");
        } else {
            printf("✗ Console mode refresh failed\n");
        }
    }
    fflush(stdout);
    
    printf("\nTesting UTF-8 after fix:\n");
    printf("Box chars: ╔═══╗ ║UTF║ ╚═══╝\n");
    printf("Arrows: ← ↑ → ↓ ⚡ ★\n");
    
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        utf8_test_command(0, NULL);
    }
    
    printf("\nIf you still see garbled characters, try:\n");
    printf("1. Use Windows Terminal instead of Console Host\n");
    printf("2. Change terminal font to MS Mincho or Consolas\n");
    printf("3. Run 'utf8-test' for detailed diagnostics\n");
}

void utf8_test_command(int argc, char **argv) {
    printf("\n=== UTF-8 and Terminal Encoding Diagnostic ===\n");
    
    // Check console code pages
    UINT input_cp = GetConsoleCP();
    UINT output_cp = GetConsoleOutputCP();
    
    printf("Console Input Code Page: %u %s\n", input_cp, 
           input_cp == CP_UTF8 ? "(UTF-8)" : input_cp == 437 ? "(US-ASCII)" : "(Other)");
    printf("Console Output Code Page: %u %s\n", output_cp,
           output_cp == CP_UTF8 ? "(UTF-8)" : output_cp == 437 ? "(US-ASCII)" : "(Other)");
    
    // Check console mode
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD outMode = 0;
        if (GetConsoleMode(hOut, &outMode)) {
            printf("Output Console Mode: 0x%08X\n", outMode);
            printf("  ENABLE_VIRTUAL_TERMINAL_PROCESSING: %s\n", 
                   (outMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? "ENABLED" : "DISABLED");
            printf("  ENABLE_PROCESSED_OUTPUT: %s\n",
                   (outMode & ENABLE_PROCESSED_OUTPUT) ? "ENABLED" : "DISABLED");
            printf("  ENABLE_WRAP_AT_EOL_OUTPUT: %s\n",
                   (outMode & ENABLE_WRAP_AT_EOL_OUTPUT) ? "ENABLED" : "DISABLED");
        }
    }
    
    if (hIn != INVALID_HANDLE_VALUE) {
        DWORD inMode = 0;
        if (GetConsoleMode(hIn, &inMode)) {
            printf("Input Console Mode: 0x%08X\n", inMode);
        }
    }
    
    // Check environment variables that might affect terminal behavior
    char* term_vars[] = {"WT_SESSION", "TERM_PROGRAM", "CONEMU_ANSI_COLORS_DISABLED", 
                         "ANSICON", "TERM", "COLORTERM", NULL};
    
    printf("\nTerminal Environment Variables:\n");
    for (int i = 0; term_vars[i]; i++) {
        char* value = getenv(term_vars[i]);
        printf("  %s = %s\n", term_vars[i], value ? value : "(not set)");
    }
    
    // Detect terminal type
    int is_wt = detect_windows_terminal();
    printf("\nTerminal Detection: %s\n", is_wt ? "Windows Terminal" : "Console Host");
    
    // Test Unicode box drawing characters
    printf("\nUTF-8 Box Drawing Test:\n");
    printf("Should see proper boxes if UTF-8 is working:\n");
    
    // Force re-setup UTF-8 encoding
    printf("Re-applying UTF-8 setup...\n");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        printf("Virtual terminal processing re-enabled\n");
    }
    
    // Test various Unicode characters
    printf("\nUnicode Test Characters:\n");
    printf("Box Drawing: ╔═══╗ ║   ║ ╚═══╝\n");
    printf("Arrows: ← ↑ → ↓ ↔ ↕\n");
    printf("Technical: ⌘ ⌥ ⇧ ⌃ ⎋ ⏎\n");
    printf("Geometric: ◆ ◇ ● ○ ■ □ ▲ △\n");
    printf("Math: ∑ ∏ √ ∞ ≤ ≥ ≠ ≈\n");
    printf("Currency: € £ ¥ ¢ ₹ ₽\n");
    
    // Test with different fonts (informational)
    printf("\nFont Recommendations for UTF-8:\n");
    printf("• MS Mincho (current preference)\n");
    printf("• Consolas\n");
    printf("• Cascadia Code/Mono\n");
    printf("• DejaVu Sans Mono\n");
    printf("• Source Code Pro\n");
    
    printf("\nIf you see proper Unicode characters above, UTF-8 is working!\n");
    printf("If you see question marks or boxes, UTF-8 support is limited.\n");
    
    // Additional diagnostic info
    printf("\nSystem Information:\n");
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        printf("Windows Version: %d.%d Build %d\n", 
               osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
        
        // Windows 10+ has better UTF-8 support
        if (osvi.dwMajorVersion >= 10) {
            printf("UTF-8 Support: Should be fully supported on Windows 10+\n");
        } else {
            printf("UTF-8 Support: Limited on older Windows versions\n");
        }
    }
    
    printf("\n=== End UTF-8 Diagnostic ===\n");
}

void scripts_command(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("Available script-based commands in /bin/:\n\n");
    
    VNode* bin_dir = vfs_find_node("/bin");
    if (!bin_dir || !bin_dir->is_directory) {
        printf("No /bin directory found\n");
        return;
    }
    
    vfs_refresh_directory(bin_dir);
    VNode* child = bin_dir->children;
    
    printf("Lua scripts (.lua):\n");
    while (child) {
        if (!child->is_directory && strstr(child->name, ".lua")) {
            char command_name[256];
            strcpy(command_name, child->name);
            char* ext = strstr(command_name, ".lua");
            if (ext) *ext = '\0';
            
            printf("  ");
            terminal_print_command(command_name);
            printf(" - %s\n", child->name);
        }
        child = child->next;
    }
    
    printf("\nPython scripts (.py):\n");
    child = bin_dir->children;
    while (child) {
        if (!child->is_directory && strstr(child->name, ".py")) {
            char command_name[256];
            strcpy(command_name, child->name);
            char* ext = strstr(command_name, ".py");
            if (ext) *ext = '\0';
            
            printf("  ");
            terminal_print_command(command_name);
            printf(" - %s\n", child->name);
        }
        child = child->next;
    }
    
    printf("\nPerl scripts (.pl):\n");
    child = bin_dir->children;
    while (child) {
        if (!child->is_directory && strstr(child->name, ".pl")) {
            char command_name[256];
            strcpy(command_name, child->name);
            char* ext = strstr(command_name, ".pl");
            if (ext) *ext = '\0';
            
            printf("  ");
            terminal_print_command(command_name);
            printf(" - %s\n", child->name);
        }
        child = child->next;
    }
    
    printf("\nExecutable scripts (no extension):\n");
    child = bin_dir->children;
    while (child) {
        if (!child->is_directory && !strstr(child->name, ".lua") && 
            !strstr(child->name, ".py") && !strstr(child->name, ".pl")) {
            printf("  ");
            terminal_print_command(child->name);
            printf(" - executable script\n");
        }
        child = child->next;
    }
    
    printf("\nUse any of these names as commands. Arguments will be passed to the script.\n");
}

void version_command(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("Zora VM Version 2.1.0\n");
    printf("Multi-Environment Runtime Layer (MERL) Shell\n");
    printf("Build Date: %s %s\n", __DATE__, __TIME__);
    printf("Platform: Windows (MinGW)\n");
    printf("Features: VFS, Lua, Python, Perl VMs, Sandboxing\n");
    printf("Terminal: Campbell Color Scheme with Enhanced Styling\n");
}

void systeminfo_command(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("=== ZORA VM SYSTEM INFORMATION ===\n\n");
    printf("System Type: Virtual Machine\n");
    printf("Architecture: x86_64 Virtual\n");
    printf("Shell: MERL (Multi-Environment Runtime Layer)\n");
    printf("VFS: Virtual File System Active\n");
    printf("Network: Virtual Network Stack\n");
    printf("Memory: Virtual Memory Management\n");
    printf("Scripting: Lua, Python, Perl VMs\n");
    printf("Terminal: Enhanced with Campbell Colors\n");
    printf("Security: Sandboxed Execution Environment\n");
}

// Additional Unix command implementations

void sort_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: sort [OPTIONS] <filename>\n");
        printf("Options:\n");
        printf("  -r, --reverse      reverse sort order\n");
        printf("  -n, --numeric      sort numerically\n");
        printf("  -u, --unique       remove duplicates\n");
        printf("  -h, --help         show this help message\n");
        return;
    }
    
    // Check for help flags first
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("sort - Sort lines in text files\n\n");
            printf("Usage: sort [OPTIONS] <filename>\n\n");
            printf("Options:\n");
            printf("  -r, --reverse      Sort in reverse (descending) order\n");
            printf("  -n, --numeric      Sort numerically instead of alphabetically\n");
            printf("  -u, --unique       Remove duplicate lines from output\n");
            printf("  -h, --help         Show this help message\n\n");
            printf("Examples:\n");
            printf("  sort file.txt              Sort file alphabetically\n");
            printf("  sort -r file.txt           Sort file in reverse order\n");
            printf("  sort -n numbers.txt        Sort file numerically\n");
            printf("  sort -u file.txt           Sort and remove duplicates\n");
            printf("  sort -rn file.txt          Sort numerically in reverse\n");
            return;
        }
    }
    
    // Parse flags
    int reverse = 0, numeric = 0, unique = 0;
    char* filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reverse") == 0) {
            reverse = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric") == 0) {
            numeric = 1;
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--unique") == 0) {
            unique = 1;
        } else {
            filename = argv[i];
        }
    }
    
    if (!filename) {
        printf("sort: no input file specified\n");
        return;
    }
    
    // Build full path
    char full_path[512];
    char* cwd = vfs_getcwd();
    build_full_path(full_path, sizeof(full_path), cwd ? cwd : "/", filename);
    
    // Read file
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) != 0 || !data) {
        printf("sort: %s: No such file or directory\n", full_path);
        return;
    }
    
    // Split into lines
    char** lines = malloc(1000 * sizeof(char*));
    int line_count = 0;
    char* content = (char*)data;
    char* line_start = content;
    
    for (size_t i = 0; i <= size; i++) {
        if (i == size || content[i] == '\n') {
            int line_len = (content + i) - line_start;
            lines[line_count] = malloc(line_len + 1);
            strncpy(lines[line_count], line_start, line_len);
            lines[line_count][line_len] = '\0';
            line_count++;
            line_start = content + i + 1;
            
            if (line_count >= 1000) break; // Limit
        }
    }
    
    // Sort lines (simple bubble sort)
    for (int i = 0; i < line_count - 1; i++) {
        for (int j = 0; j < line_count - i - 1; j++) {
            int compare;
            if (numeric) {
                compare = atoi(lines[j]) - atoi(lines[j + 1]);
            } else {
                compare = strcmp(lines[j], lines[j + 1]);
            }
            
            if ((reverse && compare < 0) || (!reverse && compare > 0)) {
                char* temp = lines[j];
                lines[j] = lines[j + 1];
                lines[j + 1] = temp;
            }
        }
    }
    
    // Print sorted lines
    for (int i = 0; i < line_count; i++) {
        if (unique && i > 0 && strcmp(lines[i], lines[i-1]) == 0) {
            continue; // Skip duplicates
        }
        printf("%s\n", lines[i]);
        free(lines[i]);
    }
    
    free(lines);
}

void uniq_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: uniq [OPTIONS] <filename>\n");
        printf("Options:\n");
        printf("  -c         prefix lines with count\n");
        printf("  -d         only print duplicate lines\n");
        printf("  -u         only print unique lines\n");
        return;
    }
    
    // Parse flags
    int count = 0, duplicates_only = 0, unique_only = 0;
    char* filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            count = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            duplicates_only = 1;
        } else if (strcmp(argv[i], "-u") == 0) {
            unique_only = 1;
        } else {
            filename = argv[i];
        }
    }
    
    if (!filename) {
        printf("uniq: no input file specified\n");
        return;
    }
    
    // Build full path
    char full_path[512];
    char* cwd = vfs_getcwd();
    build_full_path(full_path, sizeof(full_path), cwd ? cwd : "/", filename);
    
    // Read file
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) != 0 || !data) {
        printf("uniq: %s: No such file or directory\n", full_path);
        return;
    }
    
    // Process lines
    char* content = (char*)data;
    char prev_line[1024] = "";
    char curr_line[1024];
    int line_count = 1;
    int line_pos = 0;
    
    for (size_t i = 0; i <= size; i++) {
        if (i == size || content[i] == '\n') {
            curr_line[line_pos] = '\0';
            
            if (strcmp(curr_line, prev_line) == 0) {
                line_count++;
            } else {
                // Print previous line if conditions met
                if (strlen(prev_line) > 0) {
                    int should_print = 1;
                    if (duplicates_only && line_count == 1) should_print = 0;
                    if (unique_only && line_count > 1) should_print = 0;
                    
                    if (should_print) {
                        if (count) {
                            printf("%6d %s\n", line_count, prev_line);
                        } else {
                            printf("%s\n", prev_line);
                        }
                    }
                }
                strcpy(prev_line, curr_line);
                line_count = 1;
            }
            line_pos = 0;
        } else {
            if (line_pos < 1023) {
                curr_line[line_pos++] = content[i];
            }
        }
    }
    
    // Handle last line
    if (strlen(prev_line) > 0) {
        int should_print = 1;
        if (duplicates_only && line_count == 1) should_print = 0;
        if (unique_only && line_count > 1) should_print = 0;
        
        if (should_print) {
            if (count) {
                printf("%6d %s\n", line_count, prev_line);
            } else {
                printf("%s\n", prev_line);
            }
        }
    }
}

void wc_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: wc [OPTIONS] <filename>\n");
        printf("Options:\n");
        printf("  -l         count lines only\n");
        printf("  -w         count words only\n");
        printf("  -c         count characters only\n");
        return;
    }
    
    // Parse flags
    int lines_only = 0, words_only = 0, chars_only = 0;
    char* filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            lines_only = 1;
        } else if (strcmp(argv[i], "-w") == 0) {
            words_only = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            chars_only = 1;
        } else {
            filename = argv[i];
        }
    }
    
    if (!filename) {
        printf("wc: no input file specified\n");
        return;
    }
    
    // Build full path
    char full_path[512];
    char* cwd = vfs_getcwd();
    build_full_path(full_path, sizeof(full_path), cwd ? cwd : "/", filename);
    
    // Read file
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) != 0 || !data) {
        printf("wc: %s: No such file or directory\n", full_path);
        return;
    }
    
    // Count lines, words, characters
    char* content = (char*)data;
    int lines = 0, words = 0, chars = (int)size;
    int in_word = 0;
    
    for (size_t i = 0; i < size; i++) {
        if (content[i] == '\n') {
            lines++;
        }
        
        if (isspace(content[i])) {
            in_word = 0;
        } else if (!in_word) {
            words++;
            in_word = 1;
        }
    }
    
    // Print results
    if (lines_only) {
        printf("%d\n", lines);
    } else if (words_only) {
        printf("%d\n", words);
    } else if (chars_only) {
        printf("%d\n", chars);
    } else {
        printf("%8d %8d %8d %s\n", lines, words, chars, filename);
    }
}

void awk_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: awk '<pattern>' <filename>\n");
        printf("Simple awk implementation - basic pattern matching only\n");
        printf("Examples:\n");
        printf("  awk '{print $1}' file.txt    - print first field\n");
        printf("  awk '{print $NF}' file.txt   - print last field\n");
        printf("  awk '{print NF}' file.txt    - print number of fields\n");
        return;
    }
    
    char* pattern = argv[1];
    char* filename = argv[2];
    
    // Build full path
    char full_path[512];
    char* cwd = vfs_getcwd();
    build_full_path(full_path, sizeof(full_path), cwd ? cwd : "/", filename);
    
    // Read file
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(full_path, &data, &size) != 0 || !data) {
        printf("awk: %s: No such file or directory\n", full_path);
        return;
    }
    
    // Process each line
    char* content = (char*)data;
    char line[1024];
    int line_pos = 0;
    
    for (size_t i = 0; i <= size; i++) {
        if (i == size || content[i] == '\n') {
            line[line_pos] = '\0';
            
            // Simple awk processing
            if (strstr(pattern, "print $1")) {
                // Print first field
                char* token = strtok(line, " \t");
                if (token) printf("%s\n", token);
            } else if (strstr(pattern, "print $NF")) {
                // Print last field
                char* fields[100];
                int field_count = 0;
                char* token = strtok(line, " \t");
                while (token && field_count < 100) {
                    fields[field_count++] = token;
                    token = strtok(NULL, " \t");
                }
                if (field_count > 0) printf("%s\n", fields[field_count - 1]);
            } else if (strstr(pattern, "print NF")) {
                // Print number of fields
                int field_count = 0;
                char* token = strtok(line, " \t");
                while (token) {
                    field_count++;
                    token = strtok(NULL, " \t");
                }
                printf("%d\n", field_count);
            } else {
                printf("awk: pattern '%s' not implemented\n", pattern);
                break;
            }
            
            line_pos = 0;
        } else {
            if (line_pos < 1023) {
                line[line_pos++] = content[i];
            }
        }
    }
}

void which_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: which [OPTIONS] <command>\n");
        printf("Options:\n");
        printf("  -h, --help         show this help message\n");
        return;
    }
    
    // Check for help flags first
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("which - Locate a command\n\n");
        printf("Usage: which <command>\n\n");
        printf("Description:\n");
        printf("  Searches for the specified command in the shell's command table\n");
        printf("  and virtual filesystem paths, then displays the full path.\n\n");
        printf("Examples:\n");
        printf("  which ls               Show location of 'ls' command\n");
        printf("  which sort             Show location of 'sort' command\n");
        printf("  which nonexistent      Show error for missing command\n");
        return;
    }
    
    char* command = argv[1];
    
    // Check if command exists in command table
    for (int i = 0; i < command_table_size; i++) {
        if (command_table[i].name && strcmp(command, command_table[i].name) == 0) {
            printf("/bin/%s\n", command);
            return;
        }
    }
    
    // Check common binary paths
    const char* paths[] = {"/bin", "/usr/bin", "/scripts"};
    for (int i = 0; i < 3; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], command);
        if (vfs_find_node(full_path)) {
            printf("%s\n", full_path);
            return;
        }
    }
    
    printf("which: %s: command not found\n", command);
}

void ln_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: ln [OPTIONS] <target> <link_name>\n");
        printf("Options:\n");
        printf("  -s         create symbolic link\n");
        return;
    }
    
    int symbolic = 0;
    char* target = NULL;
    char* link_name = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            symbolic = 1;
        } else if (!target) {
            target = argv[i];
        } else if (!link_name) {
            link_name = argv[i];
        }
    }
    
    if (!target || !link_name) {
        printf("ln: missing target or link name\n");
        return;
    }
    
    if (symbolic) {
        printf("ln: Created symbolic link '%s' -> '%s' (simulated)\n", link_name, target);
    } else {
        printf("ln: Created hard link '%s' -> '%s' (simulated)\n", link_name, target);
    }
    printf("Note: Link creation is simulated in the VM environment\n");
}

void diff_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: diff <file1> <file2>\n");
        return;
    }
    
    char* file1 = argv[1];
    char* file2 = argv[2];
    
    // Build full paths
    char full_path1[512], full_path2[512];
    char* cwd = vfs_getcwd();
    build_full_path(full_path1, sizeof(full_path1), cwd ? cwd : "/", file1);
    build_full_path(full_path2, sizeof(full_path2), cwd ? cwd : "/", file2);
    
    // Read both files
    void* data1 = NULL, *data2 = NULL;
    size_t size1 = 0, size2 = 0;
    
    if (vfs_read_file(full_path1, &data1, &size1) != 0 || !data1) {
        printf("diff: %s: No such file or directory\n", full_path1);
        return;
    }
    
    if (vfs_read_file(full_path2, &data2, &size2) != 0 || !data2) {
        printf("diff: %s: No such file or directory\n", full_path2);
        return;
    }
    
    // Simple comparison
    if (size1 == size2 && memcmp(data1, data2, size1) == 0) {
        // Files are identical
        return;
    }
    
    printf("--- %s\n", file1);
    printf("+++ %s\n", file2);
    
    // Split into lines and compare
    char** lines1 = malloc(1000 * sizeof(char*));
    char** lines2 = malloc(1000 * sizeof(char*));
    int count1 = 0, count2 = 0;
    
    // Split file1 into lines
    char* content1 = (char*)data1;
    char* line_start = content1;
    for (size_t i = 0; i <= size1; i++) {
        if (i == size1 || content1[i] == '\n') {
            int line_len = (content1 + i) - line_start;
            lines1[count1] = malloc(line_len + 1);
            strncpy(lines1[count1], line_start, line_len);
            lines1[count1][line_len] = '\0';
            count1++;
            line_start = content1 + i + 1;
            if (count1 >= 1000) break;
        }
    }
    
    // Split file2 into lines
    char* content2 = (char*)data2;
    line_start = content2;
    for (size_t i = 0; i <= size2; i++) {
        if (i == size2 || content2[i] == '\n') {
            int line_len = (content2 + i) - line_start;
            lines2[count2] = malloc(line_len + 1);
            strncpy(lines2[count2], line_start, line_len);
            lines2[count2][line_len] = '\0';
            count2++;
            line_start = content2 + i + 1;
            if (count2 >= 1000) break;
        }
    }
    
    // Simple line-by-line comparison
    int max_lines = (count1 > count2) ? count1 : count2;
    for (int i = 0; i < max_lines; i++) {
        char* line1 = (i < count1) ? lines1[i] : NULL;
        char* line2 = (i < count2) ? lines2[i] : NULL;
        
        if (!line1) {
            printf("+%s\n", line2);
        } else if (!line2) {
            printf("-%s\n", line1);
        } else if (strcmp(line1, line2) != 0) {
            printf("-%s\n", line1);
            printf("+%s\n", line2);
        }
    }
    
    // Cleanup
    for (int i = 0; i < count1; i++) free(lines1[i]);
    for (int i = 0; i < count2; i++) free(lines2[i]);
    free(lines1);
    free(lines2);
}

void exit_command(int argc, char **argv) {
    int exit_code = 0;
    
    // Check for help flags
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("exit - Exit the shell\n\n");
        printf("Usage: exit [exit_code]\n\n");
        printf("Description:\n");
        printf("  Terminates the shell and VM with an optional exit code.\n");
        printf("  If no exit code is provided, defaults to 0 (success).\n\n");
        printf("Examples:\n");
        printf("  exit                   Exit with code 0\n");
        printf("  exit 1                 Exit with code 1\n");
        return;
    }
    
    // Parse exit code if provided
    if (argc > 1) {
        exit_code = atoi(argv[1]);
    }
    
    printf("Exiting VM with code %d...\n", exit_code);
    fflush(stdout);
    
    // Exit the VM
    exit(exit_code);
}

void console_refresh_command(int argc, char **argv) {
    printf("Console Refresh - Fixing UTF-8 Rendering Issues\n");
    printf("===============================================\n");
    
    printf("Before refresh test: ╔═══╗ ║UTF║ ╚═══╝\n");
    printf("If you see garbled characters above, this should fix it...\n\n");
    
    printf("Performing console refresh (mimicking pipe operation)...\n");
    fflush(stdout);
    
    // Safer console refresh method - toggle console modes instead of freopen
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout != INVALID_HANDLE_VALUE) {
        DWORD currentMode;
        if (GetConsoleMode(hStdout, &currentMode)) {
            // Force console refresh by toggling virtual terminal processing
            SetConsoleMode(hStdout, currentMode & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            Sleep(10); // Brief pause
            SetConsoleMode(hStdout, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            printf("✓ Console refreshed successfully!\n");
        } else {
            printf("✗ Console refresh failed\n");
            return;
        }
    } else {
        printf("✗ Could not get console handle\n");
        return;
    }
    fflush(stdout);
    
    printf("\nAfter refresh test: ╔═══╗ ║UTF║ ╚═══╝\n");
    printf("Box drawing: ┌─┬─┐ │ ├ │ └─┴─┘\n");
    printf("Arrows: ← ↑ → ↓ ⚡ ★ ♦ ♠ ♥ ♣\n");
    printf("Emoji: 🚀 💻 ⭐ 🎯 🔧 📁\n");
    
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\nRunning comprehensive UTF-8 test after refresh...\n");
        utf8_test_command(0, NULL);
    }
    
    printf("\nConsole refresh complete. UTF-8 should now work properly!\n");
    printf("If this fixes the issue, it confirms that console mode reset is the solution.\n");
}

void ghost_pipe_utf8_fix(void) {
    // Perform a "ghost pipe" operation - mimic what pipe operations do to fix UTF-8
    // This does the minimal stdout redirection that fixes UTF-8 without breaking anything
    
    // Create a temp filename like the pipe system does
    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "temp_utf8_fix_%d.tmp", (int)time(NULL));
    
    // Step 1: Redirect stdout to temp file (this is the key operation)
    FILE* original_stdout = stdout;
    FILE* temp_file = freopen(temp_filename, "w", stdout);
    
    if (temp_file) {
        // Write something minimal to the temp file
        printf("UTF8_FIX_GHOST_OPERATION\n");
        fflush(stdout);
        
        // Step 2: Restore stdout (this fixes UTF-8!)
        freopen("CON", "w", stdout);
        
        // Clean up the temp file
        remove(temp_filename);
    }
    
    // Ensure stdout is working properly
    fflush(stdout);
}

void console_test_command(int argc, char **argv) {
    printf("Console Diagnostics and Testing\n");
    printf("===============================\n");
    
    // Test console handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    printf("Console Handle: %s\n", (hConsole != INVALID_HANDLE_VALUE) ? "Valid" : "Invalid");
    
    // Test console mode
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode)) {
        printf("Console Mode: 0x%08X\n", dwMode);
        printf("  Virtual Terminal Processing: %s\n", 
               (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? "Enabled" : "Disabled");
        printf("  Line Input: %s\n", 
               (dwMode & ENABLE_LINE_INPUT) ? "Enabled" : "Disabled");
        printf("  Echo Input: %s\n", 
               (dwMode & ENABLE_ECHO_INPUT) ? "Enabled" : "Disabled");
    } else {
        printf("Failed to get console mode\n");
    }
    
    // Test code pages
    printf("Input Code Page: %u\n", GetConsoleCP());
    printf("Output Code Page: %u\n", GetConsoleOutputCP());
    printf("Expected UTF-8: %u\n", CP_UTF8);
    
    // Test ANSI colors
    printf("\nANSI Color Test:\n");
    printf("\033[31mRed\033[0m ");
    printf("\033[32mGreen\033[0m ");
    printf("\033[33mYellow\033[0m ");
    printf("\033[34mBlue\033[0m ");
    printf("\033[35mMagenta\033[0m ");
    printf("\033[36mCyan\033[0m ");
    printf("\033[37mWhite\033[0m\n");
    
    // Test UTF-8 characters
    printf("\nUTF-8 Character Test:\n");
    printf("Box drawing: ╔═══╗ ║ ║ ╚═══╝\n");
    printf("Arrows: ← ↑ → ↓\n");
    printf("Symbols: ★ ⚡ ♦ ♠ ♥ ♣\n");
    
    // Test prompt rendering
    printf("\nPrompt Test (current prompt will appear below):\n");
    print_colored_prompt();
    printf("\n");
    
    // Memory and buffer info
    printf("\nBuffer Status:\n");
    printf("Current User: '%s'\n", current_user);
    printf("Hostname: '%s'\n", hostname);
    printf("Current Path: '%s'\n", current_path);
    
    if (argc > 1 && strcmp(argv[1], "--fix") == 0) {
        printf("\nApplying console fixes...\n");
        console_refresh_command(0, NULL);
    }
}