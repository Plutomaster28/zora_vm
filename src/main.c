#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>  // Add this include
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
#include "lua/lua_vm.h"
#include "binary/binary_executor.h"

#ifdef PYTHON_SCRIPTING
#include "python/python_vm.h"
#endif
#ifdef PERL_SCRIPTING
#include "perl/perl_vm.h"
#endif

static volatile int running = 1;
static int vm_initialized = 0;

// Add crash protection variables
static volatile int vm_crash_guard = 0;
static jmp_buf vm_main_loop;

// Enhanced signal handler with crash protection
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

// VM crash protection handler
void vm_crash_handler(int sig) {
    if (vm_crash_guard) {
        printf("CRITICAL: VM crash guard triggered - preventing VM death\n");
        printf("Signal: %d\n", sig);
        printf("This indicates a serious bug in binary execution\n");
        
        // Force cleanup and return to VM shell
        binary_executor_cleanup();
        // Note: cleanup_sandbox_environment() might not exist, so use sandbox_cleanup()
        sandbox_cleanup();
        
        // Reset crash guard temporarily to prevent infinite loops
        vm_crash_guard = 0;
        
        // Don't exit - return to VM shell
        printf("Attempting to recover and return to VM shell...\n");
        longjmp(vm_main_loop, 1);
    } else {
        // If crash guard is off, use normal signal handler
        signal_handler(sig);
    }
}

// Initialize crash protection
void vm_init_crash_protection() {
    printf("Initializing VM crash protection...\n");
    signal(SIGSEGV, vm_crash_handler);
    signal(SIGABRT, vm_crash_handler);
    signal(SIGFPE, vm_crash_handler);
    signal(SIGILL, vm_crash_handler);  // Add illegal instruction
    vm_crash_guard = 1;
    printf("VM crash protection enabled\n");
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

int main(int argc, char* argv[]) {
    // Set up signal handlers first
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting Zora VM...\n");

    // Initialize crash protection early
    vm_init_crash_protection();

    // Set jump point for crash recovery
    if (setjmp(vm_main_loop) != 0) {
        printf("VM recovered from crash, reinitializing...\n");
        // Re-enable crash guard after recovery
        vm_crash_guard = 1;
    }

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
    
    // Enable strict sandbox mode
    printf("Enabling strict sandbox mode...\n");
    sandbox_set_strict_mode(1);
    sandbox_block_network_access(1);
    sandbox_block_file_system_access(1);
    sandbox_block_system_calls(1);
    
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

    // Initialize VFS
    printf("Initializing VFS...\n");
    if (vfs_init() != 0) {
        fprintf(stderr, "Failed to initialize VFS\n");
        goto cleanup;
    }
    
    // Ensure the entire ZoraPerl directory structure exists
    printf("Ensuring ZoraPerl directory structure exists...\n");
    create_directory_recursive("../ZoraPerl");
    create_directory_recursive("../ZoraPerl/documents");
    create_directory_recursive("../ZoraPerl/scripts");
    create_directory_recursive("../ZoraPerl/data");
    create_directory_recursive("../ZoraPerl/projects");
    
    // Initialize persistent directories
    printf("Setting up persistent storage...\n");
    
    // Create the basic directory structure
    vfs_create_directory("/persistent");
    vfs_create_directory("/persistent/documents");
    vfs_create_directory("/persistent/scripts");
    vfs_create_directory("/persistent/data");
    vfs_create_directory("/persistent/projects");
    
    // Mount ZoraPerl directory
    vfs_mount_persistent("/persistent", "../ZoraPerl");
    
    // Create subdirectories for different purposes
    vfs_mount_persistent("/persistent/documents", "../ZoraPerl/documents");
    vfs_mount_persistent("/persistent/scripts", "../ZoraPerl/scripts");
    vfs_mount_persistent("/persistent/data", "../ZoraPerl/data");
    vfs_mount_persistent("/persistent/projects", "../ZoraPerl/projects");
    
    printf("Persistent storage ready\n");

    // Initialize network virtualization
    printf("Initializing virtual network...\n");
    if (network_init() != 0) {
        fprintf(stderr, "Failed to initialize virtual network.\n");
        goto cleanup;
    }

    // Initialize Lua scripting engine
    printf("Initializing Lua scripting engine...\n");
    if (lua_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Lua VM\n");
        goto cleanup;
    }

#ifdef PYTHON_SCRIPTING
    printf("Initializing Python scripting engine...\n");
    if (python_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Python VM\n");
        goto cleanup;
    }
#endif

#ifdef PERL_SCRIPTING
    printf("Initializing Perl scripting engine...\n");
    if (perl_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Perl VM\n");
        goto cleanup;
    }
#endif

    printf("Initializing binary executor...\n");
    if (binary_executor_init() != 0) {
        fprintf(stderr, "Failed to initialize binary executor\n");
        goto cleanup;
    }

    printf("Zora VM initialized successfully. Starting MERL shell...\n");
    printf("========================================\n");

    // Start the MERL shell as the "OS" with crash protection
    int result = 0;
    while (running) {
        if (setjmp(vm_main_loop) != 0) {
            printf("Recovered from crash in MERL shell, restarting...\n");
            // Re-enable crash guard
            vm_crash_guard = 1;
            continue;
        }
        
        result = merl_run();
        
        if (result != 0) {
            fprintf(stderr, "MERL shell execution failed with code: %d\n", result);
        }
        break; // Normal exit
    }

cleanup:
    // Disable crash guard during cleanup
    vm_crash_guard = 0;
    
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
    lua_vm_cleanup();
    vfs_cleanup();
    binary_executor_cleanup();
    
#ifdef PYTHON_SCRIPTING
    python_vm_cleanup();
#endif
#ifdef PERL_SCRIPTING
    perl_vm_cleanup();
#endif
    
    printf("Zora VM shutdown complete.\n");
    return 0;
}