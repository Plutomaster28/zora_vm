#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// CPU Architecture Constants
#define CPU_REGISTER_COUNT 16
#define CPU_STACK_SIZE 65536
#define CPU_CACHE_L1_SIZE (32 * 1024)
#define CPU_CACHE_L2_SIZE (256 * 1024)
#define CPU_FREQUENCY_MHZ 2400

// CPU Register Names
typedef enum {
    REG_R0 = 0, REG_R1, REG_R2, REG_R3,    // General purpose
    REG_R4, REG_R5, REG_R6, REG_R7,        // General purpose
    REG_SP,                                  // Stack pointer
    REG_BP,                                  // Base pointer
    REG_IP,                                  // Instruction pointer
    REG_FLAGS,                               // Flags register
    REG_CR0, REG_CR1, REG_CR2, REG_CR3     // Control registers
} CPURegister;

// CPU Flags
#define FLAG_ZERO        0x0001
#define FLAG_CARRY       0x0002
#define FLAG_NEGATIVE    0x0004
#define FLAG_OVERFLOW    0x0008
#define FLAG_INTERRUPT   0x0010
#define FLAG_SUPERVISOR  0x0020
#define FLAG_DEBUG       0x0040
#define FLAG_TRACE       0x0080

// CPU States
typedef enum {
    CPU_STATE_RESET,
    CPU_STATE_RUNNING,
    CPU_STATE_HALTED,
    CPU_STATE_INTERRUPT,
    CPU_STATE_EXCEPTION,
    CPU_STATE_DEBUG
} CPUState_t;

// Instruction formats
typedef enum {
    INST_NOP = 0x00,
    INST_HALT = 0x01,
    INST_LOAD = 0x10,
    INST_STORE = 0x11,
    INST_MOVE = 0x12,
    INST_ADD = 0x20,
    INST_SUB = 0x21,
    INST_MUL = 0x22,
    INST_DIV = 0x23,
    INST_AND = 0x30,
    INST_OR = 0x31,
    INST_XOR = 0x32,
    INST_NOT = 0x33,
    INST_CMP = 0x40,
    INST_JMP = 0x50,
    INST_JZ = 0x51,
    INST_JNZ = 0x52,
    INST_CALL = 0x60,
    INST_RET = 0x61,
    INST_PUSH = 0x70,
    INST_POP = 0x71,
    INST_INT = 0x80,
    INST_IRET = 0x81
} InstructionType;

// Interrupt vectors
typedef enum {
    INT_TIMER = 0x00,
    INT_KEYBOARD = 0x01,
    INT_DISK = 0x02,
    INT_NETWORK = 0x03,
    INT_SYSCALL = 0x80,
    INT_PAGE_FAULT = 0x0E,
    INT_GENERAL_FAULT = 0x0D,
    INT_DIVIDE_ERROR = 0x00
} InterruptVector;

// CPU performance counters
typedef struct {
    uint64_t instructions_executed;
    uint64_t cycles_elapsed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t branch_predictions;
    uint64_t branch_mispredictions;
    uint64_t page_faults;
    uint64_t interrupts_handled;
} CPUPerfCounters;

// CPU cache structure
typedef struct {
    uint8_t* data;
    uint32_t* tags;
    uint8_t* valid;
    uint32_t size;
    uint32_t line_size;
    uint32_t associativity;
    uint64_t hits;
    uint64_t misses;
} CPUCache;

// Main CPU state structure
typedef struct {
    // Core registers
    uint32_t registers[CPU_REGISTER_COUNT];
    
    // Program state
    uint32_t pc;           // Program counter (same as IP)
    uint32_t sp;           // Stack pointer
    uint32_t bp;           // Base pointer
    uint32_t flags;        // Flags register
    
    // Control state
    CPUState_t state;
    int running;
    uint32_t privilege_level;
    
    // Performance and debugging
    CPUPerfCounters perf;
    uint64_t cycle_count;
    uint32_t frequency_mhz;
    
    // Cache subsystem
    CPUCache l1_cache;
    CPUCache l2_cache;
    
    // Interrupt handling
    uint32_t interrupt_vector;
    uint32_t interrupt_pending;
    uint32_t interrupt_mask;
    
    // Pipeline state
    uint32_t pipeline_stage;
    uint32_t pipeline_stall;
    
    // Branch prediction
    uint32_t branch_predictor[256];
    uint32_t branch_history;
    
} CPUState;

// CPU functions
int cpu_init(void);
void cpu_cleanup(void);
void cpu_reset(void);

// Execution control
void cpu_run(void);
void cpu_step(void);
void cpu_halt(void);
CPUState* cpu_get_state(void);

// Instruction execution
void cpu_execute_instruction(CPUState *cpu, uint32_t instruction);
uint32_t cpu_fetch_instruction(CPUState *cpu);
void cpu_decode_and_execute(CPUState *cpu, uint32_t instruction);

// Register operations
uint32_t cpu_get_register(CPURegister reg);
void cpu_set_register(CPURegister reg, uint32_t value);
void cpu_dump_registers(void);

// Flag operations
void cpu_set_flag(uint32_t flag);
void cpu_clear_flag(uint32_t flag);
int cpu_test_flag(uint32_t flag);
void cpu_update_flags(uint32_t result);

// Interrupt handling
void cpu_handle_interrupt(CPUState *cpu, InterruptVector vector);
void cpu_set_interrupt_pending(InterruptVector vector);
void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);

// Cache operations
int cpu_cache_read(CPUCache* cache, uint32_t address, uint8_t* data, size_t size);
int cpu_cache_write(CPUCache* cache, uint32_t address, const uint8_t* data, size_t size);
void cpu_cache_flush(CPUCache* cache);
void cpu_cache_invalidate(CPUCache* cache);

// Performance monitoring
CPUPerfCounters* cpu_get_performance_counters(void);
void cpu_reset_performance_counters(void);
double cpu_get_mips(void); // Million Instructions Per Second
double cpu_get_cache_hit_ratio(void);

// Debug and diagnostics
void cpu_print_state(void);
void cpu_print_performance(void);
const char* cpu_get_state_string(void);
const char* cpu_instruction_to_string(uint32_t instruction);

#endif // CPU_H