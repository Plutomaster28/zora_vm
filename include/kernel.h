#ifndef KERNEL_H
#define KERNEL_H

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Kernel functions
int kernel_main(void);
int kernel_start(void);
void kernel_shutdown(void);
int kernel_is_running(void);

#endif // KERNEL_H