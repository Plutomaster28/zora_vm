#include "zoraperl.h"
#include "cpu.h"
#include "memory.h"
#include "device.h"
#include "merl.h"  // Add MERL integration
#include <stdlib.h>
#include <string.h>

zoraperl_vm_t* g_zoraperl_vm = NULL;

int zoraperl_init(void) {
    g_zoraperl_vm = malloc(sizeof(zoraperl_vm_t));
    if (!g_zoraperl_vm) {
        return -1;
    }
    
    memset(g_zoraperl_vm, 0, sizeof(zoraperl_vm_t));
    g_zoraperl_vm->current_dir = strdup("/");
    g_zoraperl_vm->running = 1;
    
    // Initialize virtual filesystem
    if (zoraperl_vfs_init() != 0) {
        free(g_zoraperl_vm->current_dir);
        free(g_zoraperl_vm);
        return -1;
    }
    
    // Initialize MERL shell within the VM
    if (merl_init() != 0) {
        zoraperl_vfs_cleanup();
        free(g_zoraperl_vm->current_dir);
        free(g_zoraperl_vm);
        return -1;
    }
    
    printf("ZoraPerl VM runtime initialized with MERL shell\n");
    return 0;
}

void zoraperl_cleanup(void) {
    if (g_zoraperl_vm) {
        merl_cleanup();  // Cleanup MERL first
        zoraperl_vfs_cleanup();
        free(g_zoraperl_vm->current_dir);
        free(g_zoraperl_vm);
        g_zoraperl_vm = NULL;
    }
}

int zoraperl_start_shell(void) {
    printf("=== Zora VM Custom OS ===\n");
    printf("MERL Shell v1.0 - Running in Virtual Machine\n");
    printf("Type 'help' for available commands, 'exit' to quit\n\n");
    
    // Set VM context for MERL so it can access VM resources
    merl_set_vm_context(g_zoraperl_vm);
    
    // Start the MERL shell main loop
    int result = merl_run();
    
    return result;
}

int zoraperl_execute_script(const char* script) {
    // Pass script execution to MERL
    return merl_execute_command(script);
}

void zoraperl_set_vm_context(void* cpu, void* memory, void* devices) {
    if (g_zoraperl_vm) {
        g_zoraperl_vm->vm_context = cpu; // Store references for VM integration
        // Also pass these to MERL for direct hardware access
        merl_set_hardware_context(cpu, memory, devices);
    }
}