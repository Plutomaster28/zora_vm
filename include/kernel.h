#ifndef KERNEL_H
#define KERNEL_H

#include <windows.h>

// Kernel functions
int kernel_main(void);
int kernel_start(void);
void kernel_shutdown(void);
int kernel_is_running(void);

#endif // KERNEL_H