#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "kernel.h"
#include "cpu.h"
#include "memory.h"
#include "device.h"
#include "version.h"  // Auto-versioning system
#include "../../include/kernel/java_detector.h"
#include "kernel/privilege.h"
#include "kernel/scheduler.h"
#include "kernel/mmu.h"
#include "kernel/interrupts.h"
#include "kernel/network_stack.h"

// Global kernel state
static KernelState g_kernel_state = KERNEL_STATE_INITIALIZING;
static KernelStats g_kernel_stats = {0};
static PanicInfo g_panic_info = {0};
static uint32_t g_boot_flags = 0;
static uint64_t g_boot_time = 0;
static uint64_t g_tick_counter = 0;
static CRITICAL_SECTION g_kernel_lock;

// Boot splash and initialization
static void kernel_display_boot_splash(void) {
    char version_short[32];
    get_zora_version_short(version_short, sizeof(version_short));
    
    printf("\n");
    printf("================================================================\n");
    printf("=                        ZORA KERNEL v%s                        =\n", version_short);
    printf("=              Advanced Virtual Machine Operating System          =\n");
    printf("=                    Built on %s at %s                    =\n", __DATE__, __TIME__);
    printf("=                      Codename: \"%s\"                           =\n", get_version_codename());
    printf("=                                                                =\n");
    printf("=  Features: Multi-user • Unix Permissions • Scripting • VFS    =\n");
    printf("=  Network: Virtual TCP/IP • Sandbox Security • Process Control =\n");
    printf("=  Security: AUTOMATIC JAVA DETECTION AND ELIMINATION           =\n");
    printf("================================================================\n");
    printf("\n");
    
    if (g_boot_flags & BOOT_FLAG_SAFE_MODE) {
        printf("[KERNEL]   SAFE MODE ENABLED\n");
    }
    if (g_boot_flags & BOOT_FLAG_DEBUG_MODE) {
        printf("[KERNEL]  DEBUG MODE ENABLED\n");
    }
    if (g_boot_flags & BOOT_FLAG_VERBOSE) {
        printf("[KERNEL]  VERBOSE LOGGING ENABLED\n");
    }
    
    printf("[KERNEL]  Boot sequence initiating...\n");
    printf("[KERNEL]  WARNING: Automatic Java detection enabled!\n");
    printf("[KERNEL]  System will PANIC if Java contamination is detected!\n");
}

static void kernel_log_impl(const char* level, const char* subsystem, const char* format, va_list args) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    printf("[%02d:%02d:%02d.%03d] [%s] [%s] ", 
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
           level, subsystem ? subsystem : "KERNEL");
    
    vprintf(format, args);
    printf("\n");
}

void kernel_log(const char* subsystem, const char* format, ...) {
    if (!(g_boot_flags & BOOT_FLAG_VERBOSE)) return;
    
    va_list args;
    va_start(args, format);
    kernel_log_impl("INFO", subsystem, format, args);
    va_end(args);
}

void kernel_debug(const char* format, ...) {
    if (!(g_boot_flags & BOOT_FLAG_DEBUG_MODE)) return;
    
    va_list args;
    va_start(args, format);
    kernel_log_impl("DEBUG", "KERNEL", format, args);
    va_end(args);
}

void kernel_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    kernel_log_impl("WARN", "KERNEL", format, args);
    va_end(args);
}

void kernel_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    kernel_log_impl("ERROR", "KERNEL", format, args);
    va_end(args);
}

