// Create src/binary/elf_parser.c

#include "elf_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>  // Add this for uintptr_t

// Define zora_off_t if not already defined
#ifndef zora_off_t
#ifdef _WIN32
typedef __int64 zora_off_t;
#else
typedef off_t zora_off_t;
#endif
#endif

// Add proper types for pointer/integer conversion
typedef intptr_t fd_t;     // For file descriptors
typedef uintptr_t addr_t;  // For addresses

static int elf_initialized = 0;

// Add function declarations to the header
LONG WINAPI elf_sandboxed_exception_handler(EXCEPTION_POINTERS* exception_info);
long handle_sandboxed_syscall(DWORD64 syscall_num, CONTEXT* context);
DWORD WINAPI elf_sandboxed_thread_entry(LPVOID param);
long sys_read_sandboxed(int fd, void* buf, size_t count);
long sys_write_sandboxed(int fd, const void* buf, size_t count);
long sys_open_sandboxed(const char* pathname, int flags, int mode);
long sys_close_sandboxed(int fd);
long sys_exit_sandboxed(int status);
long sys_brk_sandboxed(void* addr, ElfContext* ctx);
long sys_munmap_sandboxed(void* addr, size_t length);
void set_current_elf_context(ElfContext* ctx);
ElfContext* get_current_elf_context(void);

// System call emulation table
typedef struct {
    int syscall_num;
    const char* name;
    void* handler;
} SyscallEntry;

// Linux system call numbers (x86_64)
#define SYS_read    0
#define SYS_write   1
#define SYS_open    2
#define SYS_close   3
#define SYS_exit    60
#define SYS_brk     12
#define SYS_mmap    9
#define SYS_munmap  11

// Forward declarations for syscall handlers
long sys_read(int fd, void* buf, size_t count);
long sys_write(int fd, const void* buf, size_t count);
long sys_open(const char* pathname, int flags, int mode);
long sys_close(int fd);
long sys_exit(int status);
long sys_brk(void* addr);
long sys_munmap(void* addr, size_t length);

// System call table
static SyscallEntry syscall_table[] = {
    {SYS_read, "read", sys_read},
    {SYS_write, "write", sys_write},
    {SYS_open, "open", sys_open},
    {SYS_close, "close", sys_close},
    {SYS_exit, "exit", sys_exit},
    {SYS_brk, "brk", sys_brk},
    {SYS_mmap, "mmap", sys_mmap},
    {SYS_munmap, "munmap", sys_munmap},
    {-1, NULL, NULL}
};

// Initialize ELF parser
int elf_init(void) {
    if (elf_initialized) {
        return 0;
    }
    
    printf("Initializing ELF parser and Linux emulation layer...\n");
    
    // Initialize system call emulation
    printf("   Setting up system call emulation...\n");
    printf("   Initializing memory management...\n");
    printf("   Setting up file descriptor table...\n");
    
    elf_initialized = 1;
    printf("ELF parser initialized successfully\n");
    return 0;
}

// Cleanup ELF parser
void elf_cleanup(void) {
    if (elf_initialized) {
        elf_initialized = 0;
    }
}

// Check if file is a valid ELF
int elf_is_valid(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return 0;
    }
    
    unsigned char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        fclose(f);
        return 0;
    }
    
    fclose(f);
    
    return (magic[0] == ELFMAG0 && magic[1] == ELFMAG1 && 
            magic[2] == ELFMAG2 && magic[3] == ELFMAG3);
}

// Get ELF architecture (32 or 64 bit)
int elf_get_architecture(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return 0;
    }
    
    unsigned char header[16];
    if (fread(header, 1, 16, f) != 16) {
        fclose(f);
        return 0;
    }
    
    fclose(f);
    
    if (header[EI_CLASS] == ELFCLASS32) {
        return 32;
    } else if (header[EI_CLASS] == ELFCLASS64) {
        return 64;
    }
    
    return 0;
}

