#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"

#define MAX_PROCESSES 10

// Simulated Process structure
typedef struct {
    int active;
    char description[128];
} Process;

static Process process_table[MAX_PROCESSES];

// Task launcher
int fork_task(const char *desc) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].active) {
            process_table[i].active = 1;
            snprintf(process_table[i].description, sizeof(process_table[i].description), "%s", desc);
            printf("[KERNEL] Task [%d] started: %s\n", i, desc);
            return i;
        }
    }
    printf("[KERNEL] No available slots for new task.\n");
    return -1;
}

void kill_task(int pid) {
    if (pid < 0 || pid >= MAX_PROCESSES || !process_table[pid].active) {
        printf("[KERNEL] Error: Invalid process ID.\n");
        return;
    }
    printf("[KERNEL] Task [%d] terminated: %s\n", pid, process_table[pid].description);
    process_table[pid].active = 0;
    memset(process_table[pid].description, 0, sizeof(process_table[pid].description));
}

void list_tasks() {
    printf("[KERNEL] Active tasks:\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active) {
            printf("  Task ID: %d | Description: %s\n", i, process_table[i].description);
        }
    }
}

// File I/O simulation
void kernel_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("[KERNEL] Error reading file");
        return;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    fclose(file);
}

void kernel_write(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("[KERNEL] Error writing to file");
        return;
    }
    fprintf(file, "%s", content);
    fclose(file);
    printf("[KERNEL] Content written to %s\n", filename);
}

// Command routing
void route_command(const char *command, int argc, char **argv) {
    if (strcmp(command, "fork") == 0) {
        if (argc < 2 || argv[1] == NULL) {
            printf("Usage: fork <description>\n");
            return;
        }
        fork_task(argv[1]);
    } else if (strcmp(command, "kill") == 0) {
        if (argc < 2 || argv[1] == NULL) {
            printf("Usage: kill <task_id>\n");
            return;
        }
        kill_task(atoi(argv[1]));
    } else if (strcmp(command, "ps") == 0) {
        list_tasks();
    } else if (strcmp(command, "read") == 0) {
        if (argc < 2 || argv[1] == NULL) {
            printf("Usage: read <filename>\n");
            return;
        }
        kernel_read(argv[1]);
    } else if (strcmp(command, "write") == 0) {
        if (argc < 3 || argv[1] == NULL || argv[2] == NULL) {
            printf("Usage: write <filename> <content>\n");
            return;
        }
        kernel_write(argv[1], argv[2]);
    } else {
        printf("[KERNEL] Unknown kernel command: %s\n", command);
    }
}
