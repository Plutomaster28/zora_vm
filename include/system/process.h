#ifndef ZORA_PROCESS_H
#define ZORA_PROCESS_H

#include <stdint.h>
#include <time.h>

#define MAX_PROCESSES 256
#define MAX_PROCESS_NAME 128
#define MAX_PROCESS_ARGS 1024

// Process states
typedef enum {
    PROC_STATE_RUNNING,
    PROC_STATE_SLEEPING,
    PROC_STATE_STOPPED,
    PROC_STATE_ZOMBIE,
    PROC_STATE_DEAD
} ProcessState;

// Process priority levels
typedef enum {
    PROC_PRIORITY_REALTIME = 0,
    PROC_PRIORITY_HIGH = 1,
    PROC_PRIORITY_NORMAL = 2,
    PROC_PRIORITY_LOW = 3,
    PROC_PRIORITY_IDLE = 4
} ProcessPriority;

// Process structure
typedef struct Process {
    int pid;                        // Process ID
    int ppid;                       // Parent process ID
    char name[MAX_PROCESS_NAME];    // Process name
    char args[MAX_PROCESS_ARGS];    // Command line arguments
    ProcessState state;             // Current state
    ProcessPriority priority;       // Process priority
    uint64_t memory_used;           // Memory usage in bytes
    float cpu_percent;              // CPU usage percentage
    time_t start_time;              // Process start time
    time_t cpu_time;                // Total CPU time
    int exit_code;                  // Exit code (for zombies)
    struct Process* next;           // Linked list pointer
} Process;

// Process table
typedef struct {
    Process* processes[MAX_PROCESSES];
    int next_pid;
    int process_count;
} ProcessTable;

// Process management functions
int process_init(void);
void process_cleanup(void);

// Process creation and termination
int process_create(const char* name, const char* args, ProcessPriority priority);
int process_kill(int pid, int signal);
int process_wait(int pid, int* exit_code);

// Process information
Process* process_get(int pid);
Process* process_get_by_name(const char* name);
int process_list(Process*** out_list);
int process_count(void);

// Process state management
int process_set_state(int pid, ProcessState state);
ProcessState process_get_state(int pid);
int process_set_priority(int pid, ProcessPriority priority);

// Process statistics
int process_get_memory_usage(int pid);
float process_get_cpu_usage(int pid);
time_t process_get_runtime(int pid);

// Process tree operations
int process_get_children(int ppid, int** child_pids);
int process_get_parent(int pid);

// Process signals (simplified)
#define PROC_SIG_TERM  15  // Termination signal
#define PROC_SIG_KILL   9  // Force kill
#define PROC_SIG_STOP  19  // Stop signal
#define PROC_SIG_CONT  18  // Continue signal

int process_send_signal(int pid, int signal);

// Process search and filtering
int process_find_by_state(ProcessState state, int** pid_list);
int process_find_by_name_pattern(const char* pattern, int** pid_list);

// System-wide process information
uint64_t process_get_total_memory(void);
int process_get_running_count(void);

#endif // ZORA_PROCESS_H
