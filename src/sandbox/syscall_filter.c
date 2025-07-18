// Create src/sandbox/syscall_filter.c:

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// Windows-compatible file access filtering
static int is_path_allowed(const char* path) {
    // Only allow access to paths within the VM's directory structure
    if (strstr(path, "..\\ZoraPerl\\") == path ||
        strstr(path, "../ZoraPerl/") == path ||
        strstr(path, "\\persistent\\") ||
        strstr(path, "/persistent/") ||
        strstr(path, "zora_vm") != NULL) {
        return 1;
    }
    return 0;
}

// Log blocked file access attempts
void log_blocked_access(const char* operation, const char* path) {
    printf("SANDBOX: Blocked %s access to: %s\n", operation, path);
}

// Safer file operations (these replace dangerous calls)
FILE* safe_fopen(const char* filename, const char* mode) {
    if (!is_path_allowed(filename)) {
        log_blocked_access("fopen", filename);
        return NULL;
    }
    return fopen(filename, mode);
}

int safe_system(const char* command) {
    // Block all system commands in sandbox mode
    log_blocked_access("system", command);
    return -1;
}

// Network blocking (Windows-specific)
int safe_socket(int domain, int type, int protocol) {
    log_blocked_access("socket", "network");
    return -1;
}

// Process creation blocking
HANDLE safe_CreateProcess(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
) {
    log_blocked_access("CreateProcess", lpApplicationName ? lpApplicationName : lpCommandLine);
    return NULL;
}