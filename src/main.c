#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>  // Add this include
#include <locale.h>
#include <time.h>

// Windows-specific includes for directory handling
#include <windows.h>
#include <direct.h>

#include "cpu.h"
#include "memory.h"
#include "device.h"
#include "kernel.h"
#include "sandbox.h"
#include "merl.h"
#include "vfs/vfs.h"
#include "syscall.h"
#include "virtualization.h"
#include "vm.h"
#include "network/network.h"
#include "lua/lua_vm.h"
#include "binary/binary_executor.h"
#include "meisei/virtual_silicon.h"
#include "terminal/terminal_detector.h"
#include "unix_core.h"
#include "unix_core/unix_embedded_compiler.h"

#ifdef PYTHON_SCRIPTING
#include "python/python_vm.h"
#endif
#ifdef PERL_SCRIPTING
#include "perl/perl_vm.h"
#endif

static volatile int running = 1;
static volatile int rebooting = 0;  // Flag for reboot
static int vm_initialized = 0;

// Add crash protection variables
static volatile int vm_crash_guard = 0;
static jmp_buf vm_main_loop;

// Enhanced signal handler with crash protection
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

// VM crash protection handler
void vm_crash_handler(int sig) {
    if (vm_crash_guard) {
        printf("CRITICAL: VM crash guard triggered - preventing VM death\n");
        printf("Signal: %d\n", sig);
        printf("This indicates a serious bug in binary execution\n");
        
        // Force cleanup and return to VM shell
        binary_executor_cleanup();
        // Note: cleanup_sandbox_environment() might not exist, so use sandbox_cleanup()
        sandbox_cleanup();
        
        // Reset crash guard temporarily to prevent infinite loops
        vm_crash_guard = 0;
        
        // Don't exit - return to VM shell
        printf("Attempting to recover and return to VM shell...\n");
        longjmp(vm_main_loop, 1);
    } else {
        // If crash guard is off, use normal signal handler
        signal_handler(sig);
    }
}

// Initialize crash protection (but keep it disabled by default)
void vm_init_crash_protection() {
    printf("Initializing VM crash protection...\n");
    signal(SIGSEGV, vm_crash_handler);
    signal(SIGABRT, vm_crash_handler);
    signal(SIGFPE, vm_crash_handler);
    signal(SIGILL, vm_crash_handler);  // Add illegal instruction
    vm_crash_guard = 0;  // Start disabled - only enable during risky operations
    printf("VM crash protection installed (disabled by default)\n");
}

// Enable crash guard for risky operations
void vm_enable_crash_guard() {
    vm_crash_guard = 1;
}

// Disable crash guard for normal operations
void vm_disable_crash_guard() {
    vm_crash_guard = 0;
}

// Trigger VM reboot
void vm_trigger_reboot() {
    printf("VM reboot triggered...\n");
    rebooting = 1;
    running = 0;  // Stop the main loop
}

int vm_init(void) {
    if (vm_initialized) {
        return 0;
    }
    printf("VM environment initialized\n");
    vm_initialized = 1;
    return 0;
}

void vm_cleanup(void) {
    if (vm_initialized) {
        printf("Syncing persistent storage before shutdown...\n");
        vfs_sync_all();
        printf("VM environment cleaned up\n");
        vm_initialized = 0;
    }
}

int vm_is_running(void) {
    return vm_initialized;
}

