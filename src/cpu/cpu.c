#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include "cpu.h"
#include "memory.h"
#include "kernel/privilege.h"
#include "kernel/interrupts.h"

static CPUState cpu;

int cpu_init(void) {
    cpu.pc = 0;
    cpu.sp = 0x1000; // Stack pointer starts at 4KB
    cpu.flags = 0;
    cpu.running = 1;
    cpu.privilege_level = PRIVILEGE_KERNEL;  // Start in kernel mode
    
    // Initialize registers
    for (int i = 0; i < 8; i++) {
        cpu.registers[i] = 0;
    }
    
#if ZORA_VERBOSE_BOOT
    printf("CPU initialized (privilege level: %d)\n", cpu.privilege_level);
#endif
    return 0;
}

void cpu_cleanup(void) {
    cpu.running = 0;
    printf("CPU cleaned up\n");
}

void cpu_execute_instruction(CPUState *cpu, unsigned int instruction) {
    // Check privilege for certain instructions
    switch (instruction) {
        case 0x00: // NOP
            break;
        case 0x01: // HALT (privileged)
            if (cpu->privilege_level != PRIVILEGE_KERNEL) {
                privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
                return;
            }
            cpu->running = 0;
            break;
        case 0xFA: // CLI (clear interrupts - privileged)
            if (cpu->privilege_level != PRIVILEGE_KERNEL) {
                privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
                return;
            }
            privilege_cli();
            break;
        case 0xFB: // STI (set interrupts - privileged)
            if (cpu->privilege_level != PRIVILEGE_KERNEL) {
                privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
                return;
            }
            privilege_sti();
            break;
        case 0xF4: // HLT (halt - privileged)
            if (cpu->privilege_level != PRIVILEGE_KERNEL) {
                privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
                return;
            }
            privilege_hlt();
            break;
        default:
            printf("Unknown instruction: 0x%02X\n", instruction);
            privilege_raise_exception(EXCEPTION_INVALID_OPCODE);
            break;
    }
}

void cpu_handle_interrupt(CPUState *cpu, unsigned int interrupt) {
    // Create interrupt context
    InterruptContext ctx;
    ctx.int_no = interrupt;
    ctx.err_code = 0;
    ctx.eip = cpu->pc;
    ctx.cs = 0x08;  // Kernel code segment
    ctx.eflags = cpu->flags;
    ctx.eax = cpu->registers[0];
    ctx.ebx = cpu->registers[1];
    ctx.ecx = cpu->registers[2];
    ctx.edx = cpu->registers[3];
    
    // Dispatch to interrupt handler
    interrupt_dispatch(&ctx);
    
    // Restore registers
    cpu->registers[0] = ctx.eax;
    cpu->registers[1] = ctx.ebx;
    cpu->registers[2] = ctx.ecx;
    cpu->registers[3] = ctx.edx;
}

void cpu_run(void) {
    while (cpu.running) {
        uint8_t instruction = memory_read(cpu.pc);
        cpu_execute_instruction(&cpu, instruction);
        cpu.pc++;
        
        // Simple delay to prevent 100% CPU usage
        Sleep(1);
    }
}

CPUState* cpu_get_state(void) {
    return &cpu;
}