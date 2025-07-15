#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h> // For Windows-specific directory functions
#include <windows.h>
#include "shell.h"
#include "user.h"
#include "kernel.h"
#include "tetra.h"
#include "color-and-test.h"
#include "config.h"
#include "network/network.h" // Added include for network functions
#include "vfs/vfs.h"
#include "lua/lua_vm.h" // Include Lua VM header

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
void ping_command(int argc, char* argv[]);
void netstat_command(int argc, char* argv[]);
void nslookup_command(int argc, char* argv[]);
void telnet_command(int argc, char* argv[]);
void wget_command(int argc, char* argv[]);
void curl_command(int argc, char* argv[]);
void ssh_command(int argc, char* argv[]);
void iptables_command(int argc, char* argv[]);
void test_vfs_command(int argc, char* argv[]);
void debug_vfs_command(int argc, char* argv[]);
void lua_command(int argc, char **argv);
void luacode_command(int argc, char **argv);

#ifdef PYTHON_SCRIPTING
#include "python/python_vm.h"

void python_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: python <script.py>\n");
        return;
    }
    
    char script_path[256];
    if (argv[1][0] == '/') {
        strcpy(script_path, argv[1]);
    } else {
        snprintf(script_path, sizeof(script_path), "/persistent/scripts/%s", argv[1]);
    }
    
    printf("Executing Python script: %s\n", script_path);
    if (python_vm_load_script(script_path) != 0) {
        printf("Failed to execute Python script\n");
    }
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
    if (argv[1][0] == '/') {
        strcpy(script_path, argv[1]);
    } else {
        snprintf(script_path, sizeof(script_path), "/persistent/scripts/%s", argv[1]);
    }
    
    printf("Executing Perl script: %s\n", script_path);
    if (perl_vm_load_script(script_path) != 0) {
        printf("Failed to execute Perl script\n");
    }
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

    printf("=== Zora VM - MERL Shell ===\n");
    printf("Virtual Machine OS with MERL Shell\n");
    printf("Type 'help' for available commands, 'exit' to quit VM.\n");
    printf("VM Commands: vmstat, reboot, shutdown\n\n");

    while (1) {
        // Render the shell prompt
        printf("zora-vm:merl> ");
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

void pwd_command(int argc, char **argv) {
    char* cwd = vfs_getcwd(); // Use VFS instead of system getcwd
    if (cwd) {
        printf("Current Directory: %s\n", cwd);
    } else {
        printf("Current Directory: /\n");
    }
}

void ls_command(int argc, char **argv) {
    char* current_dir = vfs_getcwd();
    printf("Contents of %s:\n", current_dir);
    
    // Get the current directory node
    VNode* dir_node = vfs_find_node(current_dir);
    if (!dir_node) {
        printf("Error: Cannot access current directory\n");
        return;
    }
    
    if (!dir_node->is_directory) {
        printf("Error: Not a directory\n");
        return;
    }
    
    // List children of current directory
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
        printf("Usage: cd <directory>\n");
        return;
    }
    
    // Use VFS chdir instead of system chdir
    if (vfs_chdir(argv[1]) == 0) {
        printf("Changed directory to: %s\n", argv[1]);
    } else {
        printf("cd: %s: No such directory\n", argv[1]);
    }
}

void mkdir_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>\n");
        return;
    }
    if (vm_mkdir(argv[1]) == 0) { // Instead of _mkdir()
        printf("Directory created: %s\n", argv[1]);
    } else {
        printf("mkdir: Failed to create directory\n");
    }
}

void rmdir_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rmdir <directory>\n");
        return;
    }
    if (_rmdir(argv[1]) == 0) {
        printf("Directory removed: %s\n", argv[1]);
    } else {
        perror("rmdir");
    }
}

void touch_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return;
    }
    FILE *file = fopen(argv[1], "w");
    if (file) {
        fclose(file);
        printf("File created: %s\n", argv[1]);
    } else {
        perror("touch");
    }
}

void rm_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return;
    }
    if (remove(argv[1]) == 0) {
        printf("File removed: %s\n", argv[1]);
    } else {
        perror("rm");
    }
}