int main(int argc, char* argv[]) {
    // Record startup time for UTF-8 debugging
    time_t startup_time = time(NULL);
    printf("[MAIN] ZoraVM starting at timestamp %ld\n", startup_time);
    
    // Set locale and console encoding for proper Unicode/box-drawing character support
    setlocale(LC_ALL, "");
    
    // Windows-specific console encoding fixes
    #ifdef _WIN32
    printf("[MAIN] Setting up UTF-8 console encoding...\n");
    
    // Set console to UTF-8 for proper box-drawing characters
    if (SetConsoleOutputCP(CP_UTF8)) {
        printf("[MAIN] Console output set to UTF-8 (CP_UTF8)\n");
    } else {
        printf("[MAIN] WARNING: Failed to set console output to UTF-8\n");
    }
    
    if (SetConsoleCP(CP_UTF8)) {
        printf("[MAIN] Console input set to UTF-8 (CP_UTF8)\n");
    } else {
        printf("[MAIN] WARNING: Failed to set console input to UTF-8\n");
    }
    
    // Enable Unicode/UTF-8 output on Windows console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        printf("[MAIN] Current console mode: 0x%08X\n", dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(hOut, dwMode)) {
            printf("[MAIN] Virtual terminal processing enabled\n");
        } else {
            printf("[MAIN] WARNING: Failed to enable virtual terminal processing\n");
        }
    }
    
    // Force console mode reset to fix UTF-8 rendering issues
    // This mimics what happens during pipe operations that fix the UTF-8
    printf("[MAIN] Forcing console refresh to fix UTF-8 rendering...\n");
    fflush(stdout);
    
    // Instead of freopen, just force a console mode refresh
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout != INVALID_HANDLE_VALUE) {
        DWORD currentMode;
        if (GetConsoleMode(hStdout, &currentMode)) {
            // Force console refresh by toggling a mode bit
            SetConsoleMode(hStdout, currentMode & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            SetConsoleMode(hStdout, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            printf("[MAIN] Console refreshed successfully - UTF-8 should now work\n");
        }
    }
    fflush(stdout);
    
    // Quick UTF-8 test immediately after setup
    printf("[MAIN] UTF-8 test immediately after setup: ╔═══╗\n");
    
    // Detect terminal capabilities early
    int is_modern_terminal = detect_windows_terminal();
    if (!is_modern_terminal) {
        printf("NOTE: Running in legacy Console Host. For best experience, use Windows Terminal.\n");
        printf("Run 'terminal-test' command in ZoraVM to check terminal capabilities.\n");
    } else {
        printf("[MAIN] Detected Windows Terminal - UTF-8 should work properly\n");
    }
    #endif
    
    // Set up signal handlers first
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

#if ZORA_VERBOSE_BOOT
    printf("Starting Zora VM...\n");
#endif

    // Check if running in batch mode (for healthcheck)
    int batch_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--batch-mode") == 0) {
            batch_mode = 1;
            break;
        }
    }
    
    if (batch_mode) {
        printf("Running in batch mode for healthcheck\n");
        // Quick initialization and exit
        return 0;
    }

#if ZORA_RELEASE_MODE && !ZORA_VERBOSE_BOOT
    // Clean release mode startup - minimal output
    printf("=== ZORA VM ===\n");
    printf("ZoraVM Boot v2.1.0\n");
    printf("Firmware Version: %s\n", "ZoraVM-2.1.0");
    
    // Simple boot sequence similar to MERL
    const char spinner[] = "/-\\|";
    int spinner_index = 0;
    
    printf("Initializing virtual machine");
    for (int i = 0; i < 6; i++) {
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(200);
    }
    printf("OK\n");
    
    printf("Loading kernel");
    for (int i = 0; i < 4; i++) {
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(150);
    }
    printf("OK\n");
    
    printf("Mounting virtual filesystem");
    for (int i = 0; i < 3; i++) {
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(100);
    }
    printf("OK\n");
    
    printf("Boot sequence complete.\n");
    printf("========================================\n");
#endif

    // Initialize crash protection early
#if ZORA_VERBOSE_BOOT
    printf("Initializing VM crash protection...\n");
#endif
    vm_init_crash_protection();

    // Set jump point for crash recovery
    if (setjmp(vm_main_loop) != 0) {
        printf("VM recovered from crash, reinitializing...\n");
        // Re-enable crash guard after recovery
        vm_crash_guard = 1;
    }

    // Initialize sandboxing first
#if ZORA_VERBOSE_BOOT
    printf("Initializing sandbox...\n");
#endif
    if (sandbox_init() != 0) {
        fprintf(stderr, "Failed to initialize sandbox.\n");
        return 1;
    }

    // Apply resource limits
#if ZORA_VERBOSE_BOOT
    printf("Setting memory limit to %d MB...\n", MEMORY_SIZE / (1024 * 1024));
#endif
    sandbox_set_memory_limit(MEMORY_SIZE);
    sandbox_set_cpu_limit(80); // 80% CPU limit
    
    // Enable strict sandbox mode
#if ZORA_VERBOSE_BOOT
    printf("Enabling strict sandbox mode...\n");
#endif
    sandbox_set_strict_mode(1);
    sandbox_block_network_access(1);
    sandbox_block_file_system_access(1);
    sandbox_block_system_calls(1);
    
    // Initialize virtualization layer
#if ZORA_VERBOSE_BOOT
    printf("Initializing virtualization layer...\n");
#endif
    if (virtualization_init() != 0) {
        fprintf(stderr, "Failed to initialize virtualization layer.\n");
        sandbox_cleanup();
        return 1;
    }

    // Initialize the virtual machine environment
#if ZORA_VERBOSE_BOOT
    printf("Initializing VM environment...\n");
#endif
    if (vm_init() != 0) {
        fprintf(stderr, "Failed to initialize the virtual machine.\n");
        goto cleanup;
    }

    // Initialize sophisticated kernel first - this shows our OS boot sequence
#if ZORA_VERBOSE_BOOT
    printf("Initializing ZORA Kernel...\n");
#endif
    if (kernel_init() != 0) {
        fprintf(stderr, "Failed to initialize kernel.\n");
        goto cleanup;
    }

    // Set up CPU, memory, and devices within sandbox
