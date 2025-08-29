#include <stdio.h>
#include <stdlib.h>
#include "kernel.h"
#include "cpu.h"
#include "memory.h"
#include "device.h"

static int kernel_running = 0;

int kernel_main(void) {
    printf("Kernel starting up...\n");
    
    // Initialize kernel components
    cpu_init();
    
    // Initialize memory with proper size
    if (memory_init(MEMORY_SIZE) == NULL) {
        printf("Failed to initialize memory\n");
        return -1;
    }
    
    device_init();
    
    kernel_running = 1;
    printf("Kernel initialized successfully\n");
    
    // Kernel main loop
    while (kernel_running) {
        // Handle system tasks
        Sleep(10);
    }
    
    return 0;
}

int kernel_start(void) {
    return kernel_main();
}

void kernel_shutdown(void) {
    kernel_running = 0;
    printf("Kernel shutting down...\n");
}

int kernel_is_running(void) {
    return kernel_running;
}