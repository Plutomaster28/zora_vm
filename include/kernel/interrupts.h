#ifndef KERNEL_INTERRUPTS_H
#define KERNEL_INTERRUPTS_H

#include <stdint.h>

// Interrupt vectors (x86-compatible)
#define INT_DIVIDE_ERROR        0x00    // Division by zero
#define INT_DEBUG               0x01    // Debug exception
#define INT_NMI                 0x02    // Non-maskable interrupt
#define INT_BREAKPOINT          0x03    // Breakpoint
#define INT_OVERFLOW            0x04    // Overflow
#define INT_BOUND_RANGE         0x05    // Bound range exceeded
#define INT_INVALID_OPCODE      0x06    // Invalid opcode
#define INT_DEVICE_NOT_AVAIL    0x07    // Device not available
#define INT_DOUBLE_FAULT        0x08    // Double fault
#define INT_COPROCESSOR         0x09    // Coprocessor segment overrun
#define INT_INVALID_TSS         0x0A    // Invalid TSS
#define INT_SEGMENT_NOT_PRESENT 0x0B    // Segment not present
#define INT_STACK_FAULT         0x0C    // Stack segment fault
#define INT_GENERAL_PROTECTION  0x0D    // General protection fault
#define INT_PAGE_FAULT          0x0E    // Page fault
#define INT_RESERVED_0F         0x0F    // Reserved
#define INT_FPU_ERROR           0x10    // x87 FPU error
#define INT_ALIGNMENT_CHECK     0x11    // Alignment check
#define INT_MACHINE_CHECK       0x12    // Machine check
#define INT_SIMD_EXCEPTION      0x13    // SIMD floating-point exception

// Hardware interrupts (IRQs remapped to 0x20-0x2F)
#define INT_IRQ_BASE            0x20    // Base for hardware IRQs
#define INT_TIMER               0x20    // Programmable Interval Timer
#define INT_KEYBOARD            0x21    // Keyboard
#define INT_CASCADE             0x22    // Cascade for slave PIC
#define INT_COM2                0x23    // COM2
#define INT_COM1                0x24    // COM1
#define INT_LPT2                0x25    // LPT2
#define INT_FLOPPY              0x26    // Floppy disk
#define INT_LPT1                0x27    // LPT1
#define INT_RTC                 0x28    // Real-time clock
#define INT_PERIPHERAL_1        0x29    // Peripheral 1
#define INT_PERIPHERAL_2        0x2A    // Peripheral 2
#define INT_PERIPHERAL_3        0x2B    // Peripheral 3
#define INT_MOUSE               0x2C    // PS/2 Mouse
#define INT_COPROCESSOR_ERROR   0x2D    // Coprocessor/FPU
#define INT_PRIMARY_ATA         0x2E    // Primary ATA
#define INT_SECONDARY_ATA       0x2F    // Secondary ATA

// System call interrupt
#define INT_SYSCALL             0x80    // System call interrupt

// Custom device interrupts
#define INT_DEVICE_DISK         0x30    // Virtual disk
#define INT_DEVICE_NETWORK      0x31    // Virtual network
#define INT_DEVICE_TERMINAL     0x32    // Virtual terminal

#define MAX_INTERRUPTS          256     // Total interrupt vectors

// Interrupt gate types
#define IDT_GATE_TASK           0x5     // Task gate
#define IDT_GATE_INTERRUPT_16   0x6     // 16-bit interrupt gate
#define IDT_GATE_TRAP_16        0x7     // 16-bit trap gate
#define IDT_GATE_INTERRUPT_32   0xE     // 32-bit interrupt gate
#define IDT_GATE_TRAP_32        0xF     // 32-bit trap gate

// Interrupt descriptor table entry
typedef struct {
    uint16_t offset_low;        // Offset bits 0-15
    uint16_t selector;          // Code segment selector
    uint8_t  zero;              // Must be zero
    uint8_t  type_attr;         // Type and attributes
    uint16_t offset_high;       // Offset bits 16-31
} __attribute__((packed)) IDTEntry;

// IDT pointer structure
typedef struct {
    uint16_t limit;             // Size of IDT - 1
    uint32_t base;              // Base address of IDT
} __attribute__((packed)) IDTPointer;

// CPU interrupt frame (pushed by CPU on interrupt)
typedef struct {
    uint32_t eip;               // Instruction pointer
    uint32_t cs;                // Code segment
    uint32_t eflags;            // Flags register
    uint32_t esp;               // Stack pointer (if privilege change)
    uint32_t ss;                // Stack segment (if privilege change)
} __attribute__((packed)) InterruptFrame;

// Full saved context for interrupt
typedef struct {
    // Pushed by our interrupt handler
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;               // Kernel ESP
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    
    // Interrupt number and error code
    uint32_t int_no;
    uint32_t err_code;
    
    // Pushed by CPU
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;          // Only if privilege change
    uint32_t ss;                // Only if privilege change
} __attribute__((packed)) InterruptContext;

