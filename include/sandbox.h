#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdint.h>
#include <stddef.h>

// Sandbox initialization and cleanup
int sandbox_init(void);
void sandbox_cleanup(void);

// Resource limits
int sandbox_set_memory_limit(size_t bytes);
int sandbox_set_cpu_limit(int percentage);

// System call filtering
int sandbox_restrict_syscalls(void);

// Memory isolation
void* sandbox_alloc_vm_memory(size_t size);
void sandbox_free_vm_memory(void* ptr, size_t size);

#endif // SANDBOX_H