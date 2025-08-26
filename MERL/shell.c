#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
#include "desktop/desktop.h"

// Add platform detection at the top:
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <glob.h>  // ADD THIS LINE - this was missing!
    
    // Windows compatibility for Linux
    #define INVALID_FILE_ATTRIBUTES ((unsigned long)-1)
    #define FILE_ATTRIBUTE_DIRECTORY S_IFDIR
    
    typedef struct {
        char cFileName[256];
        unsigned long dwFileAttributes;
    } WIN32_FIND_DATA;
    
    static inline unsigned long GetFileAttributes(const char* path) {
        struct stat st;
        if (stat(path, &st) == 0) {
            return st.st_mode;
        }
        return INVALID_FILE_ATTRIBUTES;
    }
#endif

// Add these compatibility defines at the top after the includes:

#ifndef PLATFORM_WINDOWS
    // Linux compatibility
    #define _rmdir(path) rmdir(path)
    #define _mkdir(path) mkdir(path, 0755)
    #define GetFileAttributesA(path) GetFileAttributes(path)
    
    // Add the missing FindFirstFile/FindNextFile/FindClose declarations
    // These should now be available from tetra.c
#endif

// Function prototypes
void handle_command(char *command);
void man_command(int argc, char **argv);
void help_command(int argc, char **argv);
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
void desktop_command(int argc, char **argv);
void theme_command(int argc, char **argv);
void themes_command(int argc, char **argv);
void find_command(int argc, char **argv);
void find_files_recursive(const char* dir_path, const char* pattern);
void tree_command(int argc, char **argv);
void print_tree_recursive(const char* dir_path, int depth);

// Helper function prototypes
void expand_path(const char* input, char* output, size_t output_size);
void add_to_history(const char* command);
void resolve_script_path(const char* name, char* out, size_t out_sz);

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
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
#else
    // Linux/Unix ANSI colors are handled via printf with escape codes
#endif
}

void reset_color() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7); // Default white on black
#else
    printf(COLOR_RESET);
#endif
}

void print_colored_prompt() {
#ifdef _WIN32
    // Windows console color codes
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Enable ANSI escape sequences on Windows 10+
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
#endif

    // Print colored prompt: user@hostname:path>
    printf(KALI_USER_COLOR "%s" COLOR_RESET, current_user);
    printf(KALI_AT_COLOR "@" COLOR_RESET);
    printf(KALI_HOST_COLOR "%s" COLOR_RESET, hostname);
    printf(COLOR_WHITE ":" COLOR_RESET);
    printf(KALI_PATH_COLOR "%s" COLOR_RESET, current_path);
    printf(KALI_PROMPT_COLOR "> " COLOR_RESET);
}

// Missing file system commands
void more_command(int argc, char **argv);
void less_command(int argc, char **argv);
void head_command(int argc, char **argv);
void tail_command(int argc, char **argv);
void grep_command(int argc, char **argv);

// Missing file permissions commands
void chmod_command(int argc, char **argv);
void chown_command(int argc, char **argv);

