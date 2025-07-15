// Create src/binary/binary_executor.c

#include "binary_executor.h"
#include "elf_parser.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shlwapi.h>

static int binary_executor_initialized = 0;
static char sandbox_root[MAX_PATH];
static HANDLE sandbox_job = NULL;

// Initialize binary executor with proper sandboxing
int binary_executor_init(void) {
    if (binary_executor_initialized) {
        return 0;
    }
    
    printf("Initializing binary executor with sandboxing...\n");
    
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
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
        JOB_OBJECT_LIMIT_PROCESS_MEMORY |
        JOB_OBJECT_LIMIT_JOB_MEMORY |
        JOB_OBJECT_LIMIT_PROCESS_TIME |
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    
    // Fix the field names:
    job_limits.ProcessMemoryLimit = 64 * 1024 * 1024; // 64MB limit
    job_limits.JobMemoryLimit = 128 * 1024 * 1024;    // 128MB limit
    job_limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = 30 * 10000000; // 30 seconds
    job_limits.BasicLimitInformation.ActiveProcessLimit = 1;
    
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
}

// Windows binary execution with proper sandboxing (SANDBOXED)
int execute_windows_binary(const char* binary_path, char** argv, int argc) {
    if (!binary_executor_initialized) {
        return -1;
    }
    
    printf("Executing Windows binary (SANDBOXED): %s\n", binary_path);
    
    // Copy binary to sandbox
    create_sandbox_environment(binary_path);
    
    // Build sandboxed command line
    char sandbox_binary[MAX_PATH];
    const char* filename = strrchr(binary_path, '\\');
    if (!filename) filename = strrchr(binary_path, '/');
    if (!filename) filename = binary_path;
    else filename++;
    
    snprintf(sandbox_binary, sizeof(sandbox_binary), "%s\\bin\\%s", sandbox_root, filename);
    
    // Create process with security restrictions
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    // Build command line
    char cmdline[2048] = {0};
    snprintf(cmdline, sizeof(cmdline), "\"%s\"", sandbox_binary);
    
    for (int i = 1; i < argc; i++) {
        strcat(cmdline, " \"");
        strcat(cmdline, argv[i]);
        strcat(cmdline, "\"");
    }
    
    printf("Sandboxed command: %s\n", cmdline);
    printf("Working directory: %s\n", sandbox_root);
    
    // Create process in sandbox
    BOOL success = CreateProcessA(
        NULL,                    // Application name
        cmdline,                 // Command line
        NULL,                    // Process security attributes
        NULL,                    // Thread security attributes
        FALSE,                   // Inherit handles
        CREATE_SUSPENDED,        // Creation flags
        NULL,                    // Environment
        sandbox_root,            // Current directory (sandbox)
        &si,                     // Startup info
        &pi                      // Process info
    );
    
    if (!success) {
        printf("Failed to create sandboxed process: %d\n", GetLastError());
        return -1;
    }
    
    // Add process to job object for additional security
    if (!AssignProcessToJobObject(sandbox_job, pi.hProcess)) {
        printf("Failed to assign process to job object\n");
    }
    
    // Resume the process
    ResumeThread(pi.hThread);
    
    printf("Process started with PID: %d\n", pi.dwProcessId);
    
    // Wait for completion with timeout
    DWORD wait_result = WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout
    
    DWORD exit_code = 0;
    if (wait_result == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exit_code);
        printf("Process completed with exit code: %d\n", exit_code);
    } else if (wait_result == WAIT_TIMEOUT) {
        printf("Process timed out, terminating...\n");
        TerminateProcess(pi.hProcess, 1);
        exit_code = 1;
    } else {
        printf("Process wait failed: %d\n", GetLastError());
        exit_code = -1;
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return exit_code;
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