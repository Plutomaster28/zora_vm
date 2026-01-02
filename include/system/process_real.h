// include/system/process_real.h - Real process management using Windows API
#ifndef PROCESS_REAL_H
#define PROCESS_REAL_H

#include <stdint.h>
#include <time.h>
#include <windows.h>

#define MAX_REAL_PROCESSES 512
#define MAX_PROCESS_NAME_REAL 256
#define MAX_PROCESS_ARGS 1024

// Process states (real)
typedef enum {
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_STOPPED,
    PROC_ZOMBIE,
    PROC_DEAD
} RealProcessState;

// Process priority (real)
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} RealProcessPriority;

// Signals
#define PROC_SIG_KILL  9
#define PROC_SIG_TERM 15
#define PROC_SIG_STOP 19
#define PROC_SIG_CONT 18
#define PROC_SIG_INT   2

// Real process structure - maps to actual Windows process
typedef struct {
    // Unix-style fields
    int pid;                          // ZoraVM PID (Unix-style)
    int ppid;                         // Parent PID
    char name[MAX_PROCESS_NAME_REAL]; // Process name
    char args[MAX_PROCESS_ARGS];      // Command line arguments
    char cwd[MAX_PROCESS_ARGS];       // Current working directory
    RealProcessState state;           // Process state
    RealProcessPriority priority;     // Priority level
    time_t start_time;                // Start time
    int exit_code;                    // Exit code
    uint64_t memory_used;             // Memory usage in bytes
    float cpu_percent;                // CPU usage percentage
    
    // Windows-specific fields (THE REAL STUFF)
    HANDLE win_handle;                // Windows process HANDLE
    DWORD win_pid;                    // Windows process ID
    HANDLE win_thread;                // Main thread HANDLE
    DWORD win_thread_id;              // Main thread ID
    HANDLE stdout_pipe_read;          // Stdout pipe for capture
    HANDLE stderr_pipe_read;          // Stderr pipe for capture
    HANDLE stdin_pipe_write;          // Stdin pipe for input
    int is_background;                // Background job flag
    int job_id;                       // Job control ID
    
    // Resource tracking
    uint64_t peak_memory;             // Peak memory usage
    uint64_t read_bytes;              // I/O read bytes
    uint64_t write_bytes;             // I/O write bytes
    FILETIME kernel_time;             // Kernel mode time
    FILETIME user_time;               // User mode time
} RealProcess;

// Process table
typedef struct {
    RealProcess* processes[MAX_REAL_PROCESSES];
    int process_count;
    int next_pid;
    CRITICAL_SECTION lock;            // Thread safety
} RealProcessTable;

// Background job structure
typedef struct {
    int job_id;
    int pid;
    char command[MAX_PROCESS_ARGS];
    int is_stopped;
} BackgroundJob;

// Initialize real process management
int process_real_init(void);
void process_real_cleanup(void);

// Process creation/destruction
int process_real_spawn(const char* command, char** argv, int argc, int background, int* out_pid);
int process_real_exec(const char* path, char** argv, int argc);
int process_real_kill(int pid, int signal);
int process_real_wait(int pid, int* exit_code);

// Process queries
RealProcess* process_real_get(int pid);
RealProcess* process_real_get_by_name(const char* name);
int process_real_list(RealProcess*** out_list);
int process_real_count(void);

// Process control
int process_real_suspend(int pid);
int process_real_resume(int pid);
int process_real_set_priority(int pid, RealProcessPriority priority);

// Process monitoring
int process_real_update_stats(int pid);
void process_real_update_all_stats(void);
float process_real_get_total_cpu_usage(void);
uint64_t process_real_get_total_memory_usage(void);

// Job control
int job_add(int pid, const char* command);
int job_remove(int job_id);
BackgroundJob* job_get_by_id(int job_id);
int job_list(BackgroundJob*** out_list);
int job_get_count(void);
int job_foreground(int job_id);
int job_background(int job_id);

// I/O capture
int process_capture_output(int pid, char* buffer, size_t buffer_size);
int process_send_input(int pid, const char* input, size_t input_len);

// Helper functions
int translate_windows_pid_to_unix(DWORD win_pid);
DWORD translate_unix_pid_to_windows(int unix_pid);
const char* process_state_to_string(RealProcessState state);

#endif // PROCESS_REAL_H
