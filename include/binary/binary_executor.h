// Create include/binary/binary_executor.h

#ifndef BINARY_EXECUTOR_H
#define BINARY_EXECUTOR_H

#include <windows.h>

// Binary execution types
typedef enum {
    BINARY_TYPE_UNKNOWN,
    BINARY_TYPE_WINDOWS_PE,
    BINARY_TYPE_LINUX_ELF,
    BINARY_TYPE_SCRIPT
} BinaryType;

// Binary execution context
typedef struct {
    char* binary_path;
    char* working_dir;
    char** argv;
    int argc;
    char** env;
    BinaryType type;
    int sandboxed;
    HANDLE process_handle;
    DWORD process_id;
} BinaryContext;

// Function prototypes
int binary_executor_init(void);
void binary_executor_cleanup(void);
BinaryType detect_binary_type(const char* file_path);
int execute_windows_binary(const char* binary_path, char** argv, int argc);
int execute_linux_binary(const char* binary_path, char** argv, int argc);
int execute_sandboxed_binary(const char* binary_path, char** argv, int argc);
int create_sandbox_environment(const char* binary_path);
void cleanup_sandbox_environment(void);

// Status and accessor functions
int binary_executor_is_initialized(void);
const char* binary_executor_get_qemu_path(void);
int binary_executor_has_qemu(void);
const char* binary_executor_get_qemu_x86_64_path(void);
const char* binary_executor_get_qemu_i386_path(void);
const char* binary_executor_get_elf_parser_info(void);
int binary_executor_has_elf_support(void);
const char* binary_executor_get_elf_x86_64_support(void);
const char* binary_executor_get_elf_i386_support(void);

#endif // BINARY_EXECUTOR_H