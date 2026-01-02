// src/system/process_real.c - REAL process management using Windows API
#include "system/process_real.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

static RealProcessTable g_real_procs = {0};
static BackgroundJob* g_jobs[MAX_REAL_PROCESSES] = {0};
static int g_job_count = 0;
static int g_next_job_id = 1;

// Initialize real process management
int process_real_init(void) {
    memset(&g_real_procs, 0, sizeof(RealProcessTable));
    g_real_procs.next_pid = 1;
    g_real_procs.process_count = 0;
    InitializeCriticalSection(&g_real_procs.lock);
    
    // Create init process (PID 1) - this is ZoraVM itself
    RealProcess* init = (RealProcess*)calloc(1, sizeof(RealProcess));
    if (!init) return -1;
    
    init->pid = 1;
    init->ppid = 0;
    strcpy(init->name, "zora_vm");
    strcpy(init->args, "zora_vm");
    init->state = PROC_RUNNING;
    init->priority = PRIORITY_HIGH;
    init->start_time = time(NULL);
    init->win_handle = GetCurrentProcess();
    init->win_pid = GetCurrentProcessId();
    init->win_thread = GetCurrentThread();
    init->win_thread_id = GetCurrentThreadId();
    init->is_background = 0;
    
    EnterCriticalSection(&g_real_procs.lock);
    g_real_procs.processes[0] = init;
    g_real_procs.process_count = 1;
    g_real_procs.next_pid = 2;
    LeaveCriticalSection(&g_real_procs.lock);
    
    printf("[PROC_REAL] Real process management initialized\n");
    printf("[PROC_REAL] Init process: PID=%d, Windows PID=%lu\n", init->pid, init->win_pid);
    
    return 0;
}

void process_real_cleanup(void) {
    EnterCriticalSection(&g_real_procs.lock);
    
    // Kill all child processes
    for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
        if (g_real_procs.processes[i] && g_real_procs.processes[i]->pid != 1) {
            if (g_real_procs.processes[i]->win_handle) {
                TerminateProcess(g_real_procs.processes[i]->win_handle, 1);
                CloseHandle(g_real_procs.processes[i]->win_handle);
            }
            if (g_real_procs.processes[i]->win_thread) {
                CloseHandle(g_real_procs.processes[i]->win_thread);
            }
            // Close pipes
            if (g_real_procs.processes[i]->stdout_pipe_read) CloseHandle(g_real_procs.processes[i]->stdout_pipe_read);
            if (g_real_procs.processes[i]->stderr_pipe_read) CloseHandle(g_real_procs.processes[i]->stderr_pipe_read);
            if (g_real_procs.processes[i]->stdin_pipe_write) CloseHandle(g_real_procs.processes[i]->stdin_pipe_write);
            
            free(g_real_procs.processes[i]);
            g_real_procs.processes[i] = NULL;
        }
    }
    
    g_real_procs.process_count = 0;
    LeaveCriticalSection(&g_real_procs.lock);
    DeleteCriticalSection(&g_real_procs.lock);
    
    printf("[PROC_REAL] Real process management cleaned up\n");
}

