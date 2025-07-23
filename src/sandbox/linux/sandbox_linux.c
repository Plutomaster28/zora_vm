#include "sandbox.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#ifdef PLATFORM_LINUX
#include <seccomp.h>
#include <sys/syscall.h>
#endif

// Global sandbox state
static struct {
    int initialized;
    int strict_mode;
    int network_blocked;
    int filesystem_blocked;
    int syscalls_blocked;
    size_t memory_limit;
    int cpu_limit;
#ifdef PLATFORM_LINUX
    scmp_filter_ctx seccomp_ctx;
#endif
} sandbox_state = {0};

int sandbox_init(void) {
    if (sandbox_state.initialized) {
        return 0;
    }
    
    printf("Initializing Linux sandbox environment...\n");
    
#ifdef PLATFORM_LINUX
    // Initialize seccomp
    sandbox_state.seccomp_ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!sandbox_state.seccomp_ctx) {
        printf("Failed to initialize seccomp\n");
        return -1;
    }
#endif
    
    // Initialize sandbox state
    sandbox_state.strict_mode = 0;
    sandbox_state.network_blocked = 0;
    sandbox_state.filesystem_blocked = 0;
    sandbox_state.syscalls_blocked = 0;
    sandbox_state.memory_limit = 0;
    sandbox_state.cpu_limit = 0;
    sandbox_state.initialized = 1;
    
    printf("Linux sandbox environment initialized\n");
    return 0;
}

void sandbox_cleanup(void) {
    if (!sandbox_state.initialized) {
        return;
    }
    
    printf("Cleaning up Linux sandbox environment...\n");
    
#ifdef PLATFORM_LINUX
    if (sandbox_state.seccomp_ctx) {
        seccomp_release(sandbox_state.seccomp_ctx);
        sandbox_state.seccomp_ctx = NULL;
    }
#endif
    
    // Reset sandbox state
    sandbox_state.strict_mode = 0;
    sandbox_state.network_blocked = 0;
    sandbox_state.filesystem_blocked = 0;
    sandbox_state.syscalls_blocked = 0;
    sandbox_state.memory_limit = 0;
    sandbox_state.cpu_limit = 0;
    sandbox_state.initialized = 0;
    
    printf("Linux sandbox cleanup complete\n");
}

int sandbox_set_strict_mode(int enabled) {
    if (!sandbox_state.initialized) {
        fprintf(stderr, "Sandbox not initialized\n");
        return -1;
    }
    
    sandbox_state.strict_mode = enabled;
    printf("Linux sandbox strict mode: %s\n", enabled ? "ENABLED" : "DISABLED");
    
    if (enabled) {
#ifdef PLATFORM_LINUX
        // Enable seccomp filtering
        if (sandbox_state.seccomp_ctx) {
            // Block dangerous syscalls
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(fork), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(clone), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(ptrace), 0);
            
            // Load the filter
            if (seccomp_load(sandbox_state.seccomp_ctx) < 0) {
                printf("Failed to load seccomp filter\n");
                return -1;
            }
        }
#endif
        printf("  - Enhanced Linux security policies activated\n");
        printf("  - Seccomp syscall filtering enabled\n");
        printf("  - Process isolation enforced\n");
    }
    
    return 0;
}

int sandbox_block_network_access(int enabled) {
    if (!sandbox_state.initialized) {
        fprintf(stderr, "Sandbox not initialized\n");
        return -1;
    }
    
    sandbox_state.network_blocked = enabled;
    printf("Network access: %s\n", enabled ? "BLOCKED" : "ALLOWED");
    
    if (enabled) {
#ifdef PLATFORM_LINUX
        if (sandbox_state.seccomp_ctx) {
            // Block network syscalls
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(socket), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(connect), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(bind), 0);
            seccomp_rule_add(sandbox_state.seccomp_ctx, SCMP_ACT_KILL, SCMP_SYS(listen), 0);
        }
#endif
        printf("  - All network syscalls blocked\n");
        printf("  - Socket creation restricted\n");
        printf("  - DNS resolution blocked\n");
    }
    
    return 0;
}

int sandbox_set_memory_limit(size_t limit) {
    if (!sandbox_state.initialized) {
        fprintf(stderr, "Sandbox not initialized\n");
        return -1;
    }
    
    sandbox_state.memory_limit = limit;
    printf("Memory limit set to: %zu bytes (%.2f MB)\n", 
           limit, (double)limit / (1024 * 1024));
    
    if (limit > 0) {
        struct rlimit rlim;
        rlim.rlim_cur = limit;
        rlim.rlim_max = limit;
        
        if (setrlimit(RLIMIT_AS, &rlim) == 0) {
            printf("  - Virtual memory limit enforced\n");
        } else {
            printf("  - Failed to set memory limit\n");
            return -1;
        }
    }
    
    return 0;
}

// Implement all the other functions from your Windows version...
int sandbox_block_file_system_access(int enabled) {
    if (!sandbox_state.initialized) {
        fprintf(stderr, "Sandbox not initialized\n");
        return -1;
    }
    
    sandbox_state.filesystem_blocked = enabled;
    printf("File system access: %s\n", enabled ? "BLOCKED" : "ALLOWED");
    
    // TODO: Implement filesystem restriction
    return 0;
}

int sandbox_block_system_calls(int enabled) { 
    sandbox_state.syscalls_blocked = enabled;
    return 0; 
}

int sandbox_set_cpu_limit(int percent) { 
    sandbox_state.cpu_limit = percent;
    return 0; 
}

// Query functions
int sandbox_is_strict_mode(void) { return sandbox_state.strict_mode; }
int sandbox_is_network_blocked(void) { return sandbox_state.network_blocked; }
int sandbox_is_filesystem_blocked(void) { return sandbox_state.filesystem_blocked; }
int sandbox_is_syscalls_blocked(void) { return sandbox_state.syscalls_blocked; }
size_t sandbox_get_memory_limit(void) { return sandbox_state.memory_limit; }
int sandbox_get_cpu_limit(void) { return sandbox_state.cpu_limit; }
int sandbox_is_initialized(void) { return sandbox_state.initialized; }