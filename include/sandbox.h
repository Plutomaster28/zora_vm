#ifndef SANDBOX_H
#define SANDBOX_H

#include <stddef.h>

// Sandbox initialization and cleanup
int sandbox_init(void);
void sandbox_cleanup(void);
int sandbox_is_initialized(void);

// Sandbox configuration
int sandbox_set_strict_mode(int enabled);
int sandbox_block_network_access(int enabled);
int sandbox_block_file_system_access(int enabled);
int sandbox_block_system_calls(int enabled);
int sandbox_set_memory_limit(size_t limit);
int sandbox_set_cpu_limit(int percent);

// Sandbox status queries
int sandbox_is_strict_mode(void);
int sandbox_is_network_blocked(void);
int sandbox_is_filesystem_blocked(void);
int sandbox_is_syscalls_blocked(void);
size_t sandbox_get_memory_limit(void);
int sandbox_get_cpu_limit(void);

// NEW: Add missing function declaration
int create_sandbox_environment(const char* root_path);

#endif // SANDBOX_H