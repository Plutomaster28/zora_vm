#include <stdio.h>
#include <stdlib.h>
#include "crash.h"

void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "memory limit passed, MERL shat itself\n");
        exit(1);
    }
    return ptr;
}

void* safe_calloc(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    if (!ptr) {
        fprintf(stderr, "memory limit passed, MERL shat itself\n");
        exit(1);
    }
    return ptr;
}

void* safe_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "memory limit passed, MERL shat itself\n");
        exit(1);
    }
    return new_ptr;
}