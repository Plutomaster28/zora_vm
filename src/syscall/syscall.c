#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscall.h"
#include "vfs/vfs.h"
#include "sandbox.h"
#include "kernel/syscall_table.h"

// Blocked system calls when sandboxed
static const char* blocked_syscalls[] = {
    "system", "exec", "fork", "clone", "ptrace", "mount", "umount",
    "chroot", "setuid", "setgid", "chmod", "chown", NULL
};

// Blocked commands when sandboxed
static const char* blocked_commands[] = {
    "whoami", "dir", "curl", "wget", "ping", "mkdir", "rmdir", 
    "copy", "move", "del", "format", "net", "ipconfig", "tasklist",
    "taskkill", "sc", "reg", "regedit", "powershell", "cmd", NULL
};

static int syscall_initialized = 0;

int syscall_init(void) {
    if (syscall_initialized) {
        return 0;
    }
    
    // Initialize syscall dispatch table
    syscall_table_init();
    
#if ZORA_VERBOSE_BOOT
    printf("System call interception initialized\n");
#endif
    syscall_initialized = 1;
    return 0;
}

void syscall_cleanup(void) {
    if (syscall_initialized) {
        printf("System call interception cleaned up\n");
        syscall_initialized = 0;
    }
}

// Intercept system calls and redirect to virtual implementations
int vm_system(const char* command) {
    // Check if sandbox is blocking system calls
    if (sandbox_is_syscalls_blocked()) {
        printf("vm_system: System call '%s' blocked by sandbox\n", command);
        return -1;
    }
    
    // Check if command is in blocked list
    for (int i = 0; blocked_commands[i]; i++) {
        if (strstr(command, blocked_commands[i]) == command) {
            printf("vm_system: Command '%s' blocked by security policy\n", command);
            return -1;
        }
    }
    
    // Check for dangerous patterns
    if (strstr(command, "C:\\") || strstr(command, "/etc/") || 
        strstr(command, "..\\") || strstr(command, "../") ||
        strstr(command, "evil.com") || strstr(command, "malware")) {
        printf("vm_system: Suspicious command blocked: %s\n", command);
        return -1;
    }
    
    // Allow specific safe commands
    if (strcmp(command, "clear") == 0) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return 0;
    }
    
    // Only allow specific VM commands
    if (strncmp(command, "vm_", 3) == 0) {
        // Handle VM-specific commands
        return 0;
    }
    
    printf("vm_system: Command '%s' not allowed in sandbox\n", command);
    return -1;
}

int vm_fclose(FILE* stream) {
    // Virtual file close - for now just return success
    return 0;
}

int vm_rename(const char* old_name, const char* new_name) {
    // Virtual rename - basic implementation
    VNode* node = vfs_find_node(old_name);
    if (!node) {
        return -1;
    }
    
    // Extract new name from path
    const char* last_slash = strrchr(new_name, '/');
    const char* name_part = last_slash ? last_slash + 1 : new_name;
    
    strncpy(node->name, name_part, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    
    return 0;
}

// Process operations (stubbed for VM)
int vm_fork(void) {
    printf("vm_fork: Process forking not supported in VM\n");
    return -1;
}

int vm_exec(const char* command) {
    printf("vm_exec: Executing '%s' in VM context\n", command);
    return vm_system(command);
}

void vm_exit(int status) {
    printf("vm_exit: VM process exiting with status %d\n", status);
    exit(status);
}