#if ZORA_VERBOSE_BOOT
    printf("Initializing CPU...\n");
#endif
    if (cpu_init() != 0) {
        fprintf(stderr, "Failed to initialize CPU.\n");
        goto cleanup;
    }

#if ZORA_VERBOSE_BOOT
    printf("Initializing memory (%d MB)...\n", MEMORY_SIZE / (1024 * 1024));
#endif
    Memory* mem = memory_init(MEMORY_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "CRITICAL: Failed to initialize memory (requested %d MB)\n", MEMORY_SIZE / (1024 * 1024));
        fprintf(stderr, "This could be due to insufficient system memory or memory limits.\n");
        goto cleanup;
    }
#if ZORA_VERBOSE_BOOT
    printf("Memory initialization successful!\n");
#endif

    // Note: Devices are already initialized by the kernel's device manager
    // No need for separate device_init() call

    // Initialize MERL shell within the VM
#if ZORA_VERBOSE_BOOT
    printf("Initializing MERL shell...\n");
#endif
    if (merl_init() != 0) {
        fprintf(stderr, "Failed to initialize MERL shell.\n");
        goto cleanup;
    }

    // Initialize VFS
#if ZORA_VERBOSE_BOOT
    printf("Initializing VFS...\n");
#endif
    if (vfs_init() != 0) {
        fprintf(stderr, "Failed to initialize VFS\n");
        goto cleanup;
    }
    
    // Find ZoraPerl directory relative to executable location for portability
    char zora_perl_path[512];
    char executable_path[512];
    char executable_dir[512];
    
    // Get the full path to the current executable
    DWORD get_module_result = GetModuleFileNameA(NULL, executable_path, sizeof(executable_path));
    if (get_module_result == 0 || get_module_result == sizeof(executable_path)) {
        fprintf(stderr, "ERROR: Failed to get executable path!\n");
        goto cleanup;
    }
    
    // Extract directory from executable path
    strcpy(executable_dir, executable_path);
    char* last_backslash = strrchr(executable_dir, '\\');
    if (last_backslash) {
        *last_backslash = '\0';  // Remove filename, keep directory
    } else {
        strcpy(executable_dir, ".");  // Fallback to current directory
    }
    
    // Construct ZoraPerl path next to executable
    snprintf(zora_perl_path, sizeof(zora_perl_path), "%s\\ZoraPerl", executable_dir);
    
    // Debug: Uncomment the next line if you need to see the executable and ZoraPerl paths
    // printf("Executable at: %s\n", executable_path);
    // printf("ZoraPerl path: %s\n", zora_perl_path);
    
    // Check if ZoraPerl directory exists, create if not
    DWORD attrib = GetFileAttributesA(zora_perl_path);
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
#if ZORA_VERBOSE_BOOT
        printf("Creating ZoraPerl directory at: %s\n", zora_perl_path);
#endif
        if (create_directory_recursive(zora_perl_path) != 0) {
            fprintf(stderr, "ERROR: Failed to create ZoraPerl directory!\n");
            goto cleanup;
        }
#if ZORA_VERBOSE_BOOT
        printf("Successfully created ZoraPerl directory at: %s\n", zora_perl_path);
#endif
    } else {
        // Debug: Uncomment the next line if you need to see the ZoraPerl path
        // printf("Found existing ZoraPerl directory at: %s\n", zora_perl_path);
    }
    
    // Ensure the entire ZoraPerl directory structure exists
    create_directory_recursive(zora_perl_path);
    
    // Create subdirectories using the found path
    char subdir_path[600];
    const char* subdirs[] = {"documents", "scripts", "data", "projects", "bin", "home", "tmp", "etc", "usr", "var"};
    for (int i = 0; i < 10; i++) {
        snprintf(subdir_path, sizeof(subdir_path), "%s\\%s", zora_perl_path, subdirs[i]);
        create_directory_recursive(subdir_path);
    }
    
    // Autodiscover host root-style directories directly under /
    vfs_mount_root_autodiscover(zora_perl_path);

    // Initialize network virtualization
#if ZORA_VERBOSE_BOOT
    printf("Initializing virtual network...\n");
#endif
    if (network_init() != 0) {
        fprintf(stderr, "Failed to initialize virtual network.\n");
        goto cleanup;
    }

    // Initialize Lua scripting engine
#if ZORA_VERBOSE_BOOT
    printf("Initializing Lua scripting engine...\n");
#endif
    if (lua_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Lua VM\n");
        goto cleanup;
    }

#ifdef PYTHON_SCRIPTING
#if ZORA_VERBOSE_BOOT
    printf("Initializing Python scripting engine...\n");
#endif
    if (python_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Python VM\n");
        goto cleanup;
    }
