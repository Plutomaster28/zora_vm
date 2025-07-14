#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define MEMORY_SIZE 0x4000000 // 64 MB instead of 256 MB

// Memory structure
typedef struct {
    uint8_t* data;
    size_t size;
    size_t allocated;
    int initialized;
} Memory;

// Function declarations
Memory* memory_init(size_t size);
void memory_cleanup(void);
uint8_t memory_read(uint32_t address);
void memory_write(uint32_t address, uint8_t value);
int memory_read_block(uint32_t address, uint8_t* buffer, size_t size);
int memory_write_block(uint32_t address, const uint8_t* buffer, size_t size);
void memory_dump(uint32_t start, uint32_t end);

// Memory protection
int memory_protect(uint32_t address, size_t size, int flags);
int memory_unprotect(uint32_t address, size_t size);

// Memory mapping
void* memory_map(uint32_t address, size_t size);
int memory_unmap(void* ptr, size_t size);

// Memory statistics
size_t memory_get_total(void);
size_t memory_get_used(void);
size_t memory_get_free(void);

#endif // MEMORY_H