// Spawn a real Windows process
int process_real_spawn(const char* command, char** argv, int argc, int background, int* out_pid) {
    if (!command) return -1;
    
    EnterCriticalSection(&g_real_procs.lock);
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
        if (!g_real_procs.processes[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LeaveCriticalSection(&g_real_procs.lock);
        printf("[PROC_REAL] Process table full\n");
        return -1;
    }
    
    // Allocate process structure
    RealProcess* proc = (RealProcess*)calloc(1, sizeof(RealProcess));
    if (!proc) {
        LeaveCriticalSection(&g_real_procs.lock);
        return -1;
    }
    
    // Assign Unix-style PID
    proc->pid = g_real_procs.next_pid++;
    proc->ppid = 1; // Parent is init
    proc->state = PROC_RUNNING;
    proc->priority = PRIORITY_NORMAL;
    proc->start_time = time(NULL);
    proc->is_background = background;
    
    // Build command line
    char cmd_line[4096] = {0};
    int offset = 0;
    
    // Add command
    offset += snprintf(cmd_line + offset, sizeof(cmd_line) - offset, "\"%s\"", command);
    
    // Add arguments
    for (int i = 0; i < argc && i < 32; i++) {
        if (argv[i]) {
            offset += snprintf(cmd_line + offset, sizeof(cmd_line) - offset, " \"%s\"", argv[i]);
        }
    }
    
    strncpy(proc->name, command, MAX_PROCESS_NAME_REAL - 1);
    strncpy(proc->args, cmd_line, MAX_PROCESS_ARGS - 1);
    
    // Create pipes for I/O capture
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    
    HANDLE stdout_read = NULL, stdout_write = NULL;
    HANDLE stderr_read = NULL, stderr_write = NULL;
    HANDLE stdin_read = NULL, stdin_write = NULL;
    
    if (CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
        proc->stdout_pipe_read = stdout_read;
    }
    
    if (CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
        proc->stderr_pipe_read = stderr_read;
    }
    
    if (CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
        SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
        proc->stdin_pipe_write = stdin_write;
    }
    
    // Setup process creation
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    si.hStdInput = stdin_read;
    
    // Create the process
    BOOL success = CreateProcessA(
        NULL,                   // Application name
        cmd_line,               // Command line
        NULL,                   // Process security
        NULL,                   // Thread security
        TRUE,                   // Inherit handles
        CREATE_NO_WINDOW,       // Creation flags
        NULL,                   // Environment
        NULL,                   // Current directory
        &si,                    // Startup info
        &pi                     // Process info
    );
    
    // Close write ends of pipes (child has them)
    if (stdout_write) CloseHandle(stdout_write);
    if (stderr_write) CloseHandle(stderr_write);
    if (stdin_read) CloseHandle(stdin_read);
    
    if (!success) {
        DWORD error = GetLastError();
        printf("[PROC_REAL] Failed to create process: %s (Error: %lu)\n", command, error);
        
        // Cleanup
        if (proc->stdout_pipe_read) CloseHandle(proc->stdout_pipe_read);
        if (proc->stderr_pipe_read) CloseHandle(proc->stderr_pipe_read);
        if (proc->stdin_pipe_write) CloseHandle(proc->stdin_pipe_write);
        free(proc);
        LeaveCriticalSection(&g_real_procs.lock);
        return -1;
    }
    
    // Store Windows handles
    proc->win_handle = pi.hProcess;
    proc->win_pid = pi.dwProcessId;
    proc->win_thread = pi.hThread;
    proc->win_thread_id = pi.dwThreadId;
    
    // Add to process table
    g_real_procs.processes[slot] = proc;
    g_real_procs.process_count++;
    
    if (out_pid) *out_pid = proc->pid;
    
    printf("[PROC_REAL] Spawned process: PID=%d, Windows PID=%lu, Command=%s%s\n",
           proc->pid, proc->win_pid, command, background ? " &" : "");
    
    // If background job, add to job list
    if (background) {
        job_add(proc->pid, cmd_line);
    }
    
    LeaveCriticalSection(&g_real_procs.lock);
    
    // If foreground, wait for completion
    if (!background) {
        int exit_code = 0;
        process_real_wait(proc->pid, &exit_code);
        return exit_code;
    }
    
    return 0;
}

// Wait for process to complete
int process_real_wait(int pid, int* exit_code) {
    RealProcess* proc = process_real_get(pid);
    if (!proc || !proc->win_handle) return -1;
    
    // Wait for process to finish
    DWORD wait_result = WaitForSingleObject(proc->win_handle, INFINITE);
    
    if (wait_result == WAIT_OBJECT_0) {
        // Get exit code
        DWORD code = 0;
        if (GetExitCodeProcess(proc->win_handle, &code)) {
            if (exit_code) *exit_code = (int)code;
            proc->exit_code = (int)code;
        }
        
        // Mark as zombie
        proc->state = PROC_ZOMBIE;
        
        printf("[PROC_REAL] Process PID=%d exited with code %d\n", pid, proc->exit_code);
        return 0;
    }
    
    return -1;
}

// Kill a real process
int process_real_kill(int pid, int signal) {
    RealProcess* proc = process_real_get(pid);
    if (!proc) return -1;
    
    if (pid == 1) {
        printf("[PROC_REAL] Cannot kill init process\n");
        return -1;
    }
    
    if (!proc->win_handle) {
        printf("[PROC_REAL] Process PID=%d has no Windows handle\n", pid);
        return -1;
    }
    
    if (signal == PROC_SIG_KILL || signal == PROC_SIG_TERM) {
        // Terminate the Windows process
        if (TerminateProcess(proc->win_handle, signal)) {
            proc->state = PROC_DEAD;
            proc->exit_code = signal;
            printf("[PROC_REAL] Killed process PID=%d (Windows PID=%lu)\n", pid, proc->win_pid);
            
            // Cleanup handles
            if (proc->win_handle) CloseHandle(proc->win_handle);
            if (proc->win_thread) CloseHandle(proc->win_thread);
            proc->win_handle = NULL;
            proc->win_thread = NULL;
            
            // Remove from job list
            for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
                if (g_jobs[i] && g_jobs[i]->pid == pid) {
                    free(g_jobs[i]);
                    g_jobs[i] = NULL;
                    g_job_count--;
                    break;
                }
            }
            
            return 0;
        } else {
            DWORD error = GetLastError();
            printf("[PROC_REAL] Failed to kill PID=%d: Error %lu\n", pid, error);
            return -1;
        }
    } else if (signal == PROC_SIG_STOP) {
        // Suspend the process
        return process_real_suspend(pid);
    } else if (signal == PROC_SIG_CONT) {
        // Resume the process
        return process_real_resume(pid);
    }
    
    return -1;
}

