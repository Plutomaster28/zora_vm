// filepath: c:\Users\theni\OneDrive\Documents\zora_vm\src\binary\linux\elf_parser_linux.c
#include "binary/elf_parser.h"
#include "binary/binary_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// Linux-specific ELF implementation
static ThreadHandle current_elf_thread = 0;
static int linux_binary_executor_initialized = 0;

// Binary executor functions for Linux
int binary_executor_init_linux(void) {
    printf("Initializing Linux binary executor...\n");
    linux_binary_executor_initialized = 1;
    return 0;
}

void binary_executor_cleanup_linux(void) {
    printf("Cleaning up Linux binary executor...\n");
    linux_binary_executor_initialized = 0;
}

int binary_executor_is_initialized_linux(void) {
    return linux_binary_executor_initialized;
}

int binary_executor_has_elf_support_linux(void) {
    return 1; // Linux has native ELF support
}

BinaryType detect_binary_type_linux(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return BINARY_TYPE_UNKNOWN;
    
    unsigned char header[4];
    if (fread(header, 1, 4, f) != 4) {
        fclose(f);
        return BINARY_TYPE_UNKNOWN;
    }
    fclose(f);
    
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

int execute_sandboxed_binary_linux(const char* path, char** argv, int argc) {
    printf("Linux: Executing sandboxed binary: %s\n", path);
    
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

int execute_windows_binary_linux(const char* path, char** argv, int argc) {
    printf("Linux: Cannot execute Windows binary natively: %s\n", path);
    printf("Note: Windows binary execution requires Windows host or Wine\n");
    return -1;
}

int execute_linux_binary_linux(const char* path, char** argv, int argc) {
    printf("Linux: Executing native Linux binary: %s\n", path);
    
    // Try to execute using fork/exec
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execv(path, argv);
        printf("Failed to execute: %s\n", path);
        exit(-1);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        printf("Failed to fork process\n");
        return -1;
    }
}

// ELF thread functions
ThreadReturn THREAD_CALL elf_thread_entry(ThreadParam param) {
    ElfContext* ctx = (ElfContext*)param;
    
    printf("Linux ELF thread entry: %s\n", ctx->filename);
    
    // Set up signal handler for syscall interception
    signal(SIGILL, elf_signal_handler);
    
    // Execute the ELF binary (simplified for now)
    printf("Simulating ELF execution: %s\n", ctx->filename);
    
    return NULL;
}

ThreadReturn THREAD_CALL elf_sandboxed_thread_entry(ThreadParam param) {
    ElfContext* ctx = (ElfContext*)param;
    
    printf("Linux sandboxed ELF thread entry: %s\n", ctx->filename);
    
    // Set up signal handler for syscall interception
    signal(SIGILL, elf_sandboxed_signal_handler);
    
    // Execute the ELF binary in sandbox (simplified for now)
    printf("Simulating sandboxed ELF execution: %s\n", ctx->filename);
    
    return NULL;
}

void elf_signal_handler(int sig) {
    printf("ELF signal handler: signal %d\n", sig);
    // Handle syscall interception here
}

void elf_sandboxed_signal_handler(int sig) {
    printf("ELF sandboxed signal handler: signal %d\n", sig);
    // Handle sandboxed syscall interception here
}

long handle_syscall(SyscallNum syscall_num, SyscallContext context) {
    printf("Linux syscall: %llu\n", (unsigned long long)syscall_num);
    // Implement Linux syscall handling
    return 0;
}

long handle_sandboxed_syscall(SyscallNum syscall_num, SyscallContext context) {
    printf("Linux sandboxed syscall: %llu\n", (unsigned long long)syscall_num);
    // Implement Linux sandboxed syscall handling
    return 0;
}