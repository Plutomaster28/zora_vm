#include "kernel/privilege.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Global privilege context
PrivilegeContext g_privilege_context;

// Stack buffers for kernel and user mode
#define KERNEL_STACK_SIZE (64 * 1024)  // 64KB kernel stack
#define USER_STACK_SIZE (256 * 1024)   // 256KB user stack

static uint8_t kernel_stack_buffer[KERNEL_STACK_SIZE];
static uint8_t user_stack_buffer[USER_STACK_SIZE];

// Initialize privilege system
void privilege_init(void) {
    memset(&g_privilege_context, 0, sizeof(PrivilegeContext));
    
    // Setup kernel stack (grows downward)
    g_privilege_context.kernel_stack = (uint32_t)&kernel_stack_buffer[KERNEL_STACK_SIZE - 16];
    
    // Setup user stack (grows downward)
    g_privilege_context.user_stack = (uint32_t)&user_stack_buffer[USER_STACK_SIZE - 16];
    
    // Start in kernel mode with kernel stack active
    g_privilege_context.current_level = PRIVILEGE_KERNEL;
    g_privilege_context.flags = PRIV_FLAG_SUPERVISOR | PRIV_FLAG_IO_ACCESS |
                                PRIV_FLAG_MEM_MANAGE | PRIV_FLAG_INTERRUPT;
    g_privilege_context.current_esp = g_privilege_context.kernel_stack;
    
    printf("[PRIVILEGE] Initialized in kernel mode (ring %d)\n", g_privilege_context.current_level);
    printf("[PRIVILEGE] Kernel stack: 0x%08X (size: %d KB)\n", 
           g_privilege_context.kernel_stack, KERNEL_STACK_SIZE / 1024);
    printf("[PRIVILEGE] User stack: 0x%08X (size: %d KB)\n", 
           g_privilege_context.user_stack, USER_STACK_SIZE / 1024);
}

// Enter kernel mode (called during interrupt/syscall/exception)
void privilege_enter_kernel_mode(void) {
    if (g_privilege_context.current_level == PRIVILEGE_KERNEL) {
        return;  // Already in kernel mode
    }
    
    // Save current user mode stack pointer
    uint32_t saved_user_esp = g_privilege_context.current_esp;
    g_privilege_context.user_stack = saved_user_esp;
    
    // Validate user stack is within bounds
    uint32_t user_stack_base = (uint32_t)user_stack_buffer;
    uint32_t user_stack_top = user_stack_base + USER_STACK_SIZE;
    if (saved_user_esp < user_stack_base || saved_user_esp > user_stack_top) {
        printf("[PRIVILEGE] WARNING: User ESP 0x%08X out of bounds!\n", saved_user_esp);
    }
    
    // Switch to kernel stack
    g_privilege_context.current_esp = g_privilege_context.kernel_stack;
    
    // Elevate privilege
    g_privilege_context.current_level = PRIVILEGE_KERNEL;
    g_privilege_context.flags |= PRIV_FLAG_SUPERVISOR | PRIV_FLAG_IO_ACCESS |
                                 PRIV_FLAG_MEM_MANAGE | PRIV_FLAG_INTERRUPT;
    g_privilege_context.flags &= ~PRIV_FLAG_SYSCALL;
}

// Enter user mode (called during process switch/return from interrupt)
void privilege_enter_user_mode(void) {
    if (g_privilege_context.current_level == PRIVILEGE_USER) {
        return;  // Already in user mode
    }
    
    // Save current kernel mode stack pointer
    uint32_t saved_kernel_esp = g_privilege_context.current_esp;
    g_privilege_context.kernel_stack = saved_kernel_esp;
    
    // Validate kernel stack is within bounds
    uint32_t kernel_stack_base = (uint32_t)kernel_stack_buffer;
    uint32_t kernel_stack_top = kernel_stack_base + KERNEL_STACK_SIZE;
    if (saved_kernel_esp < kernel_stack_base || saved_kernel_esp > kernel_stack_top) {
        printf("[PRIVILEGE] WARNING: Kernel ESP 0x%08X out of bounds!\n", saved_kernel_esp);
    }
    
    // Switch to user stack
    g_privilege_context.current_esp = g_privilege_context.user_stack;
    
    // Lower privilege
    g_privilege_context.current_level = PRIVILEGE_USER;
    g_privilege_context.flags &= ~(PRIV_FLAG_SUPERVISOR | PRIV_FLAG_IO_ACCESS |
                                   PRIV_FLAG_MEM_MANAGE | PRIV_FLAG_INTERRUPT);
    g_privilege_context.flags |= PRIV_FLAG_SYSCALL;
}