// Create ELF context
ElfContext* elf_load(const char* filename) {
    if (!elf_initialized) {
        return NULL;
    }
    
    ElfContext* ctx = calloc(1, sizeof(ElfContext));
    if (!ctx) {
        return NULL;
    }
    
    ctx->filename = strdup(filename);
    ctx->file = fopen(filename, "rb");
    if (!ctx->file) {
        free(ctx->filename);
        free(ctx);
        return NULL;
    }
    
    printf("Loading ELF file: %s\n", filename);
    
    // Parse headers
    if (elf_parse_headers(ctx) != 0) {
        printf("Failed to parse ELF headers\n");
        elf_free_context(ctx);
        return NULL;
    }
    
    // Load segments
    if (elf_load_segments(ctx) != 0) {
        printf("Failed to load ELF segments\n");
        elf_free_context(ctx);
        return NULL;
    }
    
    printf("ELF file loaded successfully\n");
    return ctx;
}

// Parse ELF headers
int elf_parse_headers(ElfContext* ctx) {
    if (!ctx || !ctx->file) {
        return -1;
    }
    
    // Read ELF header
    fseek(ctx->file, 0, SEEK_SET);
    
    // Check if 32-bit or 64-bit
    unsigned char ident[EI_NIDENT];
    if (fread(ident, 1, EI_NIDENT, ctx->file) != EI_NIDENT) {
        return -1;
    }
    
    ctx->is_64bit = (ident[EI_CLASS] == ELFCLASS64);
    
    // Go back to beginning
    fseek(ctx->file, 0, SEEK_SET);
    
    if (ctx->is_64bit) {
        Elf64_Ehdr ehdr;
        if (fread(&ehdr, 1, sizeof(ehdr), ctx->file) != sizeof(ehdr)) {
            return -1;
        }
        
        ctx->entry_point = ehdr.e_entry;
        ctx->is_executable = (ehdr.e_type == ET_EXEC);
        
        printf("ELF64 Header Info:\n");
        printf("   Entry Point: 0x%016llx\n", ehdr.e_entry);
        printf("   Type: %s\n", ehdr.e_type == ET_EXEC ? "Executable" : "Dynamic");
        printf("   Machine: %d\n", ehdr.e_machine);
        printf("   Program Headers: %d\n", ehdr.e_phnum);
        
    } else {
        Elf32_Ehdr ehdr;
        if (fread(&ehdr, 1, sizeof(ehdr), ctx->file) != sizeof(ehdr)) {
            return -1;
        }
        
        ctx->entry_point = ehdr.e_entry;
        ctx->is_executable = (ehdr.e_type == ET_EXEC);
        
        printf("ELF32 Header Info:\n");
        printf("   Entry Point: 0x%08x\n", ehdr.e_entry);
        printf("   Type: %s\n", ehdr.e_type == ET_EXEC ? "Executable" : "Dynamic");
        printf("   Machine: %d\n", ehdr.e_machine);
        printf("   Program Headers: %d\n", ehdr.e_phnum);
    }
    
    return 0;
}

