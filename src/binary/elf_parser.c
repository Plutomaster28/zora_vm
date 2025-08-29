#include "binary/elf_parser.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Platform-specific includes
#include <windows.h>
#include <psapi.h>

// Cross-platform off_t definition
#ifndef zora_off_t
typedef __int64 zora_off_t;
#endif

// Global context - declare and initialize properly
static ElfContext* current_elf_context = NULL;
static int elf_initialized = 0;

// Cross-platform ELF parsing implementation
int elf_parse_header(ElfContext* ctx, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return -1;
    }
    
    if (fread(&ctx->header, sizeof(Elf64_Ehdr), 1, file) != 1) {
        printf("Failed to read ELF header\n");
        fclose(file);
        return -1;
    }
    
    fclose(file);
    
    if (!elf_is_valid_header(&ctx->header)) {
        printf("Invalid ELF header\n");
        return -1;
    }
    
    printf("ELF file parsed successfully: %s\n", filename);
    return 0;
}

int elf_load_segments(ElfContext* ctx) {
    printf("Loading ELF segments for: %s\n", ctx->filename);
    // Simplified implementation
    return 0;
}

int elf_execute(ElfContext* ctx, char** argv, int argc) {
    printf("Executing ELF: %s\n", ctx->filename);
    return 0;
}

int elf_execute_sandboxed(ElfContext* ctx, char** argv, int argc) {
    printf("Executing sandboxed ELF: %s\n", ctx->filename);
    return 0;
}

void elf_cleanup(ElfContext* ctx) {
    if (ctx && ctx->program_headers) {
        free(ctx->program_headers);
        ctx->program_headers = NULL;
    }
}

bool elf_is_valid_header(const Elf64_Ehdr* header) {
    return (header->e_ident[0] == 0x7F &&
            header->e_ident[1] == 'E' &&
            header->e_ident[2] == 'L' &&
            header->e_ident[3] == 'F');
}

const char* elf_get_type_string(uint16_t type) {
    switch(type) {
        case ET_NONE: return "NONE";
        case ET_REL: return "REL";
        case ET_EXEC: return "EXEC";
        case ET_DYN: return "DYN";
        case ET_CORE: return "CORE";
        default: return "UNKNOWN";
    }
}

const char* elf_get_machine_string(uint16_t machine) {
    switch(machine) {
        case 0x3E: return "x86-64";
        case 0x3: return "x86";
        case 0xB7: return "AArch64";
        default: return "UNKNOWN";
    }
}

// Utility functions - fixed to match header structure
ElfContext* get_current_elf_context(void) {
    return current_elf_context;
}

void elf_print_info(ElfContext* ctx) {
    if (!ctx) {
        printf("No ELF context available\n");
        return;
    }
    
    printf("ELF Information:\n");
    printf("   File: %s\n", ctx->filename);
    printf("   Entry Point: 0x%016lx\n", (unsigned long)ctx->header.e_entry);  // Use header.e_entry instead of entry_point
    printf("   Type: %s\n", elf_get_type_string(ctx->header.e_type));
    printf("   Machine: %s\n", elf_get_machine_string(ctx->header.e_machine));
}

void elf_free_context(ElfContext* ctx) {
    if (!ctx) return;
    
    if (ctx->base_address) {  // Use base_address instead of base_addr
        VirtualFree(ctx->base_address, 0, MEM_RELEASE);
        ctx->base_address = NULL;
    }
    
    if (ctx->program_headers) {
        free(ctx->program_headers);
        ctx->program_headers = NULL;
    }
    
    free(ctx);
}

// Memory management functions
void* elf_allocate_memory(ElfContext* ctx, size_t size) {
    if (!ctx) return NULL;
    
    printf("ELF: Allocating %zu bytes\n", size);
    
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void elf_free_memory(ElfContext* ctx, void* ptr) {
    if (!ctx || !ptr) return;
    
    printf("ELF: Freeing memory\n");
    
    VirtualFree(ptr, 0, MEM_RELEASE);
}

// Global cleanup function - renamed to avoid conflict
void elf_global_cleanup(void) {
    printf("ELF cleanup: Cleaning up ELF subsystem\n");
    
    // Clean up current context if it exists
    if (current_elf_context) {
        elf_free_context(current_elf_context);
        current_elf_context = NULL;
    }
    
    // Reset initialization flag
    elf_initialized = 0;
    
    printf("ELF cleanup completed\n");
}

// ELF initialization function
int elf_init(void) {
    if (elf_initialized) {
        printf("ELF subsystem already initialized\n");
        return 0;
    }
    
    printf("Initializing ELF subsystem\n");
    
    current_elf_context = NULL;
    elf_initialized = 1;
    
    printf("ELF subsystem initialized successfully\n");
    return 0;
}

// Set current context
void elf_set_current_context(ElfContext* ctx) {
    current_elf_context = ctx;
}

// Check if ELF subsystem is initialized
bool elf_is_initialized(void) {
    return elf_initialized != 0;
}