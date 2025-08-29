#include "binary/binary_executor.h"
#include "binary/elf_parser.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Windows-specific function declarations - implemented in binary_executor_win.c
extern int binary_executor_init_windows(void);
extern void binary_executor_cleanup_windows(void);
extern int binary_executor_is_initialized_windows(void);
extern int binary_executor_has_elf_support_windows(void);
extern BinaryType detect_binary_type_windows(const char* filename);
extern int execute_sandboxed_binary_windows(const char* path, char** argv, int argc);
extern int execute_windows_binary_windows(const char* path, char** argv, int argc);
extern int execute_linux_binary_windows(const char* path, char** argv, int argc);

// Wrapper functions (Windows-only implementations)
int binary_executor_init(void) {
    return binary_executor_init_windows();
}

void binary_executor_cleanup(void) {
    binary_executor_cleanup_windows();
}

int binary_executor_is_initialized(void) {
    return binary_executor_is_initialized_windows();
}

int binary_executor_has_elf_support(void) {
    return binary_executor_has_elf_support_windows();
}

BinaryType detect_binary_type(const char* filename) {
    return detect_binary_type_windows(filename);
}

int execute_sandboxed_binary(const char* path, char** argv, int argc) {
    return execute_sandboxed_binary_windows(path, argv, argc);
}

int execute_windows_binary(const char* path, char** argv, int argc) {
    return execute_windows_binary_windows(path, argv, argc);
}

int execute_linux_binary(const char* path, char** argv, int argc) {
    return execute_linux_binary_windows(path, argv, argc);
}