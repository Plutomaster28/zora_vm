#ifndef BINARY_EXECUTOR_WIN_INCLUDED
#define BINARY_EXECUTOR_WIN_INCLUDED

#include "binary/binary_executor.h"
#include "binary/elf_parser.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Windows-specific binary executor implementation
static int windows_binary_executor_initialized = 0;

int binary_executor_init_windows(void) {
#if ZORA_VERBOSE_BOOT
    printf("Initializing Windows binary executor...\n");
#endif
    windows_binary_executor_initialized = 1;
    return 0;
}

void binary_executor_cleanup_windows(void) {
    printf("Cleaning up Windows binary executor...\n");
    windows_binary_executor_initialized = 0;
}

int binary_executor_is_initialized_windows(void) {
    return windows_binary_executor_initialized;
}

int binary_executor_has_elf_support_windows(void) {
    return 1; // Windows can support ELF through emulation
}

BinaryType detect_binary_type_windows(const char* filename) {
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return BINARY_TYPE_UNKNOWN;
    }
    
    unsigned char header[4];
    DWORD bytes_read;
    if (!ReadFile(file, header, 4, &bytes_read, NULL) || bytes_read != 4) {
        CloseHandle(file);
        return BINARY_TYPE_UNKNOWN;
    }
    CloseHandle(file);
    
    // Check for ELF magic
    if (header[0] == 0x7F && header[1] == 'E' && header[2] == 'L' && header[3] == 'F') {
        return BINARY_TYPE_LINUX_ELF;
    }
    
    // Check for PE magic (simplified)
    if (header[0] == 'M' && header[1] == 'Z') {
        return BINARY_TYPE_WINDOWS_PE;
    }
    
    // Check for script shebang
    if (header[0] == '#' && header[1] == '!') {
        return BINARY_TYPE_SCRIPT;
    }
    
    return BINARY_TYPE_UNKNOWN;
}

int execute_sandboxed_binary_windows(const char* path, char** argv, int argc) {
    printf("Windows: Executing sandboxed binary: %s\n", path);
    
    // Create ELF context
    ElfContext* ctx = malloc(sizeof(ElfContext));
    if (!ctx) {
        printf("Failed to allocate ELF context\n");
        return -1;
    }
    
    memset(ctx, 0, sizeof(ElfContext));
    strncpy(ctx->filename, path, sizeof(ctx->filename) - 1);
    
    // Execute via ELF parser
    int result = elf_execute_sandboxed(ctx, argv, argc);
    
    free(ctx);
    return result;
}

int execute_windows_binary_windows(const char* path, char** argv, int argc) {
    printf("Windows: Executing native Windows binary: %s\n", path);
    
    // Build command line
    char cmd_line[2048] = {0};
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\"", path);
    
    for (int i = 1; i < argc; i++) {
        strcat(cmd_line, " \"");
        strcat(cmd_line, argv[i]);
        strcat(cmd_line, "\"");
    }
    
    // Execute using CreateProcess
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if (CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return (int)exit_code;
    }
    
    printf("Failed to execute Windows binary: %s\n", path);
    return -1;
}

int execute_linux_binary_windows(const char* path, char** argv, int argc) {
    printf("Windows: Executing Linux binary via emulation: %s\n", path);
    
    // Create ELF context
    ElfContext* ctx = malloc(sizeof(ElfContext));
    if (!ctx) {
        printf("Failed to allocate ELF context\n");
        return -1;
    }
    
    memset(ctx, 0, sizeof(ElfContext));
    strncpy(ctx->filename, path, sizeof(ctx->filename) - 1);
    
    // Execute via ELF parser
    int result = elf_execute(ctx, argv, argc);
    
    free(ctx);
    return result;
}

#endif // BINARY_EXECUTOR_WIN_INCLUDED