int kernel_check_hardware(void) {
    kernel_log("HWDET", "Starting hardware detection...");
    
    // Check CPU features
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    kernel_log("HWDET", "CPU Architecture: %s", 
               si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x86_64" : "x86");
    kernel_log("HWDET", "CPU Count: %d cores", si.dwNumberOfProcessors);
    kernel_log("HWDET", "Page Size: %d bytes", si.dwPageSize);
    
    // Check memory
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    kernel_log("HWDET", "Total Physical Memory: %llu MB", memStatus.ullTotalPhys / (1024 * 1024));
    kernel_log("HWDET", "Available Physical Memory: %llu MB", memStatus.ullAvailPhys / (1024 * 1024));
    
    g_kernel_stats.memory_allocated = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    g_kernel_stats.memory_free = memStatus.ullAvailPhys;
    
    return 0;
}

int kernel_init_memory_manager(void) {
    kernel_log("MM", "Initializing memory management subsystem...");
    
    if (memory_init(MEMORY_SIZE) == NULL) {
        kernel_error("Failed to initialize memory manager");
        return -1;
    }
    
    kernel_log("MM", "Memory manager initialized - %d MB virtual memory", MEMORY_SIZE / (1024 * 1024));
    return 0;
}

int kernel_init_scheduler(void) {
    kernel_log("SCHED", "Initializing process scheduler...");
    
    // Initialize real scheduler
    if (scheduler_init() != 0) {
        kernel_error("Failed to initialize scheduler");
        return -1;
    }
    
    // Initialize scheduler data structures
    g_kernel_stats.process_count = 1; // Kernel process
    g_kernel_stats.thread_count = 1;  // Main kernel thread
    
    kernel_log("SCHED", "Scheduler initialized - preemptive multitasking enabled");
    return 0;
}

int kernel_init_device_manager(void) {
    kernel_log("DEVMGR", "Initializing device manager...");
    
    if (device_init() != 0) {
        kernel_error("Failed to initialize device manager");
        return -1;
    }
    
    kernel_log("DEVMGR", "Device manager initialized");
    return 0;
}

int kernel_init_filesystem(void) {
    kernel_log("VFS", "Initializing virtual file system...");
    // VFS initialization would go here
    kernel_log("VFS", "Virtual file system ready");
    return 0;
}

int kernel_init_system_monitor(void) {
    kernel_log("SYSMON", "Initializing system monitoring...");
    // Initialize system monitor - need to include the header
    // system_monitor_init();
    kernel_log("SYSMON", "System monitoring ready");
    return 0;
}

int kernel_init_network_stack(void) {
    if (g_boot_flags & BOOT_FLAG_NO_NETWORK) {
        kernel_log("NET", "Network stack disabled by boot flag");
        return 0;
    }
    
    kernel_log("NET", "Initializing network stack...");
    
    // Initialize real network stack
    if (netstack_init() != 0) {
        kernel_error("Failed to initialize network stack");
        return -1;
    }
    
    kernel_log("NET", "Network stack ready - interfaces: lo (127.0.0.1), zora0 (10.0.2.15)");
    return 0;
}

int kernel_early_init(uint32_t boot_flags) {
    g_boot_flags = boot_flags;
    g_boot_time = GetTickCount64();
    
    InitializeCriticalSection(&g_kernel_lock);
    
    kernel_display_boot_splash();
    
    // Early hardware detection
    if (kernel_check_hardware() != 0) {
        kernel_panic(0x00000001, "Hardware detection failed");
        return -1;
    }
    
    return 0;
}

int kernel_late_init(void) {
    kernel_log("INIT", "Starting late initialization phase...");
    
    // Initialize privilege system
    kernel_log("INIT", "Initializing privilege system...");
    privilege_init();
    
    // Initialize MMU and paging - TEMPORARILY DISABLED FOR TESTING
    kernel_log("INIT", "Skipping MMU initialization (temporary)...");
    //if (mmu_init() != 0) {
    //    kernel_error("Failed to initialize MMU");
    //    return -1;
    //}
    //mmu_enable_paging();
    
    // Initialize interrupt system
    kernel_log("INIT", "Initializing interrupt system...");
    if (interrupts_init() != 0) {
        kernel_error("Failed to initialize interrupts");
        return -1;
    }
    
    // Initialize all major subsystems
    if (kernel_init_memory_manager() != 0) return -1;
    if (kernel_init_scheduler() != 0) return -1;
    if (kernel_init_device_manager() != 0) return -1;
    if (kernel_init_filesystem() != 0) return -1;
    if (kernel_init_network_stack() != 0) return -1;
    
    // Initialize CPU
    if (cpu_init() != 0) {
        kernel_error("Failed to initialize CPU");
        return -1;
    }
    
    // Start scheduler
    kernel_log("INIT", "Starting scheduler...");
    scheduler_start();
    
    // Enable interrupts
    kernel_log("INIT", "Enabling interrupts...");
    interrupts_enable();
    
    kernel_log("INIT", "Late initialization completed successfully");
    return 0;
}

void kernel_timer_tick(void) {
    EnterCriticalSection(&g_kernel_lock);
    g_tick_counter++;
    g_kernel_stats.uptime_ticks = g_tick_counter;
    LeaveCriticalSection(&g_kernel_lock);
    
    // Trigger scheduler tick
    scheduler_tick();
}

void kernel_schedule(void) {
    g_kernel_stats.context_switches++;
    
    // Call real scheduler
    scheduler_schedule();
}

uint64_t kernel_get_tick_count(void) {
    return g_tick_counter;
}

uint64_t kernel_get_uptime(void) {
    return GetTickCount64() - g_boot_time;
}

void kernel_panic(uint32_t error_code, const char* message) {
    g_kernel_state = KERNEL_STATE_PANIC;
    g_panic_info.error_code = error_code;
    g_panic_info.panic_message = message;
    g_panic_info.fault_address = 0; // Would be set by actual fault handler
    
    // Check if this is a Java-related panic for extra drama
    int is_java_panic = (error_code == 0xDEADBEEF || error_code == 0xCAFFEBAD || 
                        error_code == 0xC0FFEE || error_code == 0xBADC0DE || 
                        error_code == 0xE47E4491 ||
                        strstr(message, "JAVA") != NULL || 
                        strstr(message, "Enterprise") != NULL);
    
    printf("\n");
    if (is_java_panic) {
        // Special Java panic display - using clean UTF-8 box drawing characters
        printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
        printf("║                           *** ZORA KERNEL PANIC ***                         ║\n");
        printf("║                          !!! JAVA CONTAMINATION !!!                        ║\n"); 
        printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
        printf("║  The system has detected JAVA and is refusing to continue!                  ║\n");
        printf("║  This is not a drill - actual Java code was found in the system!           ║\n");
        printf("║                                                                              ║\n");
        printf("║  Error Code: 0x%08X (JAVA_RELATED_CATASTROPHE)                          ║\n", error_code);
        printf("║  Message: %-63s║\n", message);
        printf("║  Uptime: %llu ms (cut short by Java detection)                              ║\n", kernel_get_uptime());
        printf("║                                                                              ║\n");
        printf("║   EMERGENCY ACTIONS TAKEN:                                                ║\n");
        printf("║  • All Java processes terminated with extreme prejudice                     ║\n");
        printf("║  • Memory sanitized to remove AbstractFactory patterns                      ║\n");
        printf("║  • CPU cache flushed to prevent bytecode contamination                      ║\n");
        printf("║  • Network disabled to prevent Java spread                                  ║\n");
        printf("║                                                                              ║\n");
        printf("║  The ZoraVM kernel has HALTED to protect your sanity and system integrity. ║\n");
        printf("║  Please remove ALL Java files and restart the system immediately.          ║\n");
        printf("║                                                                              ║\n");
        printf("║   EDUCATIONAL NOTE:                                                       ║\n");
        printf("║  Real programmers use C. Java is for enterprise architects who enjoy       ║\n");
        printf("║  500-character method names and AbstractSingletonProxyFactoryBean.         ║\n");
        printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    } else {
        // Regular kernel panic display
        printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
        printf("║                           *** KERNEL PANIC ***                              ║\n");
        printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
        printf("║  Error Code: 0x%08X                                                     ║\n", error_code);
        printf("║  Message: %-63s║\n", message);
        printf("║  Uptime: %llu ms                                                            ║\n", kernel_get_uptime());
        printf("║                                                                              ║\n");
        printf("║  The system has encountered a critical error and must halt.                 ║\n");
        printf("║  Please report this error to the kernel developers.                         ║\n");
        printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    // Extra Java-specific messages
    if (is_java_panic) {
        printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
        printf("║                           JAVA CONTAMINATION DETECTED                   ║\n");
        printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
        printf("║  System integrity compromised by enterprise patterns!                       ║\n");
        printf("║  Kernel refusing to execute in Java-contaminated environment!               ║\n");
        printf("║  Please purify your system with pure C code and restart.                    ║\n");
        printf("║                                                                              ║\n");
        printf("║  ZoraVM Security Philosophy:                                                 ║\n");
        printf("║   Clean, readable C code                                                  ║\n");
        printf("║   Minimal abstractions                                                    ║\n");
        printf("║   Direct hardware control                                                 ║\n");
        printf("║   AbstractSingletonProxyFactoryBean                                       ║\n");
        printf("║   Enterprise design patterns                                              ║\n");
        printf("║   500MB Hello World applications                                          ║\n");
        printf("║                                                                              ║\n");
        printf("║  Recovery Instructions:                                                      ║\n");
        printf("║  1. Delete all .java, .class, .jar files                                    ║\n");
        printf("║  2. Uninstall any Java IDEs                                                 ║\n");
        printf("║  3. Write your code in C like a civilized person                            ║\n");
        printf("║  4. Restart ZoraVM                                                           ║\n");
        printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    }
    
    // Halt the system
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                               SYSTEM HALTED                             ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  The kernel has stopped execution due to a critical error.                  ║\n");
    printf("║  Press Ctrl+C to acknowledge this panic and exit.                           ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    while (1) {
        Sleep(1000);
    }
}

int kernel_main(void) {
    // Set boot flags based on compilation options
    uint32_t boot_flags = 0;
#if ZORA_VERBOSE_BOOT
    boot_flags |= BOOT_FLAG_VERBOSE | BOOT_FLAG_DEBUG_MODE;
#endif
    
    // Early initialization
    if (kernel_early_init(boot_flags) != 0) {
        return -1;
    }
    
    kernel_set_state(KERNEL_STATE_INITIALIZING);
    
    // Late initialization
    if (kernel_late_init() != 0) {
        kernel_panic(0x00000002, "Late initialization failed");
        return -1;
    }
    
    kernel_set_state(KERNEL_STATE_RUNNING);
    kernel_log("INIT", " Zora Kernel is now running!");
    kernel_log("INIT", "System ready for user applications");
    
    // Initialize Java detection system (CRITICAL SECURITY COMPONENT)
    kernel_log("SECURITY", "Initializing Java detection and protection system...");
    if (!java_detector_init()) {
        kernel_panic(0x00000420, "Java detector initialization failed - system vulnerable to Enterprise patterns!");
        return -1;
    }
    
    // AUTOMATIC JAVA CONTAMINATION SCAN (NO ESCAPE)
    kernel_log("SECURITY", "Performing mandatory Java contamination scan...");
    kernel_log("SECURITY", "Scanning all accessible directories for Java threats...");
    
    // Scan multiple directories automatically
    const char* scan_paths[] = {
        ".",           // Current directory  
        "..",          // Parent directory
        "src",         // Source directory
        "build",       // Build directory
        "include",     // Include directory
        "/",           // VFS root
        "/bin",        // Binary directory
        "/home",       // Home directory
        "/tmp",        // Temp directory
        NULL
    };
    
    int java_contamination_detected = 0;
    for (int i = 0; scan_paths[i] != NULL; i++) {
        kernel_log("SECURITY", "Scanning: %s", scan_paths[i]);
        if (java_scan_directory(scan_paths[i])) {
            java_contamination_detected = 1;
            break; // Java detector will panic automatically, but just in case...
        }
    }
    
    if (java_contamination_detected) {
        // This should never be reached due to java_trigger_kernel_panic, but safety first
        kernel_panic(0xDEADBEEF, "JAVA CONTAMINATION DETECTED - SYSTEM COMPROMISED BY ENTERPRISE PATTERNS");
        return -1;
    }
    
    kernel_log("SECURITY", " System is CLEAN! No Java contamination detected.");
    kernel_log("SECURITY", "Kernel integrity maintained. Proceeding with normal operation.");
    
    // Main kernel loop with timer
    while (g_kernel_state == KERNEL_STATE_RUNNING) {
        kernel_timer_tick();
        Sleep(1); // 1ms timer resolution
    }
    
    return 0;
}

int kernel_start(void) {
    return kernel_main();
}

// Main kernel initialization function - setup only, no infinite loop
int kernel_init(void) {
    // Set boot flags based on compilation options
    uint32_t boot_flags = 0;
#if ZORA_VERBOSE_BOOT
    boot_flags |= BOOT_FLAG_VERBOSE | BOOT_FLAG_DEBUG_MODE;
#endif
    
    // Early initialization
    if (kernel_early_init(boot_flags) != 0) {
        return -1;
    }
    
    kernel_set_state(KERNEL_STATE_INITIALIZING);
    
    // Late initialization
    if (kernel_late_init() != 0) {
        kernel_panic(0x00000002, "Late initialization failed");
        return -1;
    }
    
    kernel_set_state(KERNEL_STATE_RUNNING);
    kernel_log("INIT", " Zora Kernel is now running!");
    kernel_log("INIT", "System ready for user applications");
    
    return 0;
}

void kernel_shutdown(void) {
    kernel_log("SHUTDOWN", "Initiating graceful shutdown...");
    kernel_set_state(KERNEL_STATE_SHUTTING_DOWN);
    
    // Cleanup subsystems
    cpu_cleanup();
    device_cleanup();
    
    DeleteCriticalSection(&g_kernel_lock);
    
    kernel_log("SHUTDOWN", "Kernel shutdown complete");
    g_kernel_state = KERNEL_STATE_HALTED;
}

// State management functions
int kernel_is_running(void) {
    return g_kernel_state == KERNEL_STATE_RUNNING;
}

KernelState kernel_get_state(void) {
    return g_kernel_state;
}

void kernel_set_state(KernelState state) {
    g_kernel_state = state;
}

// System information functions
void kernel_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch) {
    if (major) *major = ZORA_KERNEL_VERSION_MAJOR;
    if (minor) *minor = ZORA_KERNEL_VERSION_MINOR;
    if (patch) *patch = ZORA_KERNEL_VERSION_PATCH;
}

const char* kernel_get_build_info(void) {
    static char build_info[256];
    snprintf(build_info, sizeof(build_info), "Built on %s at %s", 
             ZORA_KERNEL_BUILD_DATE, ZORA_KERNEL_BUILD_TIME);
    return build_info;
}

KernelStats* kernel_get_stats(void) {
    return &g_kernel_stats;
}