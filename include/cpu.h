#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    uint32_t pc;           // Program counter
    uint32_t sp;           // Stack pointer
    uint32_t flags;        // Flags register
    uint32_t registers[8]; // General purpose registers
    int running;           // CPU running state
} CPUState;

// CPU functions
int cpu_init(void);
void cpu_cleanup(void);
void cpu_execute_instruction(CPUState *cpu, unsigned int instruction);
void cpu_handle_interrupt(CPUState *cpu, unsigned int interrupt);
void cpu_run(void);
CPUState* cpu_get_state(void);

#endif // CPU_H