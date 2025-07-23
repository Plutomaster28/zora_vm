#include "platform/platform.h"
#include <stdio.h>

// Linux syscall filtering is handled by seccomp in sandbox_linux.c
// This file provides stubs for compatibility

void syscall_filter_init(void) {
    printf("Linux syscall filtering via seccomp (integrated)\n");
}

void syscall_filter_cleanup(void) {
    // Cleanup handled by sandbox_linux.c
}