void cp_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: cp <source> <destination>\n");
        return;
    }
    FILE *src = fopen(argv[1], "rb");
    if (!src) {
        perror("cp");
        return;
    }
    FILE *dest = fopen(argv[2], "wb");
    if (!dest) {
        fclose(src);
        perror("cp");
        return;
    }
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    fclose(src);
    fclose(dest);
    printf("File copied from %s to %s\n", argv[1], argv[2]);
}

void mv_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mv <source> <destination>\n");
        return;
    }
    if (rename(argv[1], argv[2]) == 0) {
        printf("File moved from %s to %s\n", argv[1], argv[2]);
    } else {
        perror("mv");
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

void clear_command(int argc, char **argv) {
    vm_system("clear"); // Instead of system("cls")
}

void echo_command(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

void cat_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return;
    }
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("cat");
        return;
    }
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    fclose(file);
}

void pull_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pull <directory-name>\n");
        printf("Example: pull internet\n");
        return;
    }

    const char *goodies_repo = "https://github.com/Plutomaster28/MERL-Extended-Goodies.git";
    const char *local_repo = "__MERL_GOODIES_TEMP__";
    char cmd[1024];

    // Step 1: Clone or update the repo
    if (GetFileAttributesA(local_repo) == INVALID_FILE_ATTRIBUTES) {
        printf("[pull] Cloning goodies repository...\n");
        snprintf(cmd, sizeof(cmd), "git clone \"%s\" \"%s\"", goodies_repo, local_repo);
        if (system(cmd) != 0) {
            printf("[pull] Failed to clone repository.\n");
            return;
        }
    } else {
        printf("[pull] Updating goodies repository...\n");
        snprintf(cmd, sizeof(cmd), "cd %s && git pull", local_repo);
        if (system(cmd) != 0) {
            printf("[pull] Failed to update repository.\n");
            return;
        }
    }

    // Step 2: Search for the directory
    char src_dir[1024];
    snprintf(src_dir, sizeof(src_dir), "%s\\%s", local_repo, argv[1]);
    DWORD attrib = GetFileAttributesA(src_dir);
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        printf("[pull] Directory '%s' not found in goodies repository.\n", argv[1]);
        return;
    }

    // Ensure etc directory exists
    _mkdir("etc");

    // Step 3: Copy the directory to the etc folder
    char dst_dir[1024];
    snprintf(dst_dir, sizeof(dst_dir), ".\\etc\\%s", argv[1]);
    snprintf(cmd, sizeof(cmd), "xcopy /E /I /Y \"%s\" \"%s\"", src_dir, dst_dir);
    printf("[pull] Copying '%s' to etc directory...\n", argv[1]);
    if (system(cmd) == 0) {
        printf("[pull] Successfully pulled '%s' into etc.\n", argv[1]);
    } else {
        printf("[pull] Failed to copy directory.\n");
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
        printf("Example: search *.txt\n");
        return;
    }

    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(argv[1], &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No files found matching pattern: %s\n", argv[1]);
        return;
    }

    printf("Files matching '%s':\n", argv[1]);
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            printf("  %s\n", find_data.cFileName);
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
    {"lua", lua_command, "Execute Lua script from /persistent/scripts/"},
    {"luacode", luacode_command, "Execute Lua code directly."},
    
    #ifdef PYTHON_SCRIPTING
    {"python", python_command, "Execute Python script from /persistent/scripts/"},
    {"pycode", pycode_command, "Execute Python code directly"},
    #endif
    #ifdef PERL_SCRIPTING
    {"perl", perl_command, "Execute Perl script from /persistent/scripts/"},
    {"plcode", plcode_command, "Execute Perl code directly"},
    #endif

    {NULL, NULL, NULL}
};
const int command_table_size = sizeof(command_table) / sizeof(Command);

void handle_command(char *command) {
    // Tokenize the command
    char *args[10];
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL && argc < 10) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

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

void lua_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: lua <script.lua>\n");
        return;
    }
    
    char script_path[256];
    if (argv[1][0] == '/') {
        strcpy(script_path, argv[1]);
    } else {
        snprintf(script_path, sizeof(script_path), "/persistent/scripts/%s", argv[1]);
    }
    
    printf("Executing Lua script: %s\n", script_path);
    if (lua_vm_load_script(script_path) != 0) {
        printf("Failed to execute script\n");
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