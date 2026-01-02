#ifndef KERNEL_PRIVILEGE_H
#define KERNEL_PRIVILEGE_H

#include <stdint.h>

// Privilege levels (ring-based protection)
#define PRIVILEGE_KERNEL    0  // Ring 0 - Kernel mode (highest privilege)
#define PRIVILEGE_DRIVER    1  // Ring 1 - Device drivers
#define PRIVILEGE_SERVICE   2  // Ring 2 - System services
#define PRIVILEGE_USER      3  // Ring 3 - User mode (lowest privilege)

// Privilege flags
#define PRIV_FLAG_SUPERVISOR   0x0001  // Can execute privileged instructions
#define PRIV_FLAG_IO_ACCESS    0x0002  // Can access I/O ports
#define PRIV_FLAG_MEM_MANAGE   0x0004  // Can manage memory mappings
#define PRIV_FLAG_INTERRUPT    0x0008  // Can handle interrupts
#define PRIV_FLAG_SYSCALL      0x0010  // Can make syscalls (user mode only)

// Current privilege context
typedef struct {
    uint32_t current_level;     // Current privilege level (0-3)
    uint32_t flags;             // Privilege flags
    uint32_t kernel_stack;      // Kernel mode stack pointer (saved)
    uint32_t user_stack;        // User mode stack pointer (saved)
    uint32_t current_esp;       // Currently active ESP
    void* kernel_context;       // Saved kernel context
    void* user_context;         // Saved user context
} PrivilegeContext;

// Global privilege state
extern PrivilegeContext g_privilege_context;

// Privilege management functions
void privilege_init(void);
void privilege_enter_kernel_mode(void);
void privilege_enter_user_mode(void);
int privilege_check_access(uint32_t required_level);
int privilege_can_execute(uint32_t instruction_flags);
int privilege_can_access_memory(uint32_t address, int write);
void privilege_raise_exception(uint32_t exception_code);

// Privilege-checked operations
void privilege_cli(void);  // Clear interrupts (privileged)
void privilege_sti(void);  // Set interrupts (privileged)
void privilege_hlt(void);  // Halt CPU (privileged)
uint32_t privilege_in(uint16_t port);   // I/O input (privileged)
void privilege_out(uint16_t port, uint32_t value);  // I/O output (privileged)

// Exception codes
#define EXCEPTION_GENERAL_PROTECTION  0x0D
#define EXCEPTION_PRIVILEGE_VIOLATION 0x0E  // Changed from 0x0C to avoid conflict
#define EXCEPTION_INVALID_OPCODE      0x06
#define EXCEPTION_STACK_FAULT         0x0C

#endif // KERNEL_PRIVILEGE_H