#endif

#ifdef PERL_SCRIPTING
#if ZORA_VERBOSE_BOOT
    printf("Initializing Perl scripting engine...\n");
#endif
    if (perl_vm_init() != 0) {
        fprintf(stderr, "Failed to initialize Perl VM\n");
        goto cleanup;
    }
#endif

    // Initialize embedded compiler system
#if ZORA_VERBOSE_BOOT
    printf("Initializing embedded compiler toolchain...\n");
#endif
    if (embedded_compiler_init() != 0) {
        fprintf(stderr, "Warning: Embedded compiler initialization failed - compilation features may be limited\n");
        // Don't fail the VM startup for compiler issues
    }

#if ZORA_VERBOSE_BOOT
    printf("Initializing binary executor...\n");
#endif
    if (binary_executor_init() != 0) {
        fprintf(stderr, "Failed to initialize binary executor\n");
        goto cleanup;
    }

#if ZORA_VERBOSE_BOOT
    printf("Initializing Meisei Virtual Silicon...\n");
#endif
    if (meisei_silicon_init() != 0) {
        fprintf(stderr, "Failed to initialize Meisei Virtual Silicon\n");
        goto cleanup;
    }

#if ZORA_VERBOSE_BOOT
    printf("Initializing Research UNIX Tenth Edition environment...\n");
#endif
    if (unix_core_init() != 0) {
        fprintf(stderr, "Failed to initialize UNIX core environment\n");
        goto cleanup;
    }

#if ZORA_VERBOSE_BOOT
    printf("Zora VM initialized successfully. Starting MERL shell...\n");
    printf("========================================\n");
#endif

    // Start the MERL shell as the "OS" with crash protection
    int result = 0;
    while (running) {
        if (setjmp(vm_main_loop) != 0) {
            printf("Recovered from crash in MERL shell, restarting...\n");
            // Re-enable crash guard
            vm_crash_guard = 1;
            continue;
        }
        
        result = merl_run();
        
        if (result != 0) {
            fprintf(stderr, "MERL shell execution failed with code: %d\n", result);
        }
        break; // Normal exit
    }

    // Check if we need to reboot
    if (rebooting) {
#if ZORA_VERBOSE_BOOT
        printf("Reboot requested - cleaning up before restart...\n");
#else
        printf("Rebooting ZoraVM...\n");
#endif
        
        // Perform cleanup
        vm_crash_guard = 0;
        merl_cleanup();
        device_cleanup();
        memory_cleanup();
        cpu_cleanup();
        vm_cleanup();
        virtualization_cleanup();
        sandbox_cleanup();
        network_cleanup();
        lua_vm_cleanup();
        vfs_cleanup();
        binary_executor_cleanup();
        meisei_silicon_cleanup();
        
        // Cleanup embedded compiler system
        embedded_compiler_cleanup();
        
#ifdef PYTHON_SCRIPTING
        python_vm_cleanup();
#endif
#ifdef PERL_SCRIPTING
        perl_vm_cleanup();
#endif
        
#if ZORA_VERBOSE_BOOT
        printf("Cleanup complete. Restarting Zora VM...\n");
#endif
        
        // Get the current executable path
        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);
        
        // Create new process to restart the VM
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        
        if (CreateProcessA(NULL, exe_path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
#if ZORA_VERBOSE_BOOT
            printf("VM restart initiated successfully.\n");
#endif
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0; // Exit current instance
        } else {
            printf("Failed to restart VM. Error code: %lu\n", GetLastError());
            printf("Please restart manually.\n");
            return 1;
        }
    }

cleanup:
    // Disable crash guard during cleanup
    vm_crash_guard = 0;
    
    // Clean up resources before exiting
#if ZORA_VERBOSE_BOOT
    printf("\nShutting down Zora VM...\n");
#endif
    
    // If there was an error during initialization, pause to show the error
    #ifdef DEBUG_PAUSE_ON_ERROR
    if (result != 0) {
        printf("Press Enter to continue...");
        getchar();
    }
    #endif
    
    merl_cleanup();
    device_cleanup();
    memory_cleanup();
    cpu_cleanup();
    vm_cleanup();
    virtualization_cleanup();
    sandbox_cleanup();
    network_cleanup();
    lua_vm_cleanup();
    vfs_cleanup();
    binary_executor_cleanup();
    meisei_silicon_cleanup();
    
    // Cleanup embedded compiler system
    embedded_compiler_cleanup();
    
#ifdef PYTHON_SCRIPTING
    python_vm_cleanup();
#endif
#ifdef PERL_SCRIPTING
    perl_vm_cleanup();
#endif
    
#if ZORA_VERBOSE_BOOT
    printf("Zora VM shutdown complete.\n");
#endif
    return 0;
}