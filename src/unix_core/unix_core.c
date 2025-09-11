#include <stdio.h>
#include "unix_core.h"

int unix_core_init(void) {
    printf("[UNIX] Initializing Research UNIX Tenth Edition Environment...\n");
    
    // Initialize directory structure first
    if (unix_directories_init() != 0) {
        printf("[UNIX] ERROR: Failed to initialize directory structure\n");
        return 1;
    }
    
    // Initialize compiler toolchain
    if (unix_compiler_init() != 0) {
        printf("[UNIX] ERROR: Failed to initialize compiler toolchain\n");
        return 1;
    }
    
    // Initialize IPC system
    if (unix_ipc_init() != 0) {
        printf("[UNIX] ERROR: Failed to initialize IPC system\n");
        return 1;
    }
    
    // Initialize text processing
    if (unix_textproc_init() != 0) {
        printf("[UNIX] ERROR: Failed to initialize text processing\n");
        return 1;
    }
    
    // Initialize games collection
    if (unix_games_init() != 0) {
        printf("[UNIX] ERROR: Failed to initialize games collection\n");
        return 1;
    }
    
    printf("[UNIX] Research UNIX Tenth Edition initialized successfully\n");
    return 0;
}

void unix_core_cleanup(void) {
    printf("[UNIX] Shutting down Research UNIX environment...\n");
    
    unix_games_cleanup();
    unix_textproc_cleanup();
    unix_ipc_cleanup();
    unix_compiler_cleanup();
    unix_directories_cleanup();
    
    printf("[UNIX] Research UNIX shutdown complete\n");
}

void unix_show_system_info(void) {
    printf("ZoraVM Research UNIX Tenth Edition\n");
    printf("==================================\n");
    printf("\n");
    printf("System Information:\n");
    printf("  UNIX Version: Research UNIX Tenth Edition (ZoraVM)\n");
    printf("  Kernel: ZoraVM Virtual Kernel v1.0\n");
    printf("  Shell: MERL Enhanced Shell with UNIX compatibility\n");
    printf("  Compiler: C, Fortran, Assembly toolchain\n");
    printf("  IPC: Message queues, semaphores, shared memory\n");
    printf("  Text Processing: sed, awk, nroff, troff\n");
    printf("  Games: Classic UNIX games collection\n");
    printf("\n");
    printf("Available UNIX commands:\n");
    printf("  cc, f77, as, ld     - Compiler toolchain\n");
    printf("  yacc, lex           - Parser generators\n");
    printf("  sed, awk, nroff     - Text processing\n");
    printf("  ipcs, msgctl        - IPC utilities\n");
    printf("  fortune, games      - Entertainment\n");
    printf("  man, ls, cat, pwd   - Standard utilities\n");
    printf("\n");
    printf("Directories:\n");
    printf("  /usr/man           - Manual pages\n");
    printf("  /usr/games         - Games and utilities\n");
    printf("  /usr/src           - Source code\n");
    printf("  /usr/lib           - Libraries\n");
    printf("  /usr/bin           - Binaries\n");
    printf("  /usr/ipc           - IPC resources\n");
    printf("  /zora              - ZoraVM specific\n");
    printf("\n");
}