// Interrupt handler function type
typedef void (*InterruptHandler)(InterruptContext* context);

// Interrupt statistics
typedef struct {
    uint64_t total_interrupts;
    uint64_t handled_interrupts;
    uint64_t spurious_interrupts;
    uint64_t nested_interrupts;
    uint64_t interrupt_counts[MAX_INTERRUPTS];
} InterruptStats;

// Interrupt controller state
typedef struct {
    IDTEntry idt[MAX_INTERRUPTS];
    InterruptHandler handlers[MAX_INTERRUPTS];
    IDTPointer idt_ptr;
    InterruptStats stats;
    int interrupts_enabled;
    int nested_level;
    uint32_t interrupt_mask;
} InterruptController;

// Initialization
int interrupts_init(void);
void interrupts_cleanup(void);

// IDT management
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);
void idt_load(void);

// Interrupt handler registration
void interrupt_register_handler(uint8_t int_no, InterruptHandler handler);
void interrupt_unregister_handler(uint8_t int_no);

// Interrupt control
void interrupts_enable(void);
void interrupts_disable(void);
int interrupts_are_enabled(void);
void interrupts_set_mask(uint32_t mask);
uint32_t interrupts_get_mask(void);

// Interrupt handling
void interrupt_dispatch(InterruptContext* context);
void interrupt_eoi(uint8_t int_no);  // End of interrupt

// Exception handlers
void exception_divide_error(InterruptContext* ctx);
void exception_debug(InterruptContext* ctx);
void exception_nmi(InterruptContext* ctx);
void exception_breakpoint(InterruptContext* ctx);
void exception_overflow(InterruptContext* ctx);
void exception_bound_range(InterruptContext* ctx);
void exception_invalid_opcode(InterruptContext* ctx);
void exception_device_not_available(InterruptContext* ctx);
void exception_double_fault(InterruptContext* ctx);
void exception_invalid_tss(InterruptContext* ctx);
void exception_segment_not_present(InterruptContext* ctx);
void exception_stack_fault(InterruptContext* ctx);
void exception_general_protection(InterruptContext* ctx);
void exception_page_fault(InterruptContext* ctx);
void exception_fpu_error(InterruptContext* ctx);
void exception_alignment_check(InterruptContext* ctx);
void exception_machine_check(InterruptContext* ctx);
void exception_simd_exception(InterruptContext* ctx);

// Hardware interrupt handlers
void irq_timer(InterruptContext* ctx);
void irq_keyboard(InterruptContext* ctx);
void irq_cascade(InterruptContext* ctx);
void irq_com2(InterruptContext* ctx);
void irq_com1(InterruptContext* ctx);
void irq_lpt2(InterruptContext* ctx);
void irq_floppy(InterruptContext* ctx);
void irq_lpt1(InterruptContext* ctx);
void irq_rtc(InterruptContext* ctx);
void irq_mouse(InterruptContext* ctx);
void irq_coprocessor(InterruptContext* ctx);
void irq_primary_ata(InterruptContext* ctx);
void irq_secondary_ata(InterruptContext* ctx);

// System call handler
void interrupt_syscall_handler(InterruptContext* ctx);

// Statistics
InterruptStats* interrupts_get_stats(void);
void interrupts_dump_stats(void);

// Timer functions
uint64_t interrupts_get_ticks(void);      // Get system ticks (ms since boot)
uint64_t interrupts_get_uptime(void);     // Get uptime in seconds

// Keyboard functions
uint8_t interrupts_keyboard_getchar(void);   // Get scancode from buffer
int interrupts_keyboard_available(void);     // Check if input available

// Assembly interrupt stubs (declared here, implemented in assembly)
extern void isr_stub_0(void);   // Divide error
extern void isr_stub_1(void);   // Debug
extern void isr_stub_2(void);   // NMI
extern void isr_stub_3(void);   // Breakpoint
extern void isr_stub_4(void);   // Overflow
extern void isr_stub_5(void);   // Bound range
extern void isr_stub_6(void);   // Invalid opcode
extern void isr_stub_7(void);   // Device not available
extern void isr_stub_8(void);   // Double fault
extern void isr_stub_10(void);  // Invalid TSS
extern void isr_stub_11(void);  // Segment not present
extern void isr_stub_12(void);  // Stack fault
extern void isr_stub_13(void);  // General protection
extern void isr_stub_14(void);  // Page fault
extern void isr_stub_16(void);  // FPU error
extern void isr_stub_17(void);  // Alignment check
extern void isr_stub_18(void);  // Machine check
extern void isr_stub_19(void);  // SIMD exception

// IRQ stubs
extern void irq_stub_0(void);   // Timer
extern void irq_stub_1(void);   // Keyboard
extern void irq_stub_14(void);  // Primary ATA
extern void irq_stub_15(void);  // Secondary ATA

// Syscall stub
extern void isr_stub_0x80(void); // System call

#endif // KERNEL_INTERRUPTS_H