// Check if current privilege level has required access
int privilege_check_access(uint32_t required_level) {
    // Lower number = higher privilege
    // Ring 0 can access anything, Ring 3 can only access Ring 3
    return (g_privilege_context.current_level <= required_level) ? 1 : 0;
}

// Check if instruction can be executed at current privilege level
int privilege_can_execute(uint32_t instruction_flags) {
    // Supervisor instructions require kernel mode
    if (instruction_flags & PRIV_FLAG_SUPERVISOR) {
        return (g_privilege_context.flags & PRIV_FLAG_SUPERVISOR) ? 1 : 0;
    }
    
    // I/O instructions require I/O privilege
    if (instruction_flags & PRIV_FLAG_IO_ACCESS) {
        return (g_privilege_context.flags & PRIV_FLAG_IO_ACCESS) ? 1 : 0;
    }
    
    // Memory management instructions require MM privilege
    if (instruction_flags & PRIV_FLAG_MEM_MANAGE) {
        return (g_privilege_context.flags & PRIV_FLAG_MEM_MANAGE) ? 1 : 0;
    }
    
    return 1;  // Normal instructions can always execute
}

// Check if memory address can be accessed
int privilege_can_access_memory(uint32_t address, int write) {
    // Kernel mode can access anything
    if (g_privilege_context.current_level == PRIVILEGE_KERNEL) {
        return 1;
    }
    
    // User mode has restrictions (checked by MMU)
    // This is a simplified check - real check is in MMU/page tables
    #define USER_SPACE_START 0x40000000
    #define KERNEL_SPACE_END 0x3FFFFFFF
    
    if (address <= KERNEL_SPACE_END) {
        // Trying to access kernel space from user mode
        return 0;
    }
    
    return 1;
}

// Raise privilege exception
void privilege_raise_exception(uint32_t exception_code) {
    printf("[PRIVILEGE] Exception 0x%02X at ring %d\n", 
           exception_code, g_privilege_context.current_level);
    
    // In real kernel, this would trigger interrupt handler
    // For now, just log it
    switch (exception_code) {
        case EXCEPTION_GENERAL_PROTECTION:
            printf("[PRIVILEGE] General Protection Fault\n");
            break;
        case EXCEPTION_PRIVILEGE_VIOLATION:
            printf("[PRIVILEGE] Privilege Violation\n");
            break;
        case EXCEPTION_INVALID_OPCODE:
            printf("[PRIVILEGE] Invalid Opcode\n");
            break;
        case EXCEPTION_STACK_FAULT:
            printf("[PRIVILEGE] Stack Fault\n");
            break;
        default:
            printf("[PRIVILEGE] Unknown Exception\n");
            break;
    }
}

// Privileged operations (checked versions)
void privilege_cli(void) {
    if (!(g_privilege_context.flags & PRIV_FLAG_SUPERVISOR)) {
        privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
        return;
    }
    // Would execute CLI instruction here
    // For simulation, just note that interrupts are disabled
}

void privilege_sti(void) {
    if (!(g_privilege_context.flags & PRIV_FLAG_SUPERVISOR)) {
        privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
        return;
    }
    // Would execute STI instruction here
}

void privilege_hlt(void) {
    if (!(g_privilege_context.flags & PRIV_FLAG_SUPERVISOR)) {
        privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
        return;
    }
    // Would execute HLT instruction here
}

uint32_t privilege_in(uint16_t port) {
    if (!(g_privilege_context.flags & PRIV_FLAG_IO_ACCESS)) {
        privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
        return 0xFFFFFFFF;
    }
    // Would execute IN instruction here
    return 0;
}

void privilege_out(uint16_t port, uint32_t value) {
    if (!(g_privilege_context.flags & PRIV_FLAG_IO_ACCESS)) {
        privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
        return;
    }
    // Would execute OUT instruction here
}