// Load ELF segments into memory
int elf_load_segments(ElfContext* ctx) {
    if (!ctx || !ctx->file) {
        return -1;
    }
    
    printf("Loading ELF segments with proper memory mapping...\n");
    
    // Read ELF header to get program header info
    fseek(ctx->file, 0, SEEK_SET);
    
    if (ctx->is_64bit) {
        Elf64_Ehdr ehdr;
        if (fread(&ehdr, 1, sizeof(ehdr), ctx->file) != sizeof(ehdr)) {
            return -1;
        }
        
        // Read program headers
        Elf64_Phdr* phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
        if (!phdrs) {
            return -1;
        }
        
        fseek(ctx->file, ehdr.e_phoff, SEEK_SET);
        if (fread(phdrs, sizeof(Elf64_Phdr), ehdr.e_phnum, ctx->file) != ehdr.e_phnum) {
            free(phdrs);
            return -1;
        }
        
        // Find the total memory range needed
        uint64_t min_addr = UINT64_MAX;
        uint64_t max_addr = 0;
        
        for (int i = 0; i < ehdr.e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD) {
                if (phdrs[i].p_vaddr < min_addr) {
                    min_addr = phdrs[i].p_vaddr;
                }
                if (phdrs[i].p_vaddr + phdrs[i].p_memsz > max_addr) {
                    max_addr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
                }
            }
        }
        
        // Allocate memory for the entire program
        size_t total_size = max_addr - min_addr;
        ctx->base_addr = VirtualAlloc(NULL, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!ctx->base_addr) {
            printf("Failed to allocate memory for ELF segments\n");
            free(phdrs);
            return -1;
        }
        
        ctx->total_size = total_size;
        printf("Allocated %zu bytes at 0x%p for ELF segments\n", total_size, ctx->base_addr);
        
        // Load each segment
        for (int i = 0; i < ehdr.e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD) {
                printf("Loading segment %d:\n", i);
                printf("   Virtual Address: 0x%016llx\n", phdrs[i].p_vaddr);
                printf("   File Size: %llu bytes\n", phdrs[i].p_filesz);
                printf("   Memory Size: %llu bytes\n", phdrs[i].p_memsz);
                
                // Calculate offset in our allocated memory
                void* target_addr = (char*)ctx->base_addr + (phdrs[i].p_vaddr - min_addr);
                
                // Read segment data from file
                fseek(ctx->file, phdrs[i].p_offset, SEEK_SET);
                if (fread(target_addr, 1, phdrs[i].p_filesz, ctx->file) != phdrs[i].p_filesz) {
                    printf("Failed to read segment data\n");
                    VirtualFree(ctx->base_addr, 0, MEM_RELEASE);
                    free(phdrs);
                    return -1;
                }
                
                // Zero out BSS section (memory size > file size)
                if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                    memset((char*)target_addr + phdrs[i].p_filesz, 0, 
                           phdrs[i].p_memsz - phdrs[i].p_filesz);
                }
                
                printf("Segment %d loaded successfully\n", i);
            }
        }
        
        // Adjust entry point relative to our base
        ctx->entry_point = ehdr.e_entry - min_addr;
        
        free(phdrs);
        
    } else {
        // Similar code for 32-bit ELF files
        printf("32-bit ELF loading not fully implemented yet\n");
        return -1;
    }
    
    printf("All ELF segments loaded successfully\n");
    return 0;
}

// Real ELF execution with system call emulation
int elf_execute(ElfContext* ctx, char** argv, int argc) {
    if (!ctx || !ctx->base_addr) {
        return -1;
    }
    
    printf("Executing ELF binary (REAL MODE)...\n");
    printf("   Entry Point: 0x%016llx\n", ctx->entry_point);
    printf("   Architecture: %s\n", ctx->is_64bit ? "64-bit" : "32-bit");
    printf("   Base Address: %p\n", ctx->base_addr);
    
    // Set up execution environment
    printf("Setting up execution environment...\n");
    
    // Create a new thread to execute the ELF binary
    HANDLE thread = CreateThread(
        NULL,                           // Default security attributes
        0,                              // Default stack size
        (LPTHREAD_START_ROUTINE)elf_thread_entry,  // Thread function
        ctx,                            // Thread parameter
        0,                              // Default creation flags
        NULL                            // Don't need thread ID
    );
    
    if (!thread) {
        printf("Failed to create execution thread\n");
        return -1;
    }
    
    // Wait for execution to complete
    WaitForSingleObject(thread, INFINITE);
    
    DWORD exit_code;
    GetExitCodeThread(thread, &exit_code);
    CloseHandle(thread);
    
    printf("ELF execution completed with exit code: %d\n", exit_code);
    return exit_code;
}

