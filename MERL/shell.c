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

// Function prototypes
void handle_command(char *command);
void man_command(int argc, char **argv);
void help_command(int argc, char **argv);

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
    char* cwd = vm_getcwd(); // Instead of _getcwd()
    printf("Current Directory: %s\n", cwd);
}

void ls_command(int argc, char **argv) {
    vm_system("ls"); // Instead of system("dir")
}

void cd_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cd <directory>\n");
        return;
    }
    if (vm_chdir(argv[1]) == 0) { // Instead of _chdir()
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
    {"shutdown", vm_shutdown_command, "Shuts down the virtual machine."}
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