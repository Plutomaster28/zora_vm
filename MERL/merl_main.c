#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h> // For Sleep function on Windows
#include "shell.h"   // Include the shell header
#include "config.h"  // Include the new config header

// Function prototypes
void start_up_beep() {
    // Beep sound for startup
    Beep(800, 500); // Frequency 1000 Hz for 500 ms
}
void boot_sequence();
void shell_loop();

// Function to set a memory limit using a Windows Job Object
void set_memory_limit_mb(SIZE_T mb) {
    HANDLE hJob = CreateJobObject(NULL, NULL);
    if (hJob == NULL) {
        fprintf(stderr, "Failed to create Job Object\n");
        exit(1);
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    jeli.ProcessMemoryLimit = mb * 1024 * 1024;

    if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
        fprintf(stderr, "Failed to set memory limit\n");
        CloseHandle(hJob);
        exit(1);
    }

    if (!AssignProcessToJobObject(hJob, GetCurrentProcess())) {
        fprintf(stderr, "Failed to assign process to Job Object\n");
        CloseHandle(hJob);
        exit(1);
    }
}

int main() {
    set_memory_limit_mb(TOTAL_MEMORY_MB); // Set memory limit before anything else

    // Call the startup beep
    start_up_beep();

    // Call the boot sequence
    boot_sequence();

    // Enter the shell loop
    shell_loop();

    return 0;
}

void boot_sequence() {
    const char spinner[] = "/-\\|"; // Spinner characters
    int spinner_index = 0;

    printf("=== MERL ===\n");
    printf("MERL Boot v0.1\n");

    // Display firmware version and BIOS
    printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    printf("BIOS: %s\n", BIOS);

    // Display timestamps
    time_t now = time(NULL);
    printf("Timestamp: %s", ctime(&now));

    // System check with spinner effect
    printf("Performing system check...\n");

    printf("Memory: ");
    for (int i = 0; i < 3; i++) { // Simulate delay
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(300); // 300ms delay
    }
    printf("%d MB OK\n", TOTAL_MEMORY_MB);

    printf("Users: ");
    for (int i = 0; i < 3; i++) {
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(300);
    }
    printf("Initialized\n");

    printf("Processes: ");
    for (int i = 0; i < 3; i++) {
        printf("%c\b", spinner[spinner_index++ % 4]);
        fflush(stdout);
        Sleep(300);
    }
    printf("Initialized\n");

    // Boot complete
    printf("Boot sequence complete.\n\n");
}

void shell_loop() {
    printf("Launching shell...\n");

    // Call the shell loop function from shell.c
    start_shell();
}
