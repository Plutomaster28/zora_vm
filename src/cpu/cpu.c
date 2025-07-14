#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "cpu.h"
#include "memory.h"

static CPUState cpu;

int cpu_init(void) {
    cpu.pc = 0;
    cpu.sp = 0x1000; // Stack pointer starts at 4KB
    cpu.flags = 0;
    cpu.running = 1;
    
    // Initialize registers
    for (int i = 0; i < 8; i++) {
        cpu.registers[i] = 0;
    }
    
    printf("CPU initialized\n");
    return 0;
}

void cpu_cleanup(void) {
    cpu.running = 0;
    printf("CPU cleaned up\n");
}

void cpu_execute_instruction(CPUState *cpu, unsigned int instruction) {
    switch (instruction) {
        case 0x00: // NOP
            break;
        case 0x01: // HALT
            cpu->running = 0;
            break;
        default:
            printf("Unknown instruction: 0x%02X\n", instruction);
            break;
    }
}

void cpu_handle_interrupt(CPUState *cpu, unsigned int interrupt) {
    printf("Handling interrupt: %d\n", interrupt);
    // Handle interrupt logic here
}

void cpu_run(void) {
    while (cpu.running) {
        uint8_t instruction = memory_read(cpu.pc);
        cpu_execute_instruction(&cpu, instruction);
        cpu.pc++;
        
        // Simple delay to prevent 100% CPU usage
        #ifdef _WIN32
        Sleep(1);
        #else
        usleep(1000);
        #endif
    }
}

CPUState* cpu_get_state(void) {
    return &cpu;
}