// Create include/binary/binary_executor.h

#ifndef BINARY_EXECUTOR_H
#define BINARY_EXECUTOR_H

#include <stddef.h>
#include <stdbool.h>
#include "platform/platform.h"

// Binary types - FIX: Use consistent naming
typedef enum {
    BINARY_UNKNOWN,
    BINARY_ELF,
    BINARY_PE, 
    BINARY_SCRIPT,
    // Add aliases for compatibility
    BINARY_TYPE_UNKNOWN = BINARY_UNKNOWN,
    BINARY_TYPE_LINUX_ELF = BINARY_ELF,
    BINARY_TYPE_WINDOWS_PE = BINARY_PE,
    BINARY_TYPE_SCRIPT = BINARY_SCRIPT
} BinaryType;

// Binary context
typedef struct {
    char* filename;
    int is_loaded;
    void* base_address;
    size_t size;
} BinaryContext;

// Core functions
int binary_executor_init(void);
void binary_executor_cleanup(void);
int binary_executor_is_initialized(void);
int binary_executor_has_elf_support(void);  // Add this missing declaration

// Binary execution
BinaryContext* binary_executor_load(const char* filename);
int binary_executor_execute(BinaryContext* ctx, char** argv, int argc);
int binary_executor_execute_sandboxed(BinaryContext* ctx, char** argv, int argc);
void binary_executor_free_context(BinaryContext* ctx);

// Shell interface functions
int execute_sandboxed_binary(const char* path, char** argv, int argc);
int execute_windows_binary(const char* path, char** argv, int argc);
int execute_linux_binary(const char* path, char** argv, int argc);
BinaryType detect_binary_type(const char* filename);

// Platform-specific function declarations
#ifdef PLATFORM_WINDOWS
int binary_executor_init_windows(void);
void binary_executor_cleanup_windows(void);
int binary_executor_is_initialized_windows(void);
int binary_executor_has_elf_support_windows(void);
BinaryType detect_binary_type_windows(const char* filename);
int execute_sandboxed_binary_windows(const char* path, char** argv, int argc);
int execute_windows_binary_windows(const char* path, char** argv, int argc);
int execute_linux_binary_windows(const char* path, char** argv, int argc);
#else
int binary_executor_init_linux(void);
void binary_executor_cleanup_linux(void);
int binary_executor_is_initialized_linux(void);
int binary_executor_has_elf_support_linux(void);
BinaryType detect_binary_type_linux(const char* filename);
int execute_sandboxed_binary_linux(const char* path, char** argv, int argc);
int execute_windows_binary_linux(const char* path, char** argv, int argc);
int execute_linux_binary_linux(const char* path, char** argv, int argc);
#endif

#endif // BINARY_EXECUTOR_H