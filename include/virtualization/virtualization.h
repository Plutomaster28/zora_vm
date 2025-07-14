#ifndef VIRTUALIZATION_H
#define VIRTUALIZATION_H

#include <stdint.h>
#include <stddef.h>

// Virtualization layer
typedef struct {
    int initialized;
    void* vfs_context;
    void* syscall_context;
    void* sandbox_context;
} VirtualizationContext;

// Virtualization functions
int virtualization_init(void);
void virtualization_cleanup(void);
int virtualization_is_active(void);

// VM state management
int vm_save_state(const char* filename);
int vm_load_state(const char* filename);
void vm_reset_state(void);

extern VirtualizationContext* g_vm_context;

#endif // VIRTUALIZATION_H