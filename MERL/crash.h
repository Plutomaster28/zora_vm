#ifndef CRASH_H
#define CRASH_H

#include <stddef.h>

void* safe_malloc(size_t size);
void* safe_calloc(size_t num, size_t size);
void* safe_realloc(void* ptr, size_t size);

#endif // CRASH_H