// Suspend process
int process_real_suspend(int pid) {
    RealProcess* proc = process_real_get(pid);
    if (!proc || !proc->win_thread) return -1;
    
    if (SuspendThread(proc->win_thread) != (DWORD)-1) {
        proc->state = PROC_STOPPED;
        printf("[PROC_REAL] Suspended process PID=%d\n", pid);
        return 0;
    }
    
    return -1;
}

// Resume process
int process_real_resume(int pid) {
    RealProcess* proc = process_real_get(pid);
    if (!proc || !proc->win_thread) return -1;
    
    if (ResumeThread(proc->win_thread) != (DWORD)-1) {
        proc->state = PROC_RUNNING;
        printf("[PROC_REAL] Resumed process PID=%d\n", pid);
        return 0;
    }
    
    return -1;
}

// Get process by PID
RealProcess* process_real_get(int pid) {
    for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
        if (g_real_procs.processes[i] && g_real_procs.processes[i]->pid == pid) {
            return g_real_procs.processes[i];
        }
    }
    return NULL;
}

// Update process statistics from Windows
int process_real_update_stats(int pid) {
    RealProcess* proc = process_real_get(pid);
    if (!proc || !proc->win_handle) return -1;
    
    // Get memory info
    PROCESS_MEMORY_COUNTERS_EX pmc = {0};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(proc->win_handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        proc->memory_used = pmc.WorkingSetSize;
        proc->peak_memory = pmc.PeakWorkingSetSize;
    }
    
    // Get CPU times
    FILETIME create_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(proc->win_handle, &create_time, &exit_time, &kernel_time, &user_time)) {
        proc->kernel_time = kernel_time;
        proc->user_time = user_time;
        
        // Calculate CPU percentage (simplified)
        ULARGE_INTEGER kt, ut;
        kt.LowPart = kernel_time.dwLowDateTime;
        kt.HighPart = kernel_time.dwHighDateTime;
        ut.LowPart = user_time.dwLowDateTime;
        ut.HighPart = user_time.dwHighDateTime;
        
        uint64_t total_time = kt.QuadPart + ut.QuadPart;
        uint64_t elapsed = (uint64_t)(time(NULL) - proc->start_time) * 10000000; // 100ns units
        
        if (elapsed > 0) {
            proc->cpu_percent = (float)(total_time * 100.0 / elapsed);
        }
    }
    
    // Get I/O stats
    IO_COUNTERS io = {0};
    if (GetProcessIoCounters(proc->win_handle, &io)) {
        proc->read_bytes = io.ReadTransferCount;
        proc->write_bytes = io.WriteTransferCount;
    }
    
    // Check if process is still running
    DWORD exit_code = 0;
    if (GetExitCodeProcess(proc->win_handle, &exit_code)) {
        if (exit_code != STILL_ACTIVE) {
            proc->state = PROC_ZOMBIE;
            proc->exit_code = (int)exit_code;
        }
    }
    
    return 0;
}

