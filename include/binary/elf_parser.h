// Create include/binary/elf_parser.h

#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "platform/platform.h"

// Platform-specific includes and type definitions
#include <windows.h>
#include <process.h>
typedef HANDLE ProcessHandle;
typedef HANDLE ThreadHandle;
typedef void* ThreadParam;
typedef CRITICAL_SECTION thread_mutex_t;
#define THREAD_CALL WINAPI

// Cross-platform syscall types
typedef uint64_t SyscallNum;
typedef void* SyscallContext;

// ELF header definitions
#define EI_NIDENT 16
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7

#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

// ELF structures
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

// ELF execution context
typedef struct {
    char filename[256];
    int is_loaded;
    void* base_address;
    size_t size;
    Elf64_Ehdr header;
    Elf64_Phdr* program_headers;
    ProcessHandle process_handle;
    thread_mutex_t heap_mutex;
    
    // Memory management
    void* heap_start;
    void* heap_end;
    size_t heap_size;
    
    // Thread management
    ThreadHandle main_thread;
    int thread_count;
    
    // Sandbox settings
    bool sandboxed;
    uint32_t allowed_syscalls[64];
    int syscall_count;
} ElfContext;

// Function declarations
int elf_parse_header(ElfContext* ctx, const char* filename);
int elf_load_segments(ElfContext* ctx);
int elf_execute(ElfContext* ctx, char** argv, int argc);
int elf_execute_sandboxed(ElfContext* ctx, char** argv, int argc);
void elf_cleanup(ElfContext* ctx);

// Thread entry points
ThreadReturn THREAD_CALL elf_thread_entry(ThreadParam param);
ThreadReturn THREAD_CALL elf_sandboxed_thread_entry(ThreadParam param);

// Signal handlers
void elf_signal_handler(int sig);
void elf_sandboxed_signal_handler(int sig);

// Memory management
void* elf_allocate_memory(ElfContext* ctx, size_t size);
void elf_free_memory(ElfContext* ctx, void* ptr);

// Syscall handling
long handle_syscall(SyscallNum syscall_num, SyscallContext context);
long handle_sandboxed_syscall(SyscallNum syscall_num, SyscallContext context);

// Utility functions
bool elf_is_valid_header(const Elf64_Ehdr* header);
const char* elf_get_type_string(uint16_t type);
const char* elf_get_machine_string(uint16_t machine);

// Additional utility functions
ElfContext* get_current_elf_context(void);
void elf_print_info(ElfContext* ctx);
void elf_free_context(ElfContext* ctx);
void elf_global_cleanup(void);
int elf_init(void);
void elf_set_current_context(ElfContext* ctx);
bool elf_is_initialized(void);

#endif // ELF_PARSER_H