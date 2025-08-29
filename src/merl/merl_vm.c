#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "merl.h"
#include "shell.h"  // Include MERL shell header
#include "vfs.h"    // Include VFS header
#include "syscall.h" // Include syscall header

merl_vm_context_t* g_merl_vm_ctx = NULL;

int merl_init(void) {
    g_merl_vm_ctx = malloc(sizeof(merl_vm_context_t));
    if (!g_merl_vm_ctx) {
        return -1;
    }
    
    memset(g_merl_vm_ctx, 0, sizeof(merl_vm_context_t));
    g_merl_vm_ctx->running = 1;
    
    // Don't initialize VFS here - it's already initialized by virtualization_init
    
    printf("MERL shell initialized in VM environment\n");
    return 0;
}

void merl_cleanup(void) {
    if (g_merl_vm_ctx) {
        // Don't cleanup VFS here - virtualization layer handles it
        free(g_merl_vm_ctx);
        g_merl_vm_ctx = NULL;
        printf("MERL shell cleaned up\n");
    }
}

int merl_run(void) {
    // Use the actual MERL shell's start_shell function
    start_shell();
    return 0;
}

int merl_execute_command(const char* command) {
    // Use the actual MERL shell's handle_command function
    char cmd_copy[1024];
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    handle_command(cmd_copy);
    return 0;
}

void merl_set_vm_context(void* vm) {
    if (g_merl_vm_ctx) {
        g_merl_vm_ctx->vm_context = vm;
    }
}

void merl_set_hardware_context(void* cpu, void* memory, void* devices) {
    if (g_merl_vm_ctx) {
        g_merl_vm_ctx->cpu_context = cpu;
        g_merl_vm_ctx->memory_context = memory;
        g_merl_vm_ctx->device_context = devices;
    }
}

// VM-specific commands (keep these as they are)
int merl_cmd_vmstat(void) {
    printf("=== Zora VM Status ===\n");
    printf("CPU: Running\n");
    printf("Memory: 256MB allocated\n");
    printf("Shell: MERL v1.0\n");
    printf("Uptime: Running\n");
    return 0;
}

int merl_cmd_cpuinfo(void) {
    printf("=== CPU Information ===\n");
    printf("Architecture: Zora Virtual CPU\n");
    printf("Cores: 1\n");
    printf("Speed: Variable\n");
    return 0;
}

int merl_cmd_meminfo(void) {
    printf("=== Memory Information ===\n");
    printf("Total: 256MB\n");
    printf("Available: 200MB\n");
    printf("Used: 56MB\n");
    return 0;
}

int merl_cmd_devices(void) {
    printf("=== Virtual Devices ===\n");
    printf("vda: Virtual Disk A\n");
    printf("tty0: Virtual Terminal\n");
    return 0;
}

int merl_cmd_reboot(void) {
    printf("Rebooting Zora VM...\n");
    if (g_merl_vm_ctx) {
        g_merl_vm_ctx->running = 0;
    }
    return 0;
}