// Set process priority
int process_real_set_priority(int pid, RealProcessPriority priority) {
    RealProcess* proc = process_real_get(pid);
    if (!proc || !proc->win_handle) return -1;
    
    DWORD win_priority;
    switch (priority) {
        case PRIORITY_REALTIME:
            win_priority = REALTIME_PRIORITY_CLASS;
            break;
        case PRIORITY_HIGH:
            win_priority = HIGH_PRIORITY_CLASS;
            break;
        case PRIORITY_NORMAL:
            win_priority = NORMAL_PRIORITY_CLASS;
            break;
        case PRIORITY_LOW:
            win_priority = IDLE_PRIORITY_CLASS;
            break;
        default:
            win_priority = NORMAL_PRIORITY_CLASS;
    }
    
    if (SetPriorityClass(proc->win_handle, win_priority)) {
        proc->priority = priority;
        return 0;
    }
    
    return -1;
}

// List all processes
int process_real_list(RealProcess*** out_list) {
    if (!out_list) return -1;
    
    *out_list = (RealProcess**)malloc(g_real_procs.process_count * sizeof(RealProcess*));
    if (!*out_list) return -1;
    
    int count = 0;
    for (int i = 0; i < MAX_REAL_PROCESSES && count < g_real_procs.process_count; i++) {
        if (g_real_procs.processes[i]) {
            (*out_list)[count++] = g_real_procs.processes[i];
        }
    }
    
    return count;
}

// Job control functions
int job_add(int pid, const char* command) {
    for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
        if (!g_jobs[i]) {
            BackgroundJob* job = (BackgroundJob*)malloc(sizeof(BackgroundJob));
            if (!job) return -1;
            
            job->job_id = g_next_job_id++;
            job->pid = pid;
            strncpy(job->command, command, MAX_PROCESS_ARGS - 1);
            job->is_stopped = 0;
            
            g_jobs[i] = job;
            g_job_count++;
            
            printf("[%d] %d\n", job->job_id, pid);
            return job->job_id;
        }
    }
    return -1;
}

BackgroundJob* job_get_by_id(int job_id) {
    for (int i = 0; i < MAX_REAL_PROCESSES; i++) {
        if (g_jobs[i] && g_jobs[i]->job_id == job_id) {
            return g_jobs[i];
        }
    }
    return NULL;
}

int job_list(BackgroundJob*** out_list) {
    if (!out_list) return -1;
    
    *out_list = (BackgroundJob**)malloc(g_job_count * sizeof(BackgroundJob*));
    if (!*out_list) return -1;
    
    int count = 0;
    for (int i = 0; i < MAX_REAL_PROCESSES && count < g_job_count; i++) {
        if (g_jobs[i]) {
            (*out_list)[count++] = g_jobs[i];
        }
    }
    
    return count;
}

int job_foreground(int job_id) {
    BackgroundJob* job = job_get_by_id(job_id);
    if (!job) return -1;
    
    RealProcess* proc = process_real_get(job->pid);
    if (!proc) return -1;
    
    printf("[%d]  + running                %s\n", job->job_id, job->command);
    
    // Wait for job to complete
    int exit_code = 0;
    return process_real_wait(job->pid, &exit_code);
}

int job_background(int job_id) {
    BackgroundJob* job = job_get_by_id(job_id);
    if (!job) return -1;
    
    RealProcess* proc = process_real_get(job->pid);
    if (!proc) return -1;
    
    // Resume if stopped
    if (job->is_stopped) {
        process_real_resume(job->pid);
        job->is_stopped = 0;
    }
    
    printf("[%d]  + continued             %s\n", job->job_id, job->command);
    return 0;
}

int job_get_count(void) {
    return g_job_count;
}

const char* process_state_to_string(RealProcessState state) {
    switch (state) {
        case PROC_RUNNING: return "R";
        case PROC_SLEEPING: return "S";
        case PROC_STOPPED: return "T";
        case PROC_ZOMBIE: return "Z";
        case PROC_DEAD: return "X";
        default: return "?";
    }
}

// ===== JOB CONTROL IMPLEMENTATION =====