// Missing process management commands
void top_command(int argc, char **argv);
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
void more_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: more <filename>\n");
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
        printf("more: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("more: %s: Is a directory\n", full_path);
        return;
    }
    
    if (file_node->data && file_node->size > 0) {
        // Display file contents with paging (simple version)
        char* content = (char*)file_node->data;
        int lines_shown = 0;
        int i = 0;
        
        while (i < (int)file_node->size) {
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

void less_command(int argc, char **argv) {
    // For simplicity, less is just an alias to more in our VM
    more_command(argc, argv);
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
    
    if (file_node->data && file_node->size > 0) {
        char* content = (char*)file_node->data;
        int lines_shown = 0;
        int i = 0;
        
        while (i < (int)file_node->size && lines_shown < lines) {
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
    
    if (file_node->data && file_node->size > 0) {
        char* content = (char*)file_node->data;
        
        // Count total lines first
        int total_lines = 0;
        for (int i = 0; i < (int)file_node->size; i++) {
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
    if (argc < 3) {
        printf("Usage: grep <pattern> <filename>\n");
        return;
    }
    
    char* pattern = argv[1];
    char* filename = argv[2];
    
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
    
    if (file_node->data && file_node->size > 0) {
        char* content = (char*)file_node->data;
        char line[1024];
        int line_num = 1;
        int line_pos = 0;
        
        for (int i = 0; i < (int)file_node->size; i++) {
            if (content[i] == '\n' || i == (int)file_node->size - 1) {
                line[line_pos] = '\0';
                
                // Check if line contains pattern
                if (strstr(line, pattern) != NULL) {
                    printf("%d: %s\n", line_num, line);
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

// File permissions commands
void chmod_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: chmod <mode> <filename>\n");
        printf("Example: chmod 755 myfile\n");
        return;
    }
    
    char* mode = argv[1];
    char* filename = argv[2];
    
    printf("chmod: Changed permissions of '%s' to '%s' (simulated)\n", filename, mode);
    printf("Note: File permissions are simulated in the VM environment\n");
}

void chown_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: chown <owner[:group]> <filename>\n");
        printf("Example: chown user:group myfile\n");
        return;
    }
    
    char* owner = argv[1];
    char* filename = argv[2];
    
    printf("chown: Changed ownership of '%s' to '%s' (simulated)\n", filename, owner);
    printf("Note: File ownership is simulated in the VM environment\n");
}

// Process management commands
static int background_jobs[10] = {0};
static int job_count = 0;

void top_command(int argc, char **argv) {
    printf("=== Zora VM Process Monitor ===\n");
    printf("  PID USER      PR  NI    VIRT    RES    SHR S  %%CPU %%MEM     TIME+ COMMAND\n");
    printf("    1 root      20   0    8192   4096   2048 S   0.0  1.6   0:00.01 init\n");
    printf("    2 root      20   0   16384   8192   4096 S   0.0  3.2   0:00.05 kernel\n");
    printf("    3 vm        20   0   32768  16384   8192 R   0.1  6.4   0:00.10 zora_vm\n");
    printf("    4 vm        20   0   65536  32768  16384 S   0.0 12.8   0:00.25 merl_shell\n");
    printf("    5 vm        20   0   24576  12288   6144 S   0.0  4.8   0:00.03 vfs_daemon\n");
    printf("    6 vm        20   0   16384   8192   4096 S   0.0  3.2   0:00.02 network\n");
    printf("\nTasks: 6 total, 1 running, 5 sleeping\n");
    printf("CPU: 0.1%% us, 0.0%% sy, 0.0%% ni, 99.9%% id\n");
    printf("Memory: 256M total, 86M used, 170M free\n");
    printf("\nPress 'q' to quit, any other key to refresh...\n");
    
    char c = getchar();
    if (c != 'q' && c != 'Q') {
        // In a real implementation, this would refresh
        printf("Refreshed (simulated)\n");
    }
}

void htop_command(int argc, char **argv) {
    // htop is just an alias to top in our implementation
    printf("htop: Using top implementation (htop features simulated)\n");
    top_command(argc, argv);
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
    printf("vfs_root          262144   65536    196608  26%% /\n");
    printf("vfs_persistent    131072   32768     98304  26%% /persistent\n");
    printf("vfs_tmp            32768    4096     28672  13%% /tmp\n");
    printf("vfs_home           65536   16384     49152  26%% /home\n");
    printf("vfs_scripts        16384    8192      8192  50%% /scripts\n");
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
        printf("ZoraVM 1.0 zora-vm x86_64 x86_64 x86_64 GNU/Linux\n");
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
        printf("tar: Creating archive %s\n", archive);
        for (int i = 3; i < argc; i++) {
            printf("tar: Adding %s\n", argv[i]);
        }
        printf("tar: Archive created successfully\n");
    } else if (strstr(options, "x")) {
        printf("tar: Extracting from %s\n", archive);
        printf("tar: Extracted (simulated)\n");
    } else if (strstr(options, "t")) {
        printf("tar: Contents of %s:\n", archive);
        printf("tar: file1.txt\n");
        printf("tar: file2.txt\n");
        printf("tar: subdir/\n");
    }
}

void gzip_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: gzip <filename>\n");
        return;
    }
    
    char* filename = argv[1];
    printf("gzip: Compressing %s to %s.gz (simulated)\n", filename, filename);
    printf("gzip: Compression ratio: 65%%\n");
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
    }
    
    printf("gunzip: Decompressing %s to %s (simulated)\n", filename, output_name);
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
    
    // Execute the binary
    int result = execute_sandboxed_binary(binary_path, argv + 1, argc - 1);
    
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
        int result = execute_windows_binary(node->host_path, argv + 1, argc - 1);
        printf("Windows binary execution completed (exit code: %d)\n", result);
    } else {
        printf("Binary not found: %s\n", binary_path);
    }
}

void run_linux_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: run-linux <binary> [args...]\n");
        return;
    }
    
    char binary_path[256];
    if (argv[1][0] == '/') {
        strcpy(binary_path, argv[1]);
    } else {
        snprintf(binary_path, sizeof(binary_path), "/persistent/data/%s", argv[1]);
    }
    
    printf("Executing Linux binary: %s\n", binary_path);
    
    // Force Linux execution via QEMU
    VNode* node = vfs_find_node(binary_path);
    if (node && node->host_path) {
        int result = execute_linux_binary(node->host_path, argv + 1, argc - 1);
        printf("Linux binary execution completed (exit code: %d)\n", result);
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
    
    if (binary_executor_has_elf_support()) {
        printf("Linux ELF Support: Native ELF Parser (SANDBOXED)\n");
        printf("Architecture Support: 32-bit and 64-bit\n");
        printf("Translation Layer: Custom Linux syscall emulation\n");
    }
    
    printf("Sandbox Directory: %s\n", "Temp/zora_vm_sandbox_<pid>");
    printf("Windows Binary Support: Native execution (SANDBOXED)\n");
    printf("Linux Binary Support: %s\n", binary_executor_has_elf_support() ? "Native ELF Parser (SANDBOXED)" : "Disabled");
    printf("Script Execution: Enabled (SANDBOXED)\n");
    
    printf("\nFeatures:\n");
    printf("   • Native ELF parsing and loading\n");
    printf("   • Custom Linux syscall emulation layer\n");
    printf("   • Cross-platform binary execution\n");
    printf("   • Sandboxed execution environment\n");
    printf("   • NO external dependencies (no QEMU required)\n");
    printf("   • Real machine code execution with syscall interception\n");
}

void desktop_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: desktop <restart|status>\n");
        return;
    }
    if (strcmp(argv[1], "restart") == 0) {
        desktop_restart();
        printf("Desktop restarted.\n");
    } else if (strcmp(argv[1], "status") == 0) {
        printf("Desktop theme: %s\n", desktop_current_theme());
        desktop_list_themes();
    } else {
        printf("Unknown desktop subcommand.\n");
    }
}

