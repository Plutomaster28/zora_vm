#include <stdio.h>
#include <windows.h>

// Simple test to verify the reboot mechanism works
int main() {
    char executable_path[MAX_PATH];
    GetModuleFileNameA(NULL, executable_path, MAX_PATH);
    
    printf("Current executable path: %s\n", executable_path);
    printf("Testing reboot mechanism...\n");
    
    // Create new process to restart
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if (CreateProcessA(
        executable_path,
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        printf("Successfully created new process (PID: %lu)\n", pi.dwProcessId);
        printf("Reboot mechanism works!\n");
        
        // Clean up handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        printf("Failed to create process: %lu\n", GetLastError());
    }
    
    printf("Press Enter to exit...\n");
    getchar();
    return 0;
}
