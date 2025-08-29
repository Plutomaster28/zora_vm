#ifndef KERNEL_H
#define KERNEL_H

#include <windows.h>
#include <stdint.h>

// Kernel version information
#define ZORA_KERNEL_VERSION_MAJOR 2
#define ZORA_KERNEL_VERSION_MINOR 1
#define ZORA_KERNEL_VERSION_PATCH 0
#define ZORA_KERNEL_BUILD_DATE __DATE__
#define ZORA_KERNEL_BUILD_TIME __TIME__

// System limits
#define MAX_PROCESSES 256
#define MAX_THREADS_PER_PROCESS 32
#define MAX_OPEN_FILES 1024
#define KERNEL_STACK_SIZE (64 * 1024)
#define USER_STACK_SIZE (16 * 1024)

// Kernel subsystem states
typedef enum {
    KERNEL_STATE_INITIALIZING,
    KERNEL_STATE_RUNNING,
    KERNEL_STATE_SHUTTING_DOWN,
    KERNEL_STATE_HALTED,
    KERNEL_STATE_PANIC
} KernelState;

// Boot flags
#define BOOT_FLAG_SAFE_MODE     0x01
#define BOOT_FLAG_DEBUG_MODE    0x02
#define BOOT_FLAG_VERBOSE       0x04
#define BOOT_FLAG_NO_NETWORK    0x08

// Kernel statistics
typedef struct {
    uint64_t boot_time;
    uint64_t uptime_ticks;
    uint32_t process_count;
    uint32_t thread_count;
    uint32_t interrupt_count;
    uint32_t syscall_count;
    uint32_t context_switches;
    uint64_t memory_allocated;
    uint64_t memory_free;
} KernelStats;

// Kernel panic information
typedef struct {
    uint32_t error_code;
    uint32_t fault_address;
    const char* panic_message;
    uint32_t call_stack[16];
    uint32_t stack_depth;
} PanicInfo;

// Core kernel functions
int kernel_main(void);
int kernel_start(void);
int kernel_early_init(uint32_t boot_flags);
int kernel_late_init(void);
void kernel_shutdown(void);
void kernel_panic(uint32_t error_code, const char* message);

// Kernel state management
int kernel_is_running(void);
KernelState kernel_get_state(void);
void kernel_set_state(KernelState state);

// Boot and initialization
int kernel_init(void);  // Main kernel initialization function
int kernel_check_hardware(void);
int kernel_init_memory_manager(void);
int kernel_init_scheduler(void);
int kernel_init_device_manager(void);
int kernel_init_filesystem(void);
int kernel_init_network_stack(void);

// System information
void kernel_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch);
const char* kernel_get_build_info(void);
KernelStats* kernel_get_stats(void);
uint64_t kernel_get_uptime(void);

// Kernel logging
void kernel_log(const char* subsystem, const char* format, ...);
void kernel_debug(const char* format, ...);
void kernel_warning(const char* format, ...);
void kernel_error(const char* format, ...);

// Timer and scheduling
void kernel_timer_tick(void);
uint64_t kernel_get_tick_count(void);
void kernel_schedule(void);

#endif // KERNEL_H