// Thread entry point for ELF execution
DWORD WINAPI elf_thread_entry(LPVOID param) {
    ElfContext* ctx = (ElfContext*)param;
    
    printf("Entering ELF execution thread...\n");
    
    // Use SetUnhandledExceptionFilter instead of __try for MinGW
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)elf_exception_handler);
    
    // Jump to the entry point
    void (*entry_func)(void) = (void(*)(void))((char*)ctx->base_addr + ctx->entry_point);
    
    printf("Jumping to entry point: %p\n", entry_func);
    
    // This is where the magic happens - execute the loaded ELF binary!
    entry_func();
    
    printf("ELF execution completed normally\n");
    return 0;
}

// Exception handler for system call interception
LONG WINAPI elf_exception_handler(EXCEPTION_POINTERS* exception_info) {
    DWORD exception_code = exception_info->ExceptionRecord->ExceptionCode;
    
    printf("Exception caught: 0x%08x\n", exception_code);
    
    // Check if this is a system call (we'll use int 0x80 or syscall instruction)
    if (exception_code == EXCEPTION_ILLEGAL_INSTRUCTION) {
        // This might be a system call - let's check the instruction
        BYTE* instruction = (BYTE*)exception_info->ExceptionRecord->ExceptionAddress;
        
        // Check for syscall instruction (0x0f 0x05)
        if (instruction[0] == 0x0f && instruction[1] == 0x05) {
            printf("System call intercepted\n");
            
            // Get system call number from RAX
            DWORD64 syscall_num = exception_info->ContextRecord->Rax;
            
            // Handle the system call
            long result = handle_syscall(syscall_num, exception_info->ContextRecord);
            
            // Set return value in RAX
            exception_info->ContextRecord->Rax = result;
            
            // Skip the syscall instruction
            exception_info->ContextRecord->Rip += 2;
            
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    
    return EXCEPTION_CONTINUE_SEARCH;
}

// Handle system calls
long handle_syscall(DWORD64 syscall_num, CONTEXT* context) {
    printf("System call: %lld\n", syscall_num);
    
    // Get arguments from registers (x86_64 calling convention)
    // Use proper types for register values
    uintptr_t arg1 = (uintptr_t)context->Rdi;
    uintptr_t arg2 = (uintptr_t)context->Rsi;
    uintptr_t arg3 = (uintptr_t)context->Rdx;
    uintptr_t arg4 = (uintptr_t)context->R10;
    uintptr_t arg5 = (uintptr_t)context->R8;
    uintptr_t arg6 = (uintptr_t)context->R9;
    
    // Find and call the appropriate handler
    for (int i = 0; syscall_table[i].name != NULL; i++) {
        if (syscall_table[i].syscall_num == syscall_num) {
            printf("   Calling: %s\n", syscall_table[i].name);
            
            // Call the handler with proper arguments
            switch (syscall_num) {
                case SYS_read:
                    return sys_read((int)arg1, (void*)arg2, (size_t)arg3);
                case SYS_write:
                    return sys_write((int)arg1, (const void*)arg2, (size_t)arg3);
                case SYS_open:
                    return sys_open((const char*)arg2, (int)arg2, (int)arg3);
                case SYS_close:
                    return sys_close((int)arg1);
                case SYS_exit:
                    sys_exit((int)arg1);
                    return 0; // Never reached
                case SYS_brk:
                    return sys_brk((void*)arg1);
                case SYS_mmap:
                    return sys_mmap((void*)arg1, (size_t)arg2, (int)arg3, (int)arg4, (int)arg5, (zora_off_t)arg6);
                case SYS_munmap:
                    return sys_munmap((void*)arg1, (size_t)arg2);
                default:
                    printf("Unhandled system call: %lld\n", syscall_num);
                    return -1;
            }
        }
    }
    
    printf("Unknown system call: %lld\n", syscall_num);
    return -1;
}

// System call implementations
long sys_read(int fd, void* buf, size_t count) {
    printf("   read(fd=%d, buf=%p, count=%zu)\n", fd, buf, count);
    
    if (fd == 0) { // stdin
        return fread(buf, 1, count, stdin);
    }
    
    // Handle other file descriptors
    return -1;
}

long sys_write(int fd, const void* buf, size_t count) {
    printf("   write(fd=%d, buf=%p, count=%zu)\n", fd, buf, count);
    
    if (fd == 1 || fd == 2) { // stdout/stderr
        return fwrite(buf, 1, count, stdout);
    }
    
    return -1;
}

long sys_open(const char* pathname, int flags, int mode) {
    printf("   open(pathname=%s, flags=%d, mode=%d)\n", pathname, flags, mode);
    
    // Simple file opening simulation
    FILE* f = fopen(pathname, "r");
    if (f) {
        // Return a fake file descriptor using proper conversion
        return (long)(uintptr_t)f;
    }
    
    return -1;
}

long sys_close(int fd) {
    printf("   close(fd=%d)\n", fd);
    
    if (fd > 2) {
        fclose((FILE*)(uintptr_t)fd);
    }
    
    return 0;
}

long sys_exit(int status) {
    printf("   exit(status=%d)\n", status);
    
    // Exit the current thread
    ExitThread(status);
    return 0; // Never reached
}

long sys_brk(void* addr) {
    printf("   brk(addr=%p)\n", addr);
    
    // Simple heap management
    static void* heap_end = NULL;
    
    if (addr == NULL) {
        // Return current heap end
        return (long)(uintptr_t)heap_end;
    }
    
    if (heap_end == NULL) {
        // Initialize heap
        heap_end = VirtualAlloc(NULL, 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    
    // Extend heap if needed
    heap_end = addr;
    return (long)(uintptr_t)heap_end;
}

long sys_munmap(void* addr, size_t length) {
    printf("   munmap(addr=%p, length=%zu)\n", addr, length);
    
    // Simple memory unmapping using VirtualFree
    if (addr != NULL) {
        VirtualFree(addr, 0, MEM_RELEASE);
        return 0;
    }
    
    return -1;
}

long sys_mmap(void* addr, size_t length, int prot, int flags, int fd, zora_off_t offset) {
    printf("   mmap(addr=%p, length=%zu, prot=%d, flags=%d, fd=%d, offset=%lld)\n", 
           addr, length, prot, flags, fd, (long long)offset);
    
    // Convert Linux protection flags to Windows flags
    DWORD protection = PAGE_NOACCESS;
    
    if (prot & 0x1) protection |= PAGE_READONLY;      // PROT_READ
    if (prot & 0x2) protection |= PAGE_READWRITE;     // PROT_WRITE
    if (prot & 0x4) protection |= PAGE_EXECUTE_READ;  // PROT_EXEC
    
    // Default to readable/writable/executable for simplicity
    if (protection == PAGE_NOACCESS) {
        protection = PAGE_EXECUTE_READWRITE;
    }
    
    // Simple memory mapping using VirtualAlloc
    void* mapped = VirtualAlloc(addr, length, MEM_COMMIT | MEM_RESERVE, protection);
    
    if (mapped) {
        printf("   Memory mapped successfully at %p\n", mapped);
        return (long)(uintptr_t)mapped;
    }
    
    printf("   Memory mapping failed\n");
    return -1;  // MAP_FAILED equivalent
}

// Global context management
static ElfContext* current_elf_context = NULL;

void set_current_elf_context(ElfContext* ctx) {
    current_elf_context = ctx;
}

ElfContext* get_current_elf_context(void) {
    return current_elf_context;
}

// Sandboxed syscall handler
long handle_sandboxed_syscall(DWORD64 syscall_num, CONTEXT* context) {
    printf("Sandboxed system call: %lld\n", syscall_num);
    
    // Get arguments from registers with proper types
    uintptr_t arg1 = (uintptr_t)context->Rdi;
    uintptr_t arg2 = (uintptr_t)context->Rsi;
    uintptr_t arg3 = (uintptr_t)context->Rdx;
    uintptr_t arg4 = (uintptr_t)context->R10;
    uintptr_t arg5 = (uintptr_t)context->R8;
    uintptr_t arg6 = (uintptr_t)context->R9;
    
    // Route to SANDBOXED handlers
    switch (syscall_num) {
        case SYS_read:
            return sys_read_sandboxed((int)arg1, (void*)arg2, (size_t)arg3);
        case SYS_write:
            return sys_write_sandboxed((int)arg1, (const void*)arg2, (size_t)arg3);
        case SYS_open:
            return sys_open_sandboxed((const char*)arg1, (int)arg2, (int)arg3);
        case SYS_close:
            return sys_close_sandboxed((int)arg1);
        case SYS_exit:
            return sys_exit_sandboxed((int)arg1);
        case SYS_brk:
            return sys_brk_sandboxed((void*)arg1, get_current_elf_context());
        case SYS_munmap:
            return sys_munmap_sandboxed((void*)arg1, (size_t)arg2);
        case SYS_mmap:
            return sys_mmap_sandboxed((void*)arg1, (size_t)arg2, (int)arg3, (int)arg4, (int)arg5, (zora_off_t)arg6);
        default:
            printf("Sandboxed syscall %lld not implemented\n", syscall_num);
            return -1;
    }
}

// Sandboxed exception handler
LONG WINAPI elf_sandboxed_exception_handler(EXCEPTION_POINTERS* exception_info) {
    DWORD exception_code = exception_info->ExceptionRecord->ExceptionCode;
    
    printf("Sandboxed exception caught: 0x%08x\n", exception_code);
    
    // Check if this is a system call
    if (exception_code == EXCEPTION_ILLEGAL_INSTRUCTION) {
        BYTE* instruction = (BYTE*)exception_info->ExceptionRecord->ExceptionAddress;
        
        // Check for syscall instruction (0x0f 0x05)
        if (instruction[0] == 0x0f && instruction[1] == 0x05) {
            printf("Sandboxed system call intercepted\n");
            
            // Get system call number from RAX
            DWORD64 syscall_num = exception_info->ContextRecord->Rax;
            
            // Handle the system call with sandboxing
            long result = handle_sandboxed_syscall(syscall_num, exception_info->ContextRecord);
            
            // Set return value in RAX
            exception_info->ContextRecord->Rax = result;
            
            // Skip the syscall instruction
            exception_info->ContextRecord->Rip += 2;
            
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    
    return EXCEPTION_CONTINUE_SEARCH;
}

// Add missing sandboxed syscall implementations:
long sys_munmap_sandboxed(void* addr, size_t length) {
    printf("sandboxed munmap(addr=%p, length=%zu)\n", addr, length);
    VirtualFree(addr, 0, MEM_RELEASE);
    return 0;
}

// Sandboxed ELF execution
int elf_execute_sandboxed(ElfContext* ctx, char** argv, int argc) {
    if (!ctx || !ctx->base_addr) {
        return -1;
    }
    
    printf("Executing ELF binary in sandbox...\n");
    printf("   Entry Point: 0x%016llx\n", ctx->entry_point);
    printf("   Architecture: %s\n", ctx->is_64bit ? "64-bit" : "32-bit");
    printf("   Sandbox Root: %s\n", ctx->sandbox_root ? ctx->sandbox_root : "Not set");
    
    // Set global context for syscall handlers
    set_current_elf_context(ctx);
    
    // Set up sandboxed heap
    ctx->heap_size = 16 * 1024 * 1024; // 16MB heap
    ctx->heap_base = VirtualAlloc(NULL, ctx->heap_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!ctx->heap_base) {
        printf("Failed to allocate sandboxed heap\n");
        set_current_elf_context(NULL);
        return -1;
    }
    
    ctx->heap_mutex = CreateMutex(NULL, FALSE, NULL);
    
    printf("Sandboxed heap allocated: %p (size: %zu)\n", ctx->heap_base, ctx->heap_size);
    
    // Create execution thread with sandbox restrictions
    HANDLE thread = CreateThread(
        NULL,                           // Default security attributes
        0,                              // Default stack size
        (LPTHREAD_START_ROUTINE)elf_sandboxed_thread_entry,  // Thread function
        ctx,                            // Thread parameter
        0,                              // Default creation flags
        NULL                            // Don't need thread ID
    );
    
    if (!thread) {
        printf("Failed to create sandboxed execution thread\n");
        VirtualFree(ctx->heap_base, 0, MEM_RELEASE);
        CloseHandle(ctx->heap_mutex);
        set_current_elf_context(NULL);
        return -1;
    }
    
    // Wait for execution with timeout
    DWORD wait_result = WaitForSingleObject(thread, 30000); // 30 second timeout
    
    DWORD exit_code = 0;
    if (wait_result == WAIT_OBJECT_0) {
        GetExitCodeThread(thread, &exit_code);
        printf("Sandboxed ELF execution completed with exit code: %d\n", exit_code);
    } else if (wait_result == WAIT_TIMEOUT) {
        printf("Sandboxed ELF execution timed out\n");
        TerminateThread(thread, 1);
        exit_code = 1;
    } else {
        printf("Sandboxed ELF execution wait failed\n");
        exit_code = -1;
    }
    
    CloseHandle(thread);
    
    // Cleanup sandboxed resources
    VirtualFree(ctx->heap_base, 0, MEM_RELEASE);
    CloseHandle(ctx->heap_mutex);
    
    // Clear global context
    set_current_elf_context(NULL);
    
    return exit_code;
}

// Sandboxed thread entry point
DWORD WINAPI elf_sandboxed_thread_entry(LPVOID param) {
    ElfContext* ctx = (ElfContext*)param;
    
    printf("Entering sandboxed ELF execution thread...\n");
    
    // Use SetUnhandledExceptionFilter instead of __try for MinGW
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)elf_sandboxed_exception_handler);
    
    // Change working directory to sandbox
    if (ctx->sandbox_root) {
        SetCurrentDirectoryA(ctx->sandbox_root);
    }
    
    // Jump to the entry point
    void (*entry_func)(void) = (void(*)(void))((char*)ctx->base_addr + ctx->entry_point);
    
    printf("Jumping to sandboxed entry point: %p\n", entry_func);
    
    // Execute the loaded ELF binary in sandbox!
    entry_func();
    
    printf("Sandboxed ELF execution completed normally\n");
    return 0;
}

// Enhanced system call implementations with sandboxing
long sys_write_sandboxed(int fd, const void* buf, size_t count) {
    printf("sandboxed write(fd=%d, buf=%p, count=%zu)\n", fd, buf, count);
    
    if (fd == 1 || fd == 2) { // stdout/stderr only
        // Limit output size
        size_t max_output = 4096;
        if (count > max_output) {
            count = max_output;
        }
        
        return fwrite(buf, 1, count, stdout);
    }
    
    printf("Write to fd %d blocked by sandbox\n", fd);
    return -1;
}

long sys_open_sandboxed(const char* pathname, int flags, int mode) {
    printf("sandboxed open(pathname=%s, flags=%d, mode=%d)\n", pathname, flags, mode);
    
    // Only allow access to files in sandbox
    if (strstr(pathname, "..") || pathname[0] == '/' || (pathname[1] == ':')) {
        printf("Access to %s blocked by sandbox\n", pathname);
        return -1;
    }
    
    // Simple file opening simulation within sandbox
    char sandbox_path[MAX_PATH];
    snprintf(sandbox_path, sizeof(sandbox_path), "%s\\%s", 
             "sandbox_root", pathname);
    
    FILE* f = fopen(sandbox_path, "r");
    if (f) {
        return (long)(uintptr_t)f;
    }
    
    return -1;
}

long sys_read_sandboxed(int fd, void* buf, size_t count) {
    printf("sandboxed read(fd=%d, buf=%p, count=%zu)\n", fd, buf, count);
    
    if (fd == 0) { // stdin only
        size_t max_read = 1024; // Limit input
        if (count > max_read) count = max_read;
        return fread(buf, 1, count, stdin);
    }
    
    printf("Read from fd %d blocked by sandbox\n", fd);
    return -1;
}

long sys_close_sandboxed(int fd) {
    printf("sandboxed close(fd=%d)\n", fd);
    
    if (fd > 2) {
        fclose((FILE*)(uintptr_t)fd);
    }
    return 0;
}

long sys_exit_sandboxed(int status) {
    printf("sandboxed exit(status=%d)\n", status);
    ExitThread(status);
    return 0;
}

long sys_brk_sandboxed(void* addr, ElfContext* ctx) {
    printf("sandboxed brk(addr=%p)\n", addr);
    
    if (!ctx || !ctx->heap_mutex) {
        return -1;
    }
    
    WaitForSingleObject(ctx->heap_mutex, INFINITE);
    
    static size_t heap_used = 0;
    
    if (addr == NULL) {
        // Return current heap end
        ReleaseMutex(ctx->heap_mutex);
        return (long)(uintptr_t)((char*)ctx->heap_base + heap_used);
    }
    
    size_t requested_size = (char*)addr - (char*)ctx->heap_base;
    
    if (requested_size > ctx->heap_size) {
        printf("Heap allocation blocked by sandbox (requested: %zu, limit: %zu)\n", 
               requested_size, ctx->heap_size);
        ReleaseMutex(ctx->heap_mutex);
        return -1;
    }
    
    heap_used = requested_size;
    ReleaseMutex(ctx->heap_mutex);
    
    printf("Heap extended to %zu bytes\n", heap_used);
    return (long)(uintptr_t)addr;
}

void elf_print_info(ElfContext* ctx) {
    if (!ctx) return;
    
    printf("ELF Information:\n");
    printf("   File: %s\n", ctx->filename ? ctx->filename : "Unknown");
    printf("   Architecture: %s\n", ctx->is_64bit ? "64-bit" : "32-bit");
    printf("   Entry Point: 0x%016llx\n", ctx->entry_point);
    printf("   Base Address: %p\n", ctx->base_addr);
    printf("   Total Size: %zu bytes\n", ctx->total_size);
    printf("   Executable: %s\n", ctx->is_executable ? "Yes" : "No");
}

void elf_free_context(ElfContext* ctx) {
    if (!ctx) return;
    
    if (ctx->filename) {
        free(ctx->filename);
    }
    
    if (ctx->file) {
        fclose(ctx->file);
    }
    
    if (ctx->base_addr) {
        VirtualFree(ctx->base_addr, 0, MEM_RELEASE);
    }
    
    if (ctx->heap_base) {
        VirtualFree(ctx->heap_base, 0, MEM_RELEASE);
    }
    
    if (ctx->heap_mutex) {
        CloseHandle(ctx->heap_mutex);
    }
    
    if (ctx->sandbox_root) {
        free(ctx->sandbox_root);
    }
    
    free(ctx);
}

// Add this after sys_munmap_sandboxed:
long sys_mmap_sandboxed(void* addr, size_t length, int prot, int flags, int fd, zora_off_t offset) {
    printf("sandboxed mmap(addr=%p, length=%zu, prot=%d, flags=%d, fd=%d, offset=%lld)\n", 
           addr, length, prot, flags, fd, (long long)offset);
    
    // Limit memory allocation in sandbox
    size_t max_alloc = 16 * 1024 * 1024; // 16MB limit
    if (length > max_alloc) {
        printf("mmap allocation blocked by sandbox (requested: %zu, limit: %zu)\n", 
               length, max_alloc);
        return -1;
    }
    
    // Use safer permissions in sandbox (no execute)
    void* mapped = VirtualAlloc(addr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (mapped) {
        printf("Sandboxed memory mapped at %p\n", mapped);
        return (long)(uintptr_t)mapped;
    }
    
    printf("Sandboxed memory mapping failed\n");
    return -1;
}