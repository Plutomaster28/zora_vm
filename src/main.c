#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "device.h"
#include "kernel.h"
#include "sandbox.h"
#include "zoraperl.h"
#include "merl.h"
#include "vfs/vfs.h"
#include "syscall.h"
#include "virtualization.h"
#include "vm.h"
#include "network/network.h"

static volatile int running = 1;
static int vm_initialized = 0;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

int vm_init(void) {
    if (vm_initialized) {
        return 0;
    }
    printf("VM environment initialized\n");
    vm_initialized = 1;
    return 0;
}

void vm_cleanup(void) {
    if (vm_initialized) {
        printf("Syncing persistent storage before shutdown...\n");
        vfs_sync_all_persistent();
        printf("VM environment cleaned up\n");
        vm_initialized = 0;
    }
}

int vm_is_running(void) {
    return vm_initialized;
}

int main() {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting Zora VM...\n");

    // Initialize sandboxing first
    printf("Initializing sandbox...\n");
    if (sandbox_init() != 0) {
        fprintf(stderr, "Failed to initialize sandbox.\n");
        return 1;
    }

    // Apply resource limits
    printf("Setting memory limit to %d MB...\n", MEMORY_SIZE / (1024 * 1024));
    sandbox_set_memory_limit(MEMORY_SIZE);
    sandbox_set_cpu_limit(80); // 80% CPU limit
    
    // Initialize virtualization layer
    printf("Initializing virtualization layer...\n");
    if (virtualization_init() != 0) {
        fprintf(stderr, "Failed to initialize virtualization layer.\n");
        sandbox_cleanup();
        return 1;
    }

    // Initialize the virtual machine environment
    printf("Initializing VM environment...\n");
    if (vm_init() != 0) {
        fprintf(stderr, "Failed to initialize the virtual machine.\n");
        goto cleanup;
    }

    // Set up CPU, memory, and devices within sandbox
    printf("Initializing CPU...\n");
    if (cpu_init() != 0) {
        fprintf(stderr, "Failed to initialize CPU.\n");
        goto cleanup;
    }

    printf("Initializing memory (%d MB)...\n", MEMORY_SIZE / (1024 * 1024));
    Memory* mem = memory_init(MEMORY_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "CRITICAL: Failed to initialize memory (requested %d MB)\n", MEMORY_SIZE / (1024 * 1024));
        fprintf(stderr, "This could be due to insufficient system memory or memory limits.\n");
        goto cleanup;
    }
    printf("Memory initialization successful!\n");

    printf("Initializing devices...\n");
    if (device_init() != 0) {
        fprintf(stderr, "Failed to initialize devices.\n");
        goto cleanup;
    }

    // Initialize ZoraPerl runtime within the VM
    printf("Initializing ZoraPerl runtime...\n");
    if (zoraperl_init() != 0) {
        fprintf(stderr, "Failed to initialize ZoraPerl runtime.\n");
        goto cleanup;
    }

    // Initialize MERL shell within the VM
    printf("Initializing MERL shell...\n");
    if (merl_init() != 0) {
        fprintf(stderr, "Failed to initialize MERL shell.\n");
        goto cleanup;
    }

    // Initialize VFS (already done in virtualization_init)
    
    // Mount persistent directories
    printf("Setting up persistent storage...\n");
    
    // Mount ZoraPerl directory
    vfs_mount_persistent("/persistent", "./ZoraPerl");
    
    // Create subdirectories for different purposes
    vfs_mount_persistent("/persistent/documents", "./ZoraPerl/documents");
    vfs_mount_persistent("/persistent/scripts", "./ZoraPerl/scripts");
    vfs_mount_persistent("/persistent/data", "./ZoraPerl/data");
    vfs_mount_persistent("/persistent/projects", "./ZoraPerl/projects");
    
    printf("Persistent storage ready\n");

    // Initialize network virtualization
    printf("Initializing virtual network...\n");
    if (network_init() != 0) {
        fprintf(stderr, "Failed to initialize virtual network.\n");
        goto cleanup;
    }

    printf("Zora VM initialized successfully. Starting MERL shell...\n");
    printf("========================================\n");

    // Start the MERL shell as the "OS"
    int result = merl_run();
    
    if (result != 0) {
        fprintf(stderr, "MERL shell execution failed with code: %d\n", result);
    }

cleanup:
    // Clean up resources before exiting
    printf("\nShutting down Zora VM...\n");
    merl_cleanup();
    zoraperl_cleanup();
    device_cleanup();
    memory_cleanup();
    cpu_cleanup();
    vm_cleanup();
    virtualization_cleanup();
    sandbox_cleanup();
    network_cleanup();
    
    printf("Zora VM shutdown complete.\n");
    return 0;
}