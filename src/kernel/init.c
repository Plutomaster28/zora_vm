#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "kernel/init.h"
#include "kernel/system_monitor.h"

static int init_stage = 0;
static time_t boot_start_time;

void init_system_start(void) {
    boot_start_time = time(NULL);
    init_stage = 0;
    
    printf("[INIT] ZoraVM Init System v1.0\n");
    printf("[INIT] Starting system initialization...\n");
}

void init_display_progress(const char* service, int stage, int total) {
    printf("[INIT] [%d/%d] Starting %s...", stage, total, service);
    fflush(stdout);
    
    // Simulate startup time
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        // Small delay for effect
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif
    }
    printf(" OK\n");
}

void init_start_services(void) {
    printf("[INIT] Starting system services...\n");
    
    const char* services[] = {
        "kernel-core",
        "memory-manager", 
        "device-manager",
        "virtual-filesystem",
        "network-stack",
        "authentication",
        "terminal-manager",
        "sandbox-security",
        "lua-engine",
        "system-monitor"
    };
    
    int service_count = sizeof(services) / sizeof(services[0]);
    
    for (int i = 0; i < service_count; i++) {
        init_display_progress(services[i], i + 1, service_count);
        init_stage++;
    }
    
    // Initialize system monitor
    system_monitor_init();
    
    printf("[INIT] All services started successfully\n");
    
    time_t boot_end_time = time(NULL);
    double boot_time = difftime(boot_end_time, boot_start_time);
    printf("[INIT] System boot completed in %.2f seconds\n", boot_time);
    printf("[INIT] ZoraVM is ready for user interaction\n");
}

void init_display_boot_logo(void) {
    printf("\n");
    printf("    ███████╗ ██████╗ ██████╗  █████╗ ██╗   ██╗███╗   ███╗\n");
    printf("    ╚══███╔╝██╔═══██╗██╔══██╗██╔══██╗██║   ██║████╗ ████║\n");
    printf("      ███╔╝ ██║   ██║██████╔╝███████║██║   ██║██╔████╔██║\n");
    printf("     ███╔╝  ██║   ██║██╔══██╗██╔══██║╚██╗ ██╔╝██║╚██╔╝██║\n");
    printf("    ███████╗╚██████╔╝██║  ██║██║  ██║ ╚████╔╝ ██║ ╚═╝ ██║\n");
    printf("    ╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝     ╚═╝\n");
    printf("\n");
    printf("           Virtual Machine Operating System v2.1.0\n");
    printf("           Advanced Multi-User Unix-like Environment\n");
    printf("\n");
}

int init_get_boot_stage(void) {
    return init_stage;
}