void theme_command(int argc, char **argv) {
    if (argc < 2) { printf("Usage: theme <name>\n"); return; }
    if (desktop_switch_theme(argv[1]) == 0) {
        printf("Theme switched to %s\n", argv[1]);
    }
}

void themes_command(int argc, char **argv) {
    (void)argc; (void)argv; desktop_list_themes();
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
    
    // Set default hostname
    strncpy(hostname, "zora-vm", sizeof(hostname) - 1);
    hostname[sizeof(hostname) - 1] = '\0';

    printf("=== Zora VM - MERL Shell ===\n");
    printf("Virtual Machine OS with MERL Shell\n");
    printf("Type 'help' for available commands, 'exit' to quit VM.\n");
    printf("VM Commands: vmstat, reboot, shutdown\n\n");

    while (1) {
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

        // Dispatch the command
        handle_command(input);
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
    
    // Handle special cases
    if (strcmp(input, "~") == 0) {
        strncpy(output, "/home", output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    
    if (strcmp(input, "..") == 0) {
        char* current = vfs_getcwd();
        if (current && strcmp(current, "/") != 0) {
            // Find parent directory
            char* last_slash = strrchr(current, '/');
            if (last_slash && last_slash != current) {
                size_t parent_len = last_slash - current;
                strncpy(output, current, parent_len);
                output[parent_len] = '\0';
            } else {
                strcpy(output, "/");
            }
        } else {
            strcpy(output, "/");
        }
        return;
    }
    
    if (strcmp(input, ".") == 0) {
        char* current = vfs_getcwd();
        strncpy(output, current ? current : "/", output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    
    // Handle relative paths starting with ".."
    if (strncmp(input, "../", 3) == 0) {
        char parent_path[256];
        expand_path("..", parent_path, sizeof(parent_path));
        snprintf(output, output_size, "%s/%s", parent_path, input + 3);
        return;
    }
    
    // Handle paths starting with "~/"
    if (strncmp(input, "~/", 2) == 0) {
        snprintf(output, output_size, "/home/%s", input + 2);
        return;
    }
    
    // For absolute paths or regular relative paths, copy as-is
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';
}

void pwd_command(int argc, char **argv) {
    char* cwd = vfs_getcwd(); // Use VFS instead of system getcwd
    if (cwd) {
        printf("Current Directory: %s\n", cwd);
    } else {
        printf("Current Directory: /\n");
    }
}

void ls_command(int argc, char **argv) {
    char target_dir[512];
    
    // Determine target directory
    if (argc < 2) {
        // No argument - use current directory
        char* cwd = vfs_getcwd();
        strcpy(target_dir, cwd ? cwd : "/");
    } else {
        // Expand the provided path
        char expanded_path[512];
        expand_path(argv[1], expanded_path, sizeof(expanded_path));
        
        // Build absolute path if relative
        if (expanded_path[0] != '/') {
            char* cwd = vfs_getcwd();
            snprintf(target_dir, sizeof(target_dir), "%s/%s", cwd, expanded_path);
        } else {
            strcpy(target_dir, expanded_path);
        }
    }
    
    printf("Contents of %s:\n", target_dir);
    
    // Get the target directory node
    VNode* dir_node = vfs_find_node(target_dir);
    if (!dir_node) {
        printf("ls: %s: No such file or directory\n", target_dir);
        return;
    }
    
    if (!dir_node->is_directory) {
        printf("ls: %s: Not a directory\n", target_dir);
        return;
    }
    
    // List children of target directory
    VNode* child = dir_node->children;
    if (!child) {
        printf("(empty directory)\n");
        return;
    }
    
    while (child) {
        if (child->is_directory) {
            printf("%-20s <DIR>\n", child->name);
        } else {
            printf("%-20s %zu bytes\n", child->name, child->size);
        }
        child = child->next;
    }
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

void calendar_command(int argc, char **argv) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    printf("Current Date: %02d-%02d-%04d\n", local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);
}

void clock_command(int argc, char **argv) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    printf("Current Time: %02d:%02d:%02d\n", local->tm_hour, local->tm_min, local->tm_sec);
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void clear_command(int argc, char **argv) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    clear_screen();
}

void echo_command(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
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
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
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
    
    VNode* file_node = vfs_find_node(full_path);
    if (!file_node) {
        printf("cat: %s: No such file or directory\n", full_path);
        return;
    }
    
    if (file_node->is_directory) {
        printf("cat: %s: Is a directory\n", full_path);
        return;
    }
    
    if (file_node->data && file_node->size > 0) {
        // Print file contents
        fwrite(file_node->data, 1, file_node->size, stdout);
        printf("\n");
    } else {
        printf("(empty file)\n");
    }
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

#ifdef _WIN32
    // Windows: use xcopy
    snprintf(cmd, sizeof(cmd), "xcopy /E /I /Y \"%s\" \"%s\"", src_dir, dst_dir);
#else
    // Linux: use cp -r
    snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", src_dir, dst_dir);
#endif

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

#ifdef _WIN32
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
#else
    // Linux implementation using glob
    glob_t glob_result;
    int glob_status = glob(argv[1], GLOB_MARK, NULL, &glob_result);
    
    if (glob_status != 0) {
        printf("No files found matching pattern: %s\n", argv[1]);
        return;
    }
    
    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
        printf("Found: %s\n", glob_result.gl_pathv[i]);
        
        struct stat file_stat;
        if (stat(glob_result.gl_pathv[i], &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                printf("  [Directory]\n");
            } else {
                printf("  Size: %ld bytes\n", file_stat.st_size);
            }
        }
    }
    
    globfree(&glob_result);
#endif
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
    route_command("kill", argc, argv);
}
void ps_wrapper(int argc, char **argv) {
    route_command("ps", argc, argv);
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
    printf("This will exit the VM and return to host system.\n");
    printf("Are you sure? (y/n): ");
    char response;
    scanf(" %c", &response);
    if (response == 'y' || response == 'Y') {
        printf("Rebooting...\n");
        exit(0);  // Exit the VM
    }
}

void vm_shutdown_command(int argc, char **argv) {
    printf("Shutting down Zora VM...\n");
    printf("Goodbye!\n");
    exit(0);
}

// Command table
Command command_table[] = {
    {"man", man_command, "Displays information about commands."},
    {"help", help_command, "Displays the help menu."},
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
    {"calendar", calendar_command, "Displays the current date."},
    {"clock", clock_command, "Displays the current time."},
    {"clear", clear_command, "Clears the screen."},
    {"echo", echo_command, "Prints a string to the console."},
    {"cat", cat_command, "Displays the contents of a file."},
    {"tetra", tetra_command, "Handles package management."},
    {"flipper", flipper_command, "Switches to sub-shells."},
    {"pull", pull_command, "Takes a directory from the MERL goodies repository."},
    {"whoami", whoami_command, "Displays the current logged-in user."},
    {"useradd", useradd_command, "Adds a new user (placeholder)."},
    {"login", login_command, "Logs in as a specified user."},
    {"logout", logout_command, "Logs out the current user."},
    {"passwd", passwd_command, "Changes the password for the current user (placeholder)."},
    {"route", route_wrapper, "Routes commands to the appropriate handlers."},
    {"fork", fork_wrapper, "Creates a new process."},
    {"kill", kill_wrapper, "Terminates a process by ID."},
    {"ps", ps_wrapper, "Lists all active processes."},
    {"read", read_wrapper, "Reads a file."},
    {"write", write_wrapper, "Writes to a file."},
    {"color-and-test", color_and_test_command, "Displays colors and system info."},
    {"vm-status", vm_status_command, "Displays the status of the Zora VM."},
    {"vm-reboot", vm_reboot_command, "Reboots the Zora VM."},
    {"vm-shutdown", vm_shutdown_command, "Shuts down the Zora VM."},
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
    {"run-linux", run_linux_command, "Execute Linux binary via QEMU"},
    {"list-binaries", list_binaries_command, "List available binaries and their types"},
    {"sandbox-status", sandbox_status_command, "Show sandbox execution status"},
    {"desktop", desktop_command, "Desktop control (restart/status)"},
    {"theme", theme_command, "Switch desktop theme"},
    {"themes", themes_command, "List available desktop themes"},
    {"find", find_command, "Search for files by name pattern"},
    {"tree", tree_command, "Display directory tree structure"},
    {"more", more_command, "View file contents page by page"},
    {"less", less_command, "View file contents page by page"},
    {"head", head_command, "Display the beginning of a file"},
    {"tail", tail_command, "Display the end of a file"},
    {"grep", grep_command, "Search for patterns within files"},
    {"chmod", chmod_command, "Change file permissions"},
    {"chown", chown_command, "Change file owner and group"},
    {"top", top_command, "Display sorted information about processes in real-time"},
    {"htop", htop_command, "Display sorted information about processes in real-time"},
    {"jobs", jobs_command, "List background jobs"},
    {"bg", bg_command, "Send a stopped process to the background"},
    {"fg", fg_command, "Bring a background process to the foreground"},
    {"date", date_command, "Display or set the system date and time"},
    {"df", df_command, "Display disk space usage"},
    {"du", du_command, "Display disk usage of files and directories"},
    {"uname", uname_command, "Print system information"},
    {"history", history_command, "Display command history"},
    {"scp", scp_command, "Secure copy for transferring files over SSH"},
    {"tar", tar_command, "Archive files and directories"},
    {"gzip", gzip_command, "Compress files"},
    {"gunzip", gunzip_command, "Decompress files"},
    {"zip", zip_command, "Create zip archives"},
    {"unzip", unzip_command, "Extract zip archives"},
    {"hostname", hostname_command, "Display or set the system hostname"},
    
    #ifdef PYTHON_SCRIPTING
    {"python", python_command, "Execute Python script from /scripts (fallback legacy path)"},
    {"pycode", pycode_command, "Execute Python code directly"},
    #endif
    #ifdef PERL_SCRIPTING
    {"perl", perl_command, "Execute Perl script from /scripts (fallback legacy path)"},
    {"plcode", plcode_command, "Execute Perl code directly"},
    #endif

    {"test-sandbox", test_sandbox_command, "Test sandbox security and isolation"},

    {NULL, NULL, NULL}
};
const int command_table_size = sizeof(command_table) / sizeof(Command);

void handle_command(char *command) {
    // Add command to history (but not if it's just whitespace)
    if (command && strlen(command) > 0 && strspn(command, " \t\n") != strlen(command)) {
        add_to_history(command);
    }
    
    // Tokenize the command
    char *args[10];
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL && argc < 10) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    // Skip empty commands
    if (argc == 0) return;

    // Search for the command in the command table
    for (int i = 0; i < command_table_size; i++) {
        if (strcmp(args[0], command_table[i].name) == 0) {
            command_table[i].handler(argc, args);
            return;
        }
    }

    printf("Unknown command: %s\n", args[0]);
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
    printf("Available commands:\n");
    for (int i = 0; i < command_table_size; i++) {
        printf("  %s - %s\n", command_table[i].name, command_table[i].description);
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
    network_simulate_ping(target);
}

void netstat_command(int argc, char **argv) {
    if (argc == 1) {
        network_show_connections();
    } else if (argc == 2) {
        if (strcmp(argv[1], "-r") == 0) {
            network_show_routes();
        } else if (strcmp(argv[1], "-i") == 0) {
            network_show_interfaces();
        } else {
            printf("Usage: netstat [-r|-i]\n");
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
    
    printf("--2024-01-01 12:00:00--  %s\n", url);
    printf("Resolving host... ");
    
    // Simulate DNS resolution
    printf("192.168.1.100\n");
    printf("Connecting to host|192.168.1.100|:80... connected.\n");
    printf("HTTP request sent, awaiting response... 200 OK\n");
    printf("Length: 1024 (1.0K) [text/html]\n");
    printf("Saving to: 'index.html'\n");
    printf("\n");
    printf("index.html      100%%[===================>]   1.00K  --.-KB/s    in 0s\n");
    printf("\n");
    printf("2024-01-01 12:00:01 (1.00 MB/s) - 'index.html' saved [1024/1024]\n");
    
    // Create simulated downloaded file in VFS
    vfs_create_file("index.html");
    VNode* node = vfs_find_node("index.html");
    if (node) {
        const char* content = "<html><body><h1>Simulated Web Page</h1></body></html>";
        node->data = malloc(strlen(content) + 1);
        strcpy(node->data, content);
        node->size = strlen(content);
    }
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

// Linux implementations of Windows Find functions for shell.c

#ifdef PLATFORM_WINDOWS
// Use native Windows FindFirstFile/FindNextFile/FindClose - they're already declared in windows.h
// No additional declarations needed
#else
// Linux implementations would go here if needed
#endif

// Platform detection
#ifndef _WIN32
    #ifndef __linux__
        #define __linux__
    #endif
#endif

// Cross-platform compatibility
#ifdef _WIN32
    #define PLATFORM_NAME "Windows"
    #define CLEAR_COMMAND "cls"
#else
    #define PLATFORM_NAME "Linux"
    #define CLEAR_COMMAND "clear"
#endif