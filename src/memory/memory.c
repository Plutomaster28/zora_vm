#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "memory.h"

static Memory* vm_memory = NULL;

Memory* memory_init(size_t size) {
    // Check if already initialized
    if (vm_memory != NULL) {
#if ZORA_VERBOSE_BOOT
        printf("Memory already initialized\n");
#endif
        return vm_memory;
    }
    
    // Allocate memory structure
    vm_memory = malloc(sizeof(Memory));
    if (!vm_memory) {
        fprintf(stderr, "Failed to allocate memory structure\n");
        return NULL;
    }
    
    // Initialize the memory structure
    vm_memory->size = size;
    vm_memory->allocated = 0;
    vm_memory->initialized = 0;
    
    // Allocate the actual memory buffer
#if ZORA_VERBOSE_BOOT
    printf("Allocating %zu bytes (%zu MB) for VM memory...\n", size, size / (1024 * 1024));
#endif
    vm_memory->data = malloc(size);
    if (!vm_memory->data) {
        fprintf(stderr, "Failed to allocate %zu bytes for VM memory\n", size);
        free(vm_memory);
        vm_memory = NULL;
        return NULL;
    }
    
    // Initialize memory to zero
    memset(vm_memory->data, 0, size);
    vm_memory->allocated = size;
    vm_memory->initialized = 1;
    
#if ZORA_VERBOSE_BOOT
    printf("Memory initialized successfully: %zu MB allocated\n", size / (1024 * 1024));
#endif
    return vm_memory;
}

void memory_cleanup(void) {
    if (vm_memory) {
        if (vm_memory->data) {
            free(vm_memory->data);
            vm_memory->data = NULL;
        }
        free(vm_memory);
        vm_memory = NULL;
        printf("Memory cleaned up\n");
    }
}

uint8_t memory_read(uint32_t address) {
    if (!vm_memory || !vm_memory->initialized || address >= vm_memory->size) {
        return 0;
    }
    return vm_memory->data[address];
}

void memory_write(uint32_t address, uint8_t value) {
    if (!vm_memory || !vm_memory->initialized || address >= vm_memory->size) {
        return;
    }
    vm_memory->data[address] = value;
}

int memory_read_block(uint32_t address, uint8_t* buffer, size_t size) {
    if (!vm_memory || !vm_memory->initialized || !buffer || address + size > vm_memory->size) {
        return -1;
    }
    memcpy(buffer, vm_memory->data + address, size);
    return 0;
}

int memory_write_block(uint32_t address, const uint8_t* buffer, size_t size) {
    if (!vm_memory || !vm_memory->initialized || !buffer || address + size > vm_memory->size) {
        return -1;
    }
    memcpy(vm_memory->data + address, buffer, size);
    return 0;
}

void memory_dump(uint32_t start, uint32_t end) {
    if (!vm_memory || !vm_memory->initialized) {
        printf("Memory not initialized\n");
        return;
    }
    
    if (end > vm_memory->size) {
        end = vm_memory->size;
    }
    
    printf("Memory dump from 0x%08X to 0x%08X:\n", start, end);
    for (uint32_t i = start; i < end; i += 16) {
        printf("%08X: ", i);
        for (int j = 0; j < 16 && i + j < end; j++) {
            printf("%02X ", vm_memory->data[i + j]);
        }
        printf("\n");
    }
}

// Memory protection (stub implementations)
int memory_protect(uint32_t address, size_t size, int flags) {
    printf("Memory protection not implemented\n");
    return 0;
}

int memory_unprotect(uint32_t address, size_t size) {
    printf("Memory unprotection not implemented\n");
    return 0;
}

// Memory mapping (stub implementations)
void* memory_map(uint32_t address, size_t size) {
    if (!vm_memory || !vm_memory->initialized || address + size > vm_memory->size) {
        return NULL;
    }
    return vm_memory->data + address;
}

int memory_unmap(void* ptr, size_t size) {
    // Nothing to do for our simple implementation
    return 0;
}

// Memory statistics
size_t memory_get_total(void) {
    return vm_memory ? vm_memory->size : 0;
}

size_t memory_get_used(void) {
    return vm_memory ? vm_memory->allocated : 0;
}

size_t memory_get_free(void) {
    return vm_memory ? (vm_memory->size - vm_memory->allocated) : 0;
}