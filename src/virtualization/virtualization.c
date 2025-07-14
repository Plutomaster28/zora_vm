#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtualization.h"
#include "vfs.h"
#include "syscall.h"

VirtualizationContext* g_vm_context = NULL;

int virtualization_init(void) {
    if (g_vm_context) {
        return 0; // Already initialized
    }
    
    g_vm_context = malloc(sizeof(VirtualizationContext));
    if (!g_vm_context) {
        return -1;
    }
    
    memset(g_vm_context, 0, sizeof(VirtualizationContext));
    
    // Initialize VFS
    if (vfs_init() != 0) {
        free(g_vm_context);
        g_vm_context = NULL;
        return -1;
    }
    
    // Initialize syscall interception
    if (syscall_init() != 0) {
        vfs_cleanup();
        free(g_vm_context);
        g_vm_context = NULL;
        return -1;
    }
    
    g_vm_context->initialized = 1;
    g_vm_context->vfs_context = vfs_get_instance();
    
    printf("Virtualization layer initialized\n");
    return 0;
}

void virtualization_cleanup(void) {
    if (g_vm_context) {
        syscall_cleanup();
        vfs_cleanup();
        free(g_vm_context);
        g_vm_context = NULL;
        printf("Virtualization layer cleaned up\n");
    }
}

int virtualization_is_active(void) {
    return g_vm_context && g_vm_context->initialized;
}

int vm_save_state(const char* filename) {
    printf("vm_save_state: Saving VM state to '%s'\n", filename);
    // TODO: Implement state saving
    return 0;
}

int vm_load_state(const char* filename) {
    printf("vm_load_state: Loading VM state from '%s'\n", filename);
    // TODO: Implement state loading
    return 0;
}

void vm_reset_state(void) {
    printf("vm_reset_state: Resetting VM to initial state\n");
    if (g_vm_context) {
        vfs_cleanup();
        vfs_init();
        g_vm_context->vfs_context = vfs_get_instance();
    }
}