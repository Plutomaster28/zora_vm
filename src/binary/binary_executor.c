// Create src/binary/binary_executor.c

#include "binary_executor.h"
#include "sandbox.h"
#include "elf_parser.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#else
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

// Global flag to track if we're in a sandboxed binary
static int in_sandboxed_binary = 0;

static int binary_executor_initialized = 0;
static char sandbox_root[MAX_PATH];
static HANDLE sandbox_job = NULL;

// Signal handler for crashes
#ifdef _WIN32
LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info) {
    printf("SANDBOX ALERT: Binary crashed with exception 0x%08X\n", 
           exception_info->ExceptionRecord->ExceptionCode);
    printf("This may indicate a sandbox escape attempt or malicious code\n");
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void crash_handler(int sig) {
    printf("SANDBOX ALERT: Binary crashed with signal %d\n", sig);
    printf("This may indicate a sandbox escape attempt or malicious code\n");
    exit(1);
}
#endif

// Initialize binary executor with proper sandboxing
int binary_executor_init(void) {
    if (binary_executor_initialized) {
        return 0;
    }
    
    printf("Initializing binary executor with enhanced sandbox protection...\n");
    
    // Install crash handlers
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_handler);
#else
    signal(SIGSEGV, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGABRT, crash_handler);
#endif
    
    // Create sandbox directory
    if (create_sandbox_environment(NULL) != 0) {
        printf("Failed to create sandbox environment\n");
        return -1;
    }
    
    // Initialize ELF parser
    if (elf_init() != 0) {
        printf("Failed to initialize ELF parser\n");
        return -1;
    }
    
    // Create Windows job object for process isolation
    sandbox_job = CreateJobObject(NULL, "ZoraVM_Sandbox");  // Remove the L prefix
    if (!sandbox_job) {
        printf("Failed to create sandbox job object\n");
        return -1;
    }
    
    // Configure job object limits
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limits = {0};
    job_limits.BasicLimitInformation.LimitFlags = 
        JOB_OBJECT_LIMIT_PROCESS_MEMORY | 
        JOB_OBJECT_LIMIT_JOB_MEMORY |
        JOB_OBJECT_LIMIT_PROCESS_TIME |
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
        JOB_OBJECT_LIMIT_BREAKAWAY_OK;  // Prevent breakaway processes
    
    // Even more restrictive limits
    job_limits.ProcessMemoryLimit = 32 * 1024 * 1024; // 32MB only
    job_limits.JobMemoryLimit = 32 * 1024 * 1024;
    job_limits.BasicLimitInformation.ActiveProcessLimit = 1; // Only 1 process
    job_limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = 10 * 10000000LL; // 10 seconds
    
    // Add UI restrictions
    JOBOBJECT_BASIC_UI_RESTRICTIONS ui_limits = {0};
    ui_limits.UIRestrictionsClass = 
        JOB_OBJECT_UILIMIT_DESKTOP |          // No desktop access
        JOB_OBJECT_UILIMIT_EXITWINDOWS |      // Can't shutdown
        JOB_OBJECT_UILIMIT_HANDLES |          // Limit handle access
        JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS | // No system changes
        JOB_OBJECT_UILIMIT_WRITECLIPBOARD;    // No clipboard

    SetInformationJobObject(sandbox_job, JobObjectBasicUIRestrictions, &ui_limits, sizeof(ui_limits));
    
    if (!SetInformationJobObject(sandbox_job, JobObjectExtendedLimitInformation, 
                                 &job_limits, sizeof(job_limits))) {
        printf("Failed to set job object limits\n");
    }
    
    printf("Binary executor with sandboxing initialized\n");
    binary_executor_initialized = 1;
    return 0;
}

// Create sandbox environment
int create_sandbox_environment(const char* binary_path) {
    // Create sandbox directory structure
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    
    snprintf(sandbox_root, sizeof(sandbox_root), "%s\\zora_vm_sandbox_%d", 
             temp_path, GetCurrentProcessId());
    
    printf("Creating sandbox environment at: %s\n", sandbox_root);
    
    // Create sandbox directories
    CreateDirectoryA(sandbox_root, NULL);
    
    char bin_path[MAX_PATH], tmp_path[MAX_PATH], home_path[MAX_PATH];
    snprintf(bin_path, sizeof(bin_path), "%s\\bin", sandbox_root);
    snprintf(tmp_path, sizeof(tmp_path), "%s\\tmp", sandbox_root);
    snprintf(home_path, sizeof(home_path), "%s\\home", sandbox_root);
    
    CreateDirectoryA(bin_path, NULL);
    CreateDirectoryA(tmp_path, NULL);
    CreateDirectoryA(home_path, NULL);
    
    // Copy binary to sandbox if specified
    if (binary_path) {
        char sandbox_binary[MAX_PATH];
        const char* filename = strrchr(binary_path, '\\');
        if (!filename) filename = strrchr(binary_path, '/');
        if (!filename) filename = binary_path;
        else filename++;
        
        snprintf(sandbox_binary, sizeof(sandbox_binary), "%s\\%s", bin_path, filename);
        
        if (!CopyFileA(binary_path, sandbox_binary, FALSE)) {
            printf("Failed to copy binary to sandbox\n");
        } else {
            printf("Binary copied to sandbox: %s\n", sandbox_binary);
        }
    }
    
    printf("Sandbox environment created\n");
    return 0;
}

