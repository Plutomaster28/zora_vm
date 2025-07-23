#include "binary/binary_executor.h"
#include "binary/elf_parser.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes - FIX: Include as separate compilation units
#ifdef PLATFORM_WINDOWS
    // Forward declarations for Windows functions - implemented in binary_executor_win.c
    extern int binary_executor_init_windows(void);
    extern void binary_executor_cleanup_windows(void);
    extern int binary_executor_is_initialized_windows(void);
    extern int binary_executor_has_elf_support_windows(void);
    extern BinaryType detect_binary_type_windows(const char* filename);
    extern int execute_sandboxed_binary_windows(const char* path, char** argv, int argc);
    extern int execute_windows_binary_windows(const char* path, char** argv, int argc);
    extern int execute_linux_binary_windows(const char* path, char** argv, int argc);
#else
    // Forward declarations for Linux functions - implemented in elf_parser_linux.c
    extern int binary_executor_init_linux(void);
    extern void binary_executor_cleanup_linux(void);
    extern int binary_executor_is_initialized_linux(void);
    extern int binary_executor_has_elf_support_linux(void);
    extern BinaryType detect_binary_type_linux(const char* filename);
    extern int execute_sandboxed_binary_linux(const char* path, char** argv, int argc);
    extern int execute_windows_binary_linux(const char* path, char** argv, int argc);
    extern int execute_linux_binary_linux(const char* path, char** argv, int argc);
#endif

// Cross-platform wrapper functions
int binary_executor_init(void) {
#ifdef PLATFORM_WINDOWS
    return binary_executor_init_windows();
#else
    return binary_executor_init_linux();
#endif
}

void binary_executor_cleanup(void) {
#ifdef PLATFORM_WINDOWS
    binary_executor_cleanup_windows();
#else
    binary_executor_cleanup_linux();
#endif
}

int binary_executor_is_initialized(void) {
#ifdef PLATFORM_WINDOWS
    return binary_executor_is_initialized_windows();
#else
    return binary_executor_is_initialized_linux();
#endif
}

int binary_executor_has_elf_support(void) {
#ifdef PLATFORM_WINDOWS
    return binary_executor_has_elf_support_windows();
#else
    return binary_executor_has_elf_support_linux();
#endif
}

BinaryType detect_binary_type(const char* filename) {
#ifdef PLATFORM_WINDOWS
    return detect_binary_type_windows(filename);
#else
    return detect_binary_type_linux(filename);
#endif
}

int execute_sandboxed_binary(const char* path, char** argv, int argc) {
#ifdef PLATFORM_WINDOWS
    return execute_sandboxed_binary_windows(path, argv, argc);
#else
    return execute_sandboxed_binary_linux(path, argv, argc);
#endif
}

int execute_windows_binary(const char* path, char** argv, int argc) {
#ifdef PLATFORM_WINDOWS
    return execute_windows_binary_windows(path, argv, argc);
#else
    return execute_windows_binary_linux(path, argv, argc);
#endif
}

int execute_linux_binary(const char* path, char** argv, int argc) {
#ifdef PLATFORM_WINDOWS
    return execute_linux_binary_windows(path, argv, argc);
#else
    return execute_linux_binary_linux(path, argv, argc);
#endif
}