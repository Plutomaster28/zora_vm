#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscall.h"
#include "vfs.h"

static int syscall_initialized = 0;

int syscall_init(void) {
    if (syscall_initialized) {
        return 0;
    }
    
    printf("System call interception initialized\n");
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
    if (!command) return -1;
    
    // Parse and handle commands in virtual environment
    if (strcmp(command, "dir") == 0 || strcmp(command, "ls") == 0) {
        return vm_ls();
    }
    else if (strcmp(command, "cls") == 0 || strcmp(command, "clear") == 0) {
        return vm_clear();
    }
    else if (strcmp(command, "pwd") == 0) {
        return vm_pwd();
    }
    else if (strcmp(command, "ps") == 0) {
        return vm_ps();
    }
    else {
        printf("vm_system: Command '%s' not allowed in sandbox\n", command);
        return -1;
    }
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