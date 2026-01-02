#include "system/process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static ProcessTable g_process_table = {0};

// Initialize process management system
int process_init(void) {
    memset(&g_process_table, 0, sizeof(ProcessTable));
    g_process_table.next_pid = 1;
    g_process_table.process_count = 0;
    
    // Create init process (PID 1)
    Process* init = (Process*)calloc(1, sizeof(Process));
    if (!init) return -1;
    
    init->pid = 1;
    init->ppid = 0;
    strcpy(init->name, "init");
    strcpy(init->args, "init");
    init->state = PROC_STATE_RUNNING;
    init->priority = PROC_PRIORITY_HIGH;
    init->start_time = time(NULL);
    init->memory_used = 0;
    init->cpu_percent = 0.0f;
    
    g_process_table.processes[0] = init;
    g_process_table.process_count = 1;
    g_process_table.next_pid = 2;
    
    return 0;
}

void process_cleanup(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i]) {
            free(g_process_table.processes[i]);
            g_process_table.processes[i] = NULL;
        }
    }
    g_process_table.process_count = 0;
    g_process_table.next_pid = 1;
}

// Create a new process
int process_create(const char* name, const char* args, ProcessPriority priority) {
    if (!name || g_process_table.process_count >= MAX_PROCESSES) {
        return -1;
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!g_process_table.processes[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return -1;
    
    // Create process
    Process* proc = (Process*)calloc(1, sizeof(Process));
    if (!proc) return -1;
    
    proc->pid = g_process_table.next_pid++;
    proc->ppid = 1; // Default parent is init
    strncpy(proc->name, name, MAX_PROCESS_NAME - 1);
    if (args) {
        strncpy(proc->args, args, MAX_PROCESS_ARGS - 1);
    }
    proc->state = PROC_STATE_RUNNING;
    proc->priority = priority;
    proc->start_time = time(NULL);
    proc->memory_used = 1024 * 1024; // Default 1MB
    proc->cpu_percent = 0.0f;
    
    g_process_table.processes[slot] = proc;
    g_process_table.process_count++;
    
    return proc->pid;
}

// Kill a process
int process_kill(int pid, int signal) {
    Process* proc = process_get(pid);
    if (!proc) return -1;
    
    if (pid == 1) {
        // Cannot kill init
        return -1;
    }
    
    if (signal == PROC_SIG_KILL || signal == PROC_SIG_TERM) {
        // Mark as zombie first
        proc->state = PROC_STATE_ZOMBIE;
        proc->exit_code = signal;
        
        // After a "cleanup period", actually remove it
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (g_process_table.processes[i] && 
                g_process_table.processes[i]->pid == pid) {
                free(g_process_table.processes[i]);
                g_process_table.processes[i] = NULL;
                g_process_table.process_count--;
                return 0;
            }
        }
    } else if (signal == PROC_SIG_STOP) {
        proc->state = PROC_STATE_STOPPED;
    } else if (signal == PROC_SIG_CONT) {
        proc->state = PROC_STATE_RUNNING;
    }
    
    return 0;
}

// Get process by PID
Process* process_get(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->pid == pid) {
            return g_process_table.processes[i];
        }
    }
    return NULL;
}

// Get process by name
Process* process_get_by_name(const char* name) {
    if (!name) return NULL;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            strcmp(g_process_table.processes[i]->name, name) == 0) {
            return g_process_table.processes[i];
        }
    }
    return NULL;
}

// List all processes
int process_list(Process*** out_list) {
    if (!out_list) return -1;
    
    *out_list = (Process**)malloc(g_process_table.process_count * sizeof(Process*));
    if (!*out_list) return -1;
    
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES && count < g_process_table.process_count; i++) {
        if (g_process_table.processes[i]) {
            (*out_list)[count++] = g_process_table.processes[i];
        }
    }
    
    return count;
}

int process_count(void) {
    return g_process_table.process_count;
}

// State management
int process_set_state(int pid, ProcessState state) {
    Process* proc = process_get(pid);
    if (!proc) return -1;
    
    proc->state = state;
    return 0;
}

ProcessState process_get_state(int pid) {
    Process* proc = process_get(pid);
    return proc ? proc->state : PROC_STATE_DEAD;
}

int process_set_priority(int pid, ProcessPriority priority) {
    Process* proc = process_get(pid);
    if (!proc) return -1;
    
    proc->priority = priority;
    return 0;
}

// Statistics
int process_get_memory_usage(int pid) {
    Process* proc = process_get(pid);
    return proc ? proc->memory_used : 0;
}

float process_get_cpu_usage(int pid) {
    Process* proc = process_get(pid);
    return proc ? proc->cpu_percent : 0.0f;
}

time_t process_get_runtime(int pid) {
    Process* proc = process_get(pid);
    if (!proc) return 0;
    
    return time(NULL) - proc->start_time;
}

// Tree operations
int process_get_children(int ppid, int** child_pids) {
    if (!child_pids) return -1;
    
    // Count children first
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->ppid == ppid) {
            count++;
        }
    }
    
    if (count == 0) {
        *child_pids = NULL;
        return 0;
    }
    
    *child_pids = (int*)malloc(count * sizeof(int));
    if (!*child_pids) return -1;
    
    int idx = 0;
    for (int i = 0; i < MAX_PROCESSES && idx < count; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->ppid == ppid) {
            (*child_pids)[idx++] = g_process_table.processes[i]->pid;
        }
    }
    
    return count;
}

int process_get_parent(int pid) {
    Process* proc = process_get(pid);
    return proc ? proc->ppid : -1;
}

// Signals
int process_send_signal(int pid, int signal) {
    return process_kill(pid, signal);
}

// Search and filtering
int process_find_by_state(ProcessState state, int** pid_list) {
    if (!pid_list) return -1;
    
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->state == state) {
            count++;
        }
    }
    
    if (count == 0) {
        *pid_list = NULL;
        return 0;
    }
    
    *pid_list = (int*)malloc(count * sizeof(int));
    if (!*pid_list) return -1;
    
    int idx = 0;
    for (int i = 0; i < MAX_PROCESSES && idx < count; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->state == state) {
            (*pid_list)[idx++] = g_process_table.processes[i]->pid;
        }
    }
    
    return count;
}

int process_find_by_name_pattern(const char* pattern, int** pid_list) {
    if (!pattern || !pid_list) return -1;
    
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            strstr(g_process_table.processes[i]->name, pattern)) {
            count++;
        }
    }
    
    if (count == 0) {
        *pid_list = NULL;
        return 0;
    }
    
    *pid_list = (int*)malloc(count * sizeof(int));
    if (!*pid_list) return -1;
    
    int idx = 0;
    for (int i = 0; i < MAX_PROCESSES && idx < count; i++) {
        if (g_process_table.processes[i] && 
            strstr(g_process_table.processes[i]->name, pattern)) {
            (*pid_list)[idx++] = g_process_table.processes[i]->pid;
        }
    }
    
    return count;
}

// System-wide stats
uint64_t process_get_total_memory(void) {
    uint64_t total = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i]) {
            total += g_process_table.processes[i]->memory_used;
        }
    }
    return total;
}

int process_get_running_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table.processes[i] && 
            g_process_table.processes[i]->state == PROC_STATE_RUNNING) {
            count++;
        }
    }
    return count;
}
