#ifndef SANDBOX_H
#define SANDBOX_H

#include <stddef.h>

// Sandbox initialization and cleanup
int sandbox_init(void);
void sandbox_cleanup(void);

// Resource limits
int sandbox_set_memory_limit(size_t limit);
int sandbox_set_cpu_limit(int percent);

// System call filtering
int sandbox_restrict_syscalls(void);

// Memory isolation
void* sandbox_alloc_vm_memory(size_t size);
void sandbox_free_vm_memory(void* ptr, size_t size);

// Strict mode control
int sandbox_set_strict_mode(int enabled);
int sandbox_is_strict_mode(void);

// Network access control
int sandbox_block_network_access(int enabled);
int sandbox_is_network_blocked(void);

// File system access control
int sandbox_block_file_system_access(int enabled);
int sandbox_is_filesystem_blocked(void);

// System call access control
int sandbox_block_system_calls(int enabled);
int sandbox_is_syscalls_blocked(void);

// Getter functions for limits
size_t sandbox_get_memory_limit(void);
int sandbox_get_cpu_limit(void);

#endif // SANDBOX_H