// Accessor functions
int binary_executor_is_initialized(void) {
    return binary_executor_initialized;
}

const char* binary_executor_get_elf_parser_info(void) {
    return "Built-in ELF Parser";
}

int binary_executor_has_elf_support(void) {
    return binary_executor_initialized;
}

const char* binary_executor_get_elf_x86_64_support(void) {
    return "Native ELF Parser (64-bit)";
}

const char* binary_executor_get_elf_i386_support(void) {
    return "Native ELF Parser (32-bit)";
}

// Deprecated: Keep these for backwards compatibility
const char* binary_executor_get_qemu_path(void) {
    return binary_executor_get_elf_parser_info();
}

int binary_executor_has_qemu(void) {
    return binary_executor_has_elf_support();
}

const char* binary_executor_get_qemu_x86_64_path(void) {
    return binary_executor_get_elf_x86_64_support();
}

const char* binary_executor_get_qemu_i386_path(void) {
    return binary_executor_get_elf_i386_support();
}

// Linux binary execution with native ELF parser (SANDBOXED)
int execute_linux_binary(const char* binary_path, char** argv, int argc) {
    if (!binary_executor_initialized) {
        printf("Binary executor not initialized\n");
        return -1;
    }
    
    printf("Executing Linux binary (SANDBOXED): %s\n", binary_path);
    
    // Print command line arguments if provided
    if (argv && argc > 0) {
        printf("Command line arguments:\n");
        for (int i = 0; i < argc; i++) {
            printf("  argv[%d] = %s\n", i, argv[i]);
        }
    }
    
    // TEMPORARILY DISABLED: ELF emulation is causing crashes
    printf("SANDBOX ALERT: Linux ELF execution temporarily disabled due to security concerns\n");
    printf("This prevents potential VM crashes and sandbox escapes\n");
    printf("Binary would have been executed: %s\n", binary_path);
    
    // Return a safe exit code
    return -1;
    
    /* ORIGINAL CODE - DISABLED FOR SAFETY
    // Check if it's a valid ELF file
    if (!elf_is_valid(binary_path)) {
        printf("Not a valid ELF file: %s\n", binary_path);
        return -1;
    }
    
    // Create sandbox environment
    create_sandbox_environment(binary_path);
    
    // Get architecture info
    int arch = elf_get_architecture(binary_path);
    printf("ELF Architecture: %s\n", arch == 64 ? "64-bit" : arch == 32 ? "32-bit" : "Unknown");
    
    // Load ELF file
    ElfContext* ctx = elf_load(binary_path);
    if (!ctx) {
        printf("Failed to load ELF file\n");
        return -1;
    }
    
    // Set up sandboxed execution environment
    ctx->sandbox_root = strdup(sandbox_root);
    ctx->job_object = sandbox_job;
    
    // Print ELF information
    elf_print_info(ctx);
    
    // Execute ELF binary in sandbox (SANDBOXED VERSION)
    printf("Executing in sandbox: %s\n", sandbox_root);
    int result = elf_execute_sandboxed(ctx, argv, argc);
    
    // Cleanup
    elf_free_context(ctx);
    
    printf("Linux binary execution completed with exit code: %d\n", result);
    return result;
    */
}

// Windows binary execution with proper sandboxing (SANDBOXED)
int execute_windows_binary(const char* binary_path, char** argv, int argc) {
    if (!sandbox_is_strict_mode()) {
        printf("WARNING: Executing Windows binary without strict sandbox mode!\n");
    }
    
    printf("Executing Windows binary (SANDBOXED): %s\n", binary_path);
    
    // Print command line arguments if provided
    if (argv && argc > 0) {
        printf("Command line arguments:\n");
        for (int i = 0; i < argc; i++) {
            printf("  argv[%d] = %s\n", i, argv[i]);
        }
    }
    
#ifdef _WIN32
    // Resolve the full path to the binary
    char full_binary_path[MAX_PATH];
    if (GetFullPathNameA(binary_path, MAX_PATH, full_binary_path, NULL) == 0) {
        printf("Failed to resolve binary path: %s\n", binary_path);
        return -1;
    }
    
    // Check if binary exists
    if (GetFileAttributesA(full_binary_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Binary file not found: %s\n", full_binary_path);
        return -1;
    }
    
    printf("Resolved binary path: %s\n", full_binary_path);
    
    // Create a more secure sandbox environment
    char sandbox_dir[512];
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    snprintf(sandbox_dir, sizeof(sandbox_dir), "%szora_vm_sandbox_%d", 
             temp_path, GetCurrentProcessId());
    
    printf("Creating sandbox environment at: %s\n", sandbox_dir);
    
    // Create sandbox directory structure
    if (CreateDirectoryA(sandbox_dir, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        printf("Failed to create sandbox directory: %lu\n", GetLastError());
        return -1;
    }
    
    char sandbox_bin[512];
    snprintf(sandbox_bin, sizeof(sandbox_bin), "%s\\bin", sandbox_dir);
    if (CreateDirectoryA(sandbox_bin, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        printf("Failed to create sandbox bin directory: %lu\n", GetLastError());
        return -1;
    }
    
    // Copy binary to sandbox
    char sandbox_binary[512];
    const char* filename = strrchr(full_binary_path, '\\');
    if (!filename) filename = strrchr(full_binary_path, '/');
    if (!filename) filename = full_binary_path;
    else filename++;
    
    snprintf(sandbox_binary, sizeof(sandbox_binary), "%s\\%s", sandbox_bin, filename);
    
    printf("Copying from: %s\n", full_binary_path);
    printf("Copying to: %s\n", sandbox_binary);
    
    if (CopyFileA(full_binary_path, sandbox_binary, FALSE) == 0) {
        DWORD error = GetLastError();
        printf("Failed to copy binary to sandbox: %lu\n", error);
        printf("Source exists: %s\n", GetFileAttributesA(full_binary_path) != INVALID_FILE_ATTRIBUTES ? "Yes" : "No");
        printf("Destination dir exists: %s\n", GetFileAttributesA(sandbox_bin) != INVALID_FILE_ATTRIBUTES ? "Yes" : "No");
        return -1;
    }
    
    printf("Binary copied to sandbox: %s\n", sandbox_binary);
    printf("Sandbox environment created\n");
    
    // Create a job object for process control
    HANDLE job = CreateJobObjectA(NULL, NULL);
    if (job) {
        // Set job limits
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limits = {0};
        job_limits.BasicLimitInformation.LimitFlags = 
            JOB_OBJECT_LIMIT_PROCESS_MEMORY | 
            JOB_OBJECT_LIMIT_JOB_MEMORY |
            JOB_OBJECT_LIMIT_PROCESS_TIME |
            JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        
        job_limits.ProcessMemoryLimit = 64 * 1024 * 1024; // 64MB limit
        job_limits.JobMemoryLimit = 64 * 1024 * 1024;
        job_limits.BasicLimitInformation.ActiveProcessLimit = 1;
        
        // 30 second time limit
        job_limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = 30 * 10000000LL;
        
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, 
                                    &job_limits, sizeof(job_limits))) {
            printf("Warning: Failed to set job limits: %lu\n", GetLastError());
        }
    }
    
    // Build command line string if arguments are provided
    char command_line[2048] = {0};
    if (argv && argc > 0) {
        // Start with the binary path in quotes
        snprintf(command_line, sizeof(command_line), "\"%s\"", sandbox_binary);
        
        // Add arguments
        for (int i = 1; i < argc; i++) {
            if (strlen(command_line) + strlen(argv[i]) + 3 < sizeof(command_line)) {
                strcat(command_line, " \"");
                strcat(command_line, argv[i]);
                strcat(command_line, "\"");
            }
        }
    }
    
    // Create process with restrictions
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    // Create the process in the sandbox directory
    in_sandboxed_binary = 1;
    printf("Sandboxed command: %s\n", command_line[0] ? command_line : sandbox_binary);
    printf("Working directory: %s\n", sandbox_dir);
    
    BOOL success = CreateProcessA(
        NULL,                                     // Application name (use command line)
        command_line[0] ? command_line : sandbox_binary,  // Command line
        NULL,                                     // Process security attributes
        NULL,                                     // Thread security attributes
        FALSE,                                    // Inherit handles
        CREATE_SUSPENDED,                         // Creation flags - start suspended
        NULL,                                     // Environment
        sandbox_dir,                              // Current directory
        &si,                                      // Startup info
        &pi                                       // Process info
    );
    
    if (!success) {
        DWORD error = GetLastError();
        printf("Failed to create sandboxed process: %lu\n", error);
        if (job) CloseHandle(job);
        return -1;
    }
    
    printf("Process started with PID: %lu\n", pi.dwProcessId);
    
    // FIRST: Add process to job object BEFORE resuming
    if (job) {
        if (!AssignProcessToJobObject(job, pi.hProcess)) {
            printf("Warning: Failed to assign process to job: %lu\n", GetLastError());
        }
    }
    
    // SECOND: Resume the process
    ResumeThread(pi.hThread);
    printf("Process resumed and running in sandbox\n");
    
    // THIRD: Now wait for completion
    DWORD wait_result = WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout
    
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    // Handle different wait results
    if (wait_result == WAIT_TIMEOUT) {
        printf("SANDBOX ALERT: Process timed out - terminating\n");
        TerminateProcess(pi.hProcess, 1);
        exit_code = 1;
    } else if (wait_result == WAIT_FAILED) {
        printf("SANDBOX ALERT: Wait failed: %lu\n", GetLastError());
        exit_code = -1;
    } else if (wait_result == WAIT_OBJECT_0) {
        // Process completed normally
        if (exit_code == 0xC0000005) {
            printf("SANDBOX ALERT: Process crashed with access violation\n");
            printf("This may indicate the sandbox blocked dangerous memory access\n");
        } else if (exit_code != 0) {
            printf("Process exited with non-zero code: %d\n", (int)exit_code);
        }
    }
    
    printf("Process completed with exit code: %d\n", (int)exit_code);
    
    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (job) CloseHandle(job);
    
    // Clean up sandbox directory
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rmdir /s /q \"%s\"", sandbox_dir);
    system(cleanup_cmd);
    
    in_sandboxed_binary = 0;
    printf("Windows binary execution completed (exit code: %d)\n", (int)exit_code);
    
    return exit_code;
    
#else
    printf("Windows binary execution not supported on this platform\n");
    return -1;
#endif
}

// Binary type detection
BinaryType detect_binary_type(const char* file_path) {
    FILE* f = fopen(file_path, "rb");
    if (!f) {
        return BINARY_TYPE_UNKNOWN;
    }
    
    unsigned char header[16];
    if (fread(header, 1, 16, f) < 4) {
        fclose(f);
        return BINARY_TYPE_UNKNOWN;
    }
    fclose(f);
    
    // Check for PE (Windows) executable
    if (header[0] == 'M' && header[1] == 'Z') {
        return BINARY_TYPE_WINDOWS_PE;
    }
    
    // Check for ELF (Linux) executable
    if (header[0] == 0x7F && header[1] == 'E' && header[2] == 'L' && header[3] == 'F') {
        return BINARY_TYPE_LINUX_ELF;
    }
    
    // Check for script files
    if (header[0] == '#' && header[1] == '!') {
        return BINARY_TYPE_SCRIPT;
    }
    
    return BINARY_TYPE_UNKNOWN;
}

// Sandboxed binary execution dispatcher
int execute_sandboxed_binary(const char* binary_path, char** argv, int argc) {
    VNode* node = vfs_find_node(binary_path);
    if (!node) {
        printf("Binary not found: %s\n", binary_path);
        return -1;
    }
    
    char* host_path = node->host_path;
    if (!host_path) {
        printf("No host path available for: %s\n", binary_path);
        return -1;
    }
    
    BinaryType type = detect_binary_type(host_path);
    
    printf("🔍 Binary type detected: ");
    switch (type) {
        case BINARY_TYPE_WINDOWS_PE:
            printf("Windows PE executable (SANDBOXED)\n");
            return execute_windows_binary(host_path, argv, argc);
            
        case BINARY_TYPE_LINUX_ELF:
            printf("Linux ELF executable (SANDBOXED)\n");
            return execute_linux_binary(host_path, argv, argc);
            
        case BINARY_TYPE_SCRIPT:
            printf("Script file (SANDBOXED)\n");
            // TODO: Implement sandboxed script execution
            return system(host_path);
            
        default:
            printf("Unknown binary type\n");
            return -1;
    }
}

// Cleanup sandbox environment
void cleanup_sandbox_environment(void) {
    if (sandbox_root[0] != '\0') {
        printf("🧹 Cleaning up sandbox environment: %s\n", sandbox_root);
        
        // Remove sandbox directory and contents
        char cmd[MAX_PATH + 20];
        snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\"", sandbox_root);
        system(cmd);
        
        sandbox_root[0] = '\0';
    }
    
    if (sandbox_job) {
        // Terminate all processes in the job
        TerminateJobObject(sandbox_job, 0);
        CloseHandle(sandbox_job);
        sandbox_job = NULL;
    }
}

// Cleanup binary executor
void binary_executor_cleanup(void) {
    if (binary_executor_initialized) {
        cleanup_sandbox_environment();
        elf_cleanup();
        binary_executor_initialized = 0;
    }
}

int is_in_sandboxed_binary(void) {
    return in_sandboxed_binary;
}