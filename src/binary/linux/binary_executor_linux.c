#include "binary_executor.h"
#include "sandbox.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// Linux implementation of binary executor
static int binary_executor_initialized = 0;
static char sandbox_root[MAX_PATH];

void crash_handler(int sig) {
    printf("SANDBOX ALERT: Binary crashed with signal %d\n", sig);
    printf("This may indicate a sandbox escape attempt or malicious code\n");
    exit(1);
}

int binary_executor_init(void) {
    if (binary_executor_initialized) {
        return 0;
    }
    
    printf("Initializing Linux binary executor with sandbox protection...\n");
    
    // Install crash handlers
    signal(SIGSEGV, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGABRT, crash_handler);
    
    binary_executor_initialized = 1;
    printf("Linux binary executor initialized\n");
    return 0;
}

// Add all the other function implementations for Linux...
void binary_executor_cleanup(void) {
    binary_executor_initialized = 0;
}

int binary_executor_is_initialized(void) {
    return binary_executor_initialized;
}

// ... implement all other functions from the header