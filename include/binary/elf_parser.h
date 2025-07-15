// Create include/binary/elf_parser.h

#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>
#include <windows.h>

// Define zora_off_t consistently
#ifdef _WIN32
typedef __int64 zora_off_t;
#else
typedef off_t zora_off_t;
#endif

#include <stdio.h>

// Fix the off_t type conflict - check if it's already defined
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long long off_t;
#endif

// ELF header constants
#define EI_NIDENT 16
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ET_EXEC 2
#define ET_DYN 3

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

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

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

// Fix the ELF loader context to match what's used in the source
typedef struct {
    char* filename;                    // Add missing filename field
    FILE* file;                        // Correct type
    void* base_addr;                   // Add missing base_addr field
    size_t total_size;                 // Add missing total_size field
    uint64_t entry_point;              // Keep this
    int is_64bit;                      // Keep this
    int is_executable;                 // Add missing is_executable field
    HANDLE process_handle;             // Add missing process_handle field
    HANDLE thread_handle;              // Add missing thread_handle field
    char* sandbox_root;                // Keep this
    HANDLE job_object;                 // Keep this
    void* heap_base;                   // Keep this
    size_t heap_size;                  // Keep this
    HANDLE heap_mutex;                 // Add missing heap_mutex field
} ElfContext;

// Function prototypes
int elf_init(void);
void elf_cleanup(void);
int elf_is_valid(const char* filename);
int elf_get_architecture(const char* filename);
ElfContext* elf_load(const char* filename);
int elf_execute(ElfContext* ctx, char** argv, int argc);
void elf_free_context(ElfContext* ctx);
int elf_parse_headers(ElfContext* ctx);
int elf_load_segments(ElfContext* ctx);
void elf_print_info(ElfContext* ctx);

// Thread and exception handling
DWORD WINAPI elf_thread_entry(LPVOID param);
LONG WINAPI elf_exception_handler(EXCEPTION_POINTERS* exception_info);
DWORD WINAPI elf_sandboxed_thread_entry(LPVOID lpParam);
LONG WINAPI elf_sandboxed_exception_handler(EXCEPTION_POINTERS* exception_info);

// Syscall handling
long handle_syscall(DWORD64 syscall_num, CONTEXT* context);
long handle_sandboxed_syscall(DWORD64 syscall_num, CONTEXT* context);

// Sandboxed execution
int elf_execute_sandboxed(ElfContext* ctx, char** argv, int argc);

// Context management
void set_current_elf_context(ElfContext* ctx);
ElfContext* get_current_elf_context(void);

// System call handlers
long sys_read(int fd, void* buf, size_t count);
long sys_write(int fd, const void* buf, size_t count);
long sys_open(const char* pathname, int flags, int mode);
long sys_close(int fd);
long sys_exit(int status);
long sys_brk(void* addr);
long sys_mmap(void* addr, size_t length, int prot, int flags, int fd, zora_off_t offset);
long sys_munmap(void* addr, size_t length);

// Sandboxed system call handlers
long sys_read_sandboxed(int fd, void* buf, size_t count);
long sys_write_sandboxed(int fd, const void* buf, size_t count);
long sys_open_sandboxed(const char* pathname, int flags, int mode);
long sys_close_sandboxed(int fd);
long sys_exit_sandboxed(int status);
long sys_brk_sandboxed(void* addr, ElfContext* ctx);
long sys_mmap_sandboxed(void* addr, size_t length, int prot, int flags, int fd, zora_off_t offset);
long sys_munmap_sandboxed(void* addr, size_t length);

#endif // ELF_PARSER_H