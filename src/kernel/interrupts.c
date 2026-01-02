#include "kernel/interrupts.h"
#include "kernel/privilege.h"
#include "kernel/scheduler.h"
#include "kernel/mmu.h"
#include <stdio.h>
#include <string.h>

// Global interrupt controller
static InterruptController g_int_controller;
static int g_interrupts_initialized = 0;

// Timer state
static volatile uint64_t g_system_ticks = 0;
static volatile uint64_t g_timer_frequency = 1000;  // 1000 Hz = 1ms per tick

// Keyboard input buffer
#define KB_BUFFER_SIZE 256
static volatile uint8_t g_keyboard_buffer[KB_BUFFER_SIZE];
static volatile int g_kb_read_pos = 0;
static volatile int g_kb_write_pos = 0;
static volatile int g_kb_count = 0;

// Initialize interrupt system
int interrupts_init(void) {
    if (g_interrupts_initialized) {
        return 0;
    }
    
    printf("[INTERRUPTS] Initializing interrupt controller...\n");
    
    memset(&g_int_controller, 0, sizeof(InterruptController));
    
    // Setup IDT pointer
    g_int_controller.idt_ptr.limit = sizeof(g_int_controller.idt) - 1;
    g_int_controller.idt_ptr.base = (uint32_t)&g_int_controller.idt;
    
    // Install exception handlers (0x00-0x1F)
    printf("[INTERRUPTS] Installing exception handlers...\n");
    interrupt_register_handler(INT_DIVIDE_ERROR, exception_divide_error);
    interrupt_register_handler(INT_DEBUG, exception_debug);
    interrupt_register_handler(INT_NMI, exception_nmi);
    interrupt_register_handler(INT_BREAKPOINT, exception_breakpoint);
    interrupt_register_handler(INT_OVERFLOW, exception_overflow);
    interrupt_register_handler(INT_BOUND_RANGE, exception_bound_range);
    interrupt_register_handler(INT_INVALID_OPCODE, exception_invalid_opcode);
    interrupt_register_handler(INT_DEVICE_NOT_AVAIL, exception_device_not_available);
    interrupt_register_handler(INT_DOUBLE_FAULT, exception_double_fault);
    interrupt_register_handler(INT_INVALID_TSS, exception_invalid_tss);
    interrupt_register_handler(INT_SEGMENT_NOT_PRESENT, exception_segment_not_present);
    interrupt_register_handler(INT_STACK_FAULT, exception_stack_fault);
    interrupt_register_handler(INT_GENERAL_PROTECTION, exception_general_protection);
    interrupt_register_handler(INT_PAGE_FAULT, exception_page_fault);
    interrupt_register_handler(INT_FPU_ERROR, exception_fpu_error);
    interrupt_register_handler(INT_ALIGNMENT_CHECK, exception_alignment_check);
    interrupt_register_handler(INT_MACHINE_CHECK, exception_machine_check);
    interrupt_register_handler(INT_SIMD_EXCEPTION, exception_simd_exception);
    
    // Install IRQ handlers (0x20-0x2F)
    printf("[INTERRUPTS] Installing IRQ handlers...\n");
    interrupt_register_handler(INT_TIMER, irq_timer);
    interrupt_register_handler(INT_KEYBOARD, irq_keyboard);
    
    // Install syscall handler (0x80)
    printf("[INTERRUPTS] Installing syscall handler...\n");
    interrupt_register_handler(INT_SYSCALL, interrupt_syscall_handler);
    
    // Load IDT (simulated for now)
    idt_load();
    
    g_interrupts_initialized = 1;
    g_int_controller.interrupts_enabled = 1;
    
    printf("[INTERRUPTS] Initialized successfully\n");
    
    return 0;
}

// Cleanup interrupt system
void interrupts_cleanup(void) {
    if (!g_interrupts_initialized) return;
    
    g_interrupts_initialized = 0;
    printf("[INTERRUPTS] Cleaned up\n");
}

// Set IDT gate
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    g_int_controller.idt[num].offset_low = handler & 0xFFFF;
    g_int_controller.idt[num].offset_high = (handler >> 16) & 0xFFFF;
    g_int_controller.idt[num].selector = selector;
    g_int_controller.idt[num].zero = 0;
    g_int_controller.idt[num].type_attr = flags;
}

// Load IDT
void idt_load(void) {
    // In real kernel, would execute LIDT instruction
    // For simulation, just log it
    printf("[INTERRUPTS] IDT loaded at 0x%08X (size: %d bytes)\n",
           g_int_controller.idt_ptr.base, g_int_controller.idt_ptr.limit + 1);
}

// Register interrupt handler
void interrupt_register_handler(uint8_t int_no, InterruptHandler handler) {
    if (int_no >= MAX_INTERRUPTS) return;
    
    g_int_controller.handlers[int_no] = handler;
    
    // Setup IDT gate
    // Flags: Present=1, DPL=0 (kernel), Type=0xE (32-bit interrupt gate)
    uint8_t flags = 0x8E;
    
    // For syscall (0x80), allow user mode access (DPL=3)
    if (int_no == INT_SYSCALL) {
        flags = 0xEE;
    }
    
    idt_set_gate(int_no, (uint32_t)handler, 0x08, flags);  // 0x08 = kernel code segment
}

// Unregister interrupt handler
void interrupt_unregister_handler(uint8_t int_no) {
    if (int_no >= MAX_INTERRUPTS) return;
    
    g_int_controller.handlers[int_no] = NULL;
}

// Enable interrupts
void interrupts_enable(void) {
    g_int_controller.interrupts_enabled = 1;
    privilege_sti();  // Set interrupt flag
}

// Disable interrupts
void interrupts_disable(void) {
    g_int_controller.interrupts_enabled = 0;
    privilege_cli();  // Clear interrupt flag
}

// Check if interrupts are enabled
int interrupts_are_enabled(void) {
    return g_int_controller.interrupts_enabled;
}

// Set interrupt mask
void interrupts_set_mask(uint32_t mask) {
    g_int_controller.interrupt_mask = mask;
}

// Get interrupt mask
uint32_t interrupts_get_mask(void) {
    return g_int_controller.interrupt_mask;
}

// Dispatch interrupt to handler
void interrupt_dispatch(InterruptContext* context) {
    if (!context) return;
    
    uint8_t int_no = context->int_no;
    
    // Update statistics
    g_int_controller.stats.total_interrupts++;
    if (int_no < MAX_INTERRUPTS) {
        g_int_controller.stats.interrupt_counts[int_no]++;
    }
    
    // Switch to kernel mode
    privilege_enter_kernel_mode();
    
    // Track nested interrupts
    g_int_controller.nested_level++;
    if (g_int_controller.nested_level > 1) {
        g_int_controller.stats.nested_interrupts++;
    }
    
    // Call handler if registered
    if (int_no < MAX_INTERRUPTS && g_int_controller.handlers[int_no]) {
        g_int_controller.handlers[int_no](context);
        g_int_controller.stats.handled_interrupts++;
    } else {
        printf("[INTERRUPTS] Unhandled interrupt 0x%02X\n", int_no);
        g_int_controller.stats.spurious_interrupts++;
    }
    
    // Send EOI if hardware interrupt
    if (int_no >= INT_IRQ_BASE && int_no < INT_IRQ_BASE + 16) {
        interrupt_eoi(int_no);
    }
    
    g_int_controller.nested_level--;
}

// End of interrupt
void interrupt_eoi(uint8_t int_no) {
    // For hardware IRQs, send EOI to PIC
    // In real kernel, would write to PIC ports 0x20/0xA0
    // For simulation, just log it
}

// Exception handlers
void exception_divide_error(InterruptContext* ctx) {
    printf("[EXCEPTION] Divide Error at EIP=0x%08X\n", ctx->eip);
}

void exception_debug(InterruptContext* ctx) {
    printf("[EXCEPTION] Debug at EIP=0x%08X\n", ctx->eip);
}

void exception_nmi(InterruptContext* ctx) {
    printf("[EXCEPTION] Non-Maskable Interrupt at EIP=0x%08X\n", ctx->eip);
}

void exception_breakpoint(InterruptContext* ctx) {
    printf("[EXCEPTION] Breakpoint at EIP=0x%08X\n", ctx->eip);
}

void exception_overflow(InterruptContext* ctx) {
    printf("[EXCEPTION] Overflow at EIP=0x%08X\n", ctx->eip);
}

void exception_bound_range(InterruptContext* ctx) {
    printf("[EXCEPTION] Bound Range Exceeded at EIP=0x%08X\n", ctx->eip);
}

void exception_invalid_opcode(InterruptContext* ctx) {
    printf("[EXCEPTION] Invalid Opcode at EIP=0x%08X\n", ctx->eip);
    privilege_raise_exception(EXCEPTION_INVALID_OPCODE);
}

void exception_device_not_available(InterruptContext* ctx) {
    printf("[EXCEPTION] Device Not Available at EIP=0x%08X\n", ctx->eip);
}

void exception_double_fault(InterruptContext* ctx) {
    printf("[EXCEPTION] DOUBLE FAULT at EIP=0x%08X (CRITICAL!)\n", ctx->eip);
    // In real kernel, this is catastrophic - usually halt the system
}

void exception_invalid_tss(InterruptContext* ctx) {
    printf("[EXCEPTION] Invalid TSS (error=0x%08X) at EIP=0x%08X\n", ctx->err_code, ctx->eip);
}

void exception_segment_not_present(InterruptContext* ctx) {
    printf("[EXCEPTION] Segment Not Present (error=0x%08X) at EIP=0x%08X\n", ctx->err_code, ctx->eip);
}

void exception_stack_fault(InterruptContext* ctx) {
    printf("[EXCEPTION] Stack Fault (error=0x%08X) at EIP=0x%08X\n", ctx->err_code, ctx->eip);
}

void exception_general_protection(InterruptContext* ctx) {
    printf("[EXCEPTION] General Protection Fault (error=0x%08X) at EIP=0x%08X\n", 
           ctx->err_code, ctx->eip);
    privilege_raise_exception(EXCEPTION_GENERAL_PROTECTION);
}

void exception_page_fault(InterruptContext* ctx) {
    // Get faulting address from CR2 (would be actual CR2 in real kernel)
    uint32_t fault_addr = 0;  // Would read CR2 here
    
    printf("[EXCEPTION] Page Fault (error=0x%08X) at EIP=0x%08X, address=0x%08X\n",
           ctx->err_code, ctx->eip, fault_addr);
    
    // Call MMU page fault handler
    mmu_page_fault_handler(fault_addr, ctx->err_code);
}

void exception_fpu_error(InterruptContext* ctx) {
    printf("[EXCEPTION] x87 FPU Error at EIP=0x%08X\n", ctx->eip);
}

void exception_alignment_check(InterruptContext* ctx) {
    printf("[EXCEPTION] Alignment Check (error=0x%08X) at EIP=0x%08X\n", ctx->err_code, ctx->eip);
}

void exception_machine_check(InterruptContext* ctx) {
    printf("[EXCEPTION] Machine Check at EIP=0x%08X (CRITICAL!)\n", ctx->eip);
}

void exception_simd_exception(InterruptContext* ctx) {
    printf("[EXCEPTION] SIMD Floating-Point Exception at EIP=0x%08X\n", ctx->eip);
}

// Hardware IRQ handlers
void irq_timer(InterruptContext* ctx) {
    // Increment system tick counter
    g_system_ticks++;
    
    // Update scheduler (call every tick)
    scheduler_tick();
    
    // Print tick info every 1000 ticks (1 second at 1000 Hz)
    if (g_system_ticks % 1000 == 0) {
        uint64_t seconds = g_system_ticks / g_timer_frequency;
        printf("[TIMER] System uptime: %llu seconds\n", seconds);
    }
}

void irq_keyboard(InterruptContext* ctx) {
    // Keyboard interrupt - read scancode from controller
    // In real kernel, would read from port 0x60
    // For simulation, we'll generate a dummy scancode
    
    // Simulate reading from keyboard controller (0x60)
    uint8_t scancode = 0x1E;  // Dummy scancode (would be actual IN instruction)
    
    // Add to circular buffer if not full
    if (g_kb_count < KB_BUFFER_SIZE) {
        g_keyboard_buffer[g_kb_write_pos] = scancode;
        g_kb_write_pos = (g_kb_write_pos + 1) % KB_BUFFER_SIZE;
        g_kb_count++;
    } else {
        // Buffer overflow - drop oldest input
        printf("[KEYBOARD] Buffer overflow!\n");
    }
}

void irq_cascade(InterruptContext* ctx) {
    // Cascade interrupt from slave PIC
}

void irq_com2(InterruptContext* ctx) {
    // COM2 serial port
}

void irq_com1(InterruptContext* ctx) {
    // COM1 serial port
}

void irq_lpt2(InterruptContext* ctx) {
    // LPT2 parallel port
}

void irq_floppy(InterruptContext* ctx) {
    // Floppy disk
}

void irq_lpt1(InterruptContext* ctx) {
    // LPT1 parallel port
}

void irq_rtc(InterruptContext* ctx) {
    // Real-time clock
}

void irq_mouse(InterruptContext* ctx) {
    // PS/2 mouse
}

void irq_coprocessor(InterruptContext* ctx) {
    // Coprocessor/FPU
}

void irq_primary_ata(InterruptContext* ctx) {
    // Primary ATA hard disk
}

void irq_secondary_ata(InterruptContext* ctx) {
    // Secondary ATA hard disk
}

// System call handler
void interrupt_syscall_handler(InterruptContext* ctx) {
    // Syscall number in EAX, arguments in EBX, ECX, EDX, ESI, EDI
    // For now, just log it
    printf("[SYSCALL] System call 0x%08X from EIP=0x%08X\n", ctx->eax, ctx->eip);
    
    // Would dispatch to syscall table here
    // Return value goes in EAX
}

// Get statistics
InterruptStats* interrupts_get_stats(void) {
    return &g_int_controller.stats;
}

// Get system ticks (milliseconds since boot)
uint64_t interrupts_get_ticks(void) {
    return g_system_ticks;
}

// Get system uptime in seconds
uint64_t interrupts_get_uptime(void) {
    return g_system_ticks / g_timer_frequency;
}

// Get keyboard scancode from buffer (returns 0 if empty)
uint8_t interrupts_keyboard_getchar(void) {
    if (g_kb_count == 0) {
        return 0;  // Buffer empty
    }
    
    uint8_t scancode = g_keyboard_buffer[g_kb_read_pos];
    g_kb_read_pos = (g_kb_read_pos + 1) % KB_BUFFER_SIZE;
    g_kb_count--;
    
    return scancode;
}

// Check if keyboard buffer has data
int interrupts_keyboard_available(void) {
    return g_kb_count > 0;
}

// Dump statistics
void interrupts_dump_stats(void) {
    printf("[INTERRUPTS] Statistics:\n");
    printf("  Total interrupts: %llu\n", g_int_controller.stats.total_interrupts);
    printf("  Handled: %llu\n", g_int_controller.stats.handled_interrupts);
    printf("  Spurious: %llu\n", g_int_controller.stats.spurious_interrupts);
    printf("  Nested: %llu\n", g_int_controller.stats.nested_interrupts);
    
    printf("\nTop interrupt sources:\n");
    for (int i = 0; i < MAX_INTERRUPTS; i++) {
        if (g_int_controller.stats.interrupt_counts[i] > 0) {
            printf("  INT 0x%02X: %llu\n", i, g_int_controller.stats.interrupt_counts[i]);
        }
    }
}

// Stub implementations for assembly functions (these would be in .asm file)
void isr_stub_0(void) { InterruptContext ctx = {0}; ctx.int_no = 0; interrupt_dispatch(&ctx); }
void isr_stub_1(void) { InterruptContext ctx = {0}; ctx.int_no = 1; interrupt_dispatch(&ctx); }
void isr_stub_2(void) { InterruptContext ctx = {0}; ctx.int_no = 2; interrupt_dispatch(&ctx); }
void isr_stub_3(void) { InterruptContext ctx = {0}; ctx.int_no = 3; interrupt_dispatch(&ctx); }
void isr_stub_4(void) { InterruptContext ctx = {0}; ctx.int_no = 4; interrupt_dispatch(&ctx); }
void isr_stub_5(void) { InterruptContext ctx = {0}; ctx.int_no = 5; interrupt_dispatch(&ctx); }
void isr_stub_6(void) { InterruptContext ctx = {0}; ctx.int_no = 6; interrupt_dispatch(&ctx); }
void isr_stub_7(void) { InterruptContext ctx = {0}; ctx.int_no = 7; interrupt_dispatch(&ctx); }
void isr_stub_8(void) { InterruptContext ctx = {0}; ctx.int_no = 8; interrupt_dispatch(&ctx); }
void isr_stub_10(void) { InterruptContext ctx = {0}; ctx.int_no = 10; interrupt_dispatch(&ctx); }
void isr_stub_11(void) { InterruptContext ctx = {0}; ctx.int_no = 11; interrupt_dispatch(&ctx); }
void isr_stub_12(void) { InterruptContext ctx = {0}; ctx.int_no = 12; interrupt_dispatch(&ctx); }
void isr_stub_13(void) { InterruptContext ctx = {0}; ctx.int_no = 13; interrupt_dispatch(&ctx); }
void isr_stub_14(void) { InterruptContext ctx = {0}; ctx.int_no = 14; interrupt_dispatch(&ctx); }
void isr_stub_16(void) { InterruptContext ctx = {0}; ctx.int_no = 16; interrupt_dispatch(&ctx); }
void isr_stub_17(void) { InterruptContext ctx = {0}; ctx.int_no = 17; interrupt_dispatch(&ctx); }
void isr_stub_18(void) { InterruptContext ctx = {0}; ctx.int_no = 18; interrupt_dispatch(&ctx); }
void isr_stub_19(void) { InterruptContext ctx = {0}; ctx.int_no = 19; interrupt_dispatch(&ctx); }

void irq_stub_0(void) { InterruptContext ctx = {0}; ctx.int_no = INT_TIMER; interrupt_dispatch(&ctx); }
void irq_stub_1(void) { InterruptContext ctx = {0}; ctx.int_no = INT_KEYBOARD; interrupt_dispatch(&ctx); }
void irq_stub_14(void) { InterruptContext ctx = {0}; ctx.int_no = INT_PRIMARY_ATA; interrupt_dispatch(&ctx); }
void irq_stub_15(void) { InterruptContext ctx = {0}; ctx.int_no = INT_SECONDARY_ATA; interrupt_dispatch(&ctx); }

void isr_stub_0x80(void) { InterruptContext ctx = {0}; ctx.int_no = INT_SYSCALL; interrupt_dispatch(&ctx); }
