#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "kernel.h"
#include "terminal/terminal_detector.h"

// System monitoring and OS-like features

typedef struct {
    int pid;
    char name[64];
    int priority;
    int cpu_usage;
    int memory_usage;
    time_t start_time;
    char status[16];
} ProcessInfo;

static ProcessInfo system_processes[64];
static int process_count = 0;
static time_t system_start_time;
static uint64_t system_uptime = 0;

void system_monitor_init(void) {
    system_start_time = time(NULL);
    process_count = 0;
    
    // Add kernel processes
    ProcessInfo kernel_proc = {0};
    kernel_proc.pid = 1;
    strcpy(kernel_proc.name, "kernel");
    kernel_proc.priority = 0;
    kernel_proc.cpu_usage = 5;
    kernel_proc.memory_usage = 2048;
    kernel_proc.start_time = system_start_time;
    strcpy(kernel_proc.status, "running");
    system_processes[process_count++] = kernel_proc;
    
    ProcessInfo init_proc = {0};
    init_proc.pid = 2;
    strcpy(init_proc.name, "init");
    init_proc.priority = 10;
    init_proc.cpu_usage = 1;
    init_proc.memory_usage = 512;
    init_proc.start_time = system_start_time;
    strcpy(init_proc.status, "running");
    system_processes[process_count++] = init_proc;
    
    ProcessInfo shell_proc = {0};
    shell_proc.pid = 3;
    strcpy(shell_proc.name, "merl-shell");
    shell_proc.priority = 20;
    shell_proc.cpu_usage = 8;
    shell_proc.memory_usage = 4096;
    shell_proc.start_time = system_start_time;
    strcpy(shell_proc.status, "running");
    system_processes[process_count++] = shell_proc;
    
    ProcessInfo vfs_proc = {0};
    vfs_proc.pid = 4;
    strcpy(vfs_proc.name, "vfs-daemon");
    vfs_proc.priority = 15;
    vfs_proc.cpu_usage = 3;
    vfs_proc.memory_usage = 1024;
    vfs_proc.start_time = system_start_time;
    strcpy(vfs_proc.status, "running");
    system_processes[process_count++] = vfs_proc;
    
    ProcessInfo network_proc = {0};
    network_proc.pid = 5;
    strcpy(network_proc.name, "net-stack");
    network_proc.priority = 25;
    network_proc.cpu_usage = 2;
    network_proc.memory_usage = 768;
    network_proc.start_time = system_start_time;
    strcpy(network_proc.status, "running");
    system_processes[process_count++] = network_proc;
}

void system_monitor_update(void) {
    system_uptime = time(NULL) - system_start_time;
    
    // Simulate dynamic process stats
    for (int i = 0; i < process_count; i++) {
        // Vary CPU usage slightly
        system_processes[i].cpu_usage += (rand() % 3) - 1;
        if (system_processes[i].cpu_usage < 0) system_processes[i].cpu_usage = 0;
        if (system_processes[i].cpu_usage > 15) system_processes[i].cpu_usage = 15;
    }
}

void system_monitor_display_processes(void) {
    const char* tl = get_box_char(BOX_TOP_LEFT);
    const char* tr = get_box_char(BOX_TOP_RIGHT);
    const char* bl = get_box_char(BOX_BOTTOM_LEFT);
    const char* br = get_box_char(BOX_BOTTOM_RIGHT);
    const char* h = get_box_char(BOX_HORIZONTAL);
    const char* v = get_box_char(BOX_VERTICAL);
    const char* td = get_box_char(BOX_T_DOWN);
    const char* tu = get_box_char(BOX_T_UP);
    const char* tr_cross = get_box_char(BOX_T_RIGHT);
    const char* tl_cross = get_box_char(BOX_T_LEFT);
    const char* cross = get_box_char(BOX_CROSS);
    
    // Top border
    printf("%s", tl);
    for (int i = 0; i < 78; i++) printf("%s", h);
    printf("%s\n", tr);
    
    // Title
    printf("%s                                ZoraVM Process Monitor                        %s\n", v, v);
    
    // Header separator
    printf("%s", tr_cross);
    for (int i = 0; i < 6; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 14; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 8; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 7; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 8; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 13; i++) printf("%s", h);
    printf("%s", td);
    for (int i = 0; i < 14; i++) printf("%s", h);
    printf("%s\n", tl_cross);
    
    // Column headers
    printf("%s PID  %s NAME         %s STATUS %s PRI   %s CPU%%   %s MEMORY(KB)  %s UPTIME       %s\n", v, v, v, v, v, v, v, v);
    
    // Header bottom
    printf("%s", tr_cross);
    for (int i = 0; i < 6; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 14; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 8; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 7; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 8; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 13; i++) printf("%s", h);
    printf("%s", cross);
    for (int i = 0; i < 14; i++) printf("%s", h);
    printf("%s\n", tl_cross);
    
    // Process data
    time_t current_time = time(NULL);
    for (int i = 0; i < process_count; i++) {
        ProcessInfo* proc = &system_processes[i];
        int uptime_mins = (current_time - proc->start_time) / 60;
        int uptime_hours = uptime_mins / 60;
        uptime_mins %= 60;
        
        printf("%s %-4d %s %-12s %s %-6s %s %-5d %s %3d%%   %s %8d    %s %02d:%02d        %s\n",
               v, proc->pid, v, proc->name, v, proc->status, v, proc->priority,
               v, proc->cpu_usage, v, proc->memory_usage, v, uptime_hours, uptime_mins, v);
    }
    
    // Bottom border
    printf("%s", bl);
    for (int i = 0; i < 78; i++) printf("%s", h);
    printf("%s\n", br);
    
    // System summary
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                System Status                                 ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ System Uptime: %02lld:%02lld:%02lld                                               ║\n",
           system_uptime / 3600, (system_uptime % 3600) / 60, system_uptime % 60);
    printf("║ Total Processes: %-3d                                                       ║\n", process_count);
    printf("║ Virtual Memory: %llu MB total, %llu MB available                           ║\n",
           memStatus.ullTotalVirtual / (1024 * 1024), memStatus.ullAvailVirtual / (1024 * 1024));
    printf("║ Physical Memory: %llu MB total, %llu MB available                          ║\n",
           memStatus.ullTotalPhys / (1024 * 1024), memStatus.ullAvailPhys / (1024 * 1024));
    printf("║ Memory Load: %ld%%                                                           ║\n", memStatus.dwMemoryLoad);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
}

void system_monitor_display_system_info(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              ZoraVM System Information                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ OS Name: ZoraVM Virtual Operating System v2.1.0                             ║\n");
    printf("║ Kernel: ZORA Kernel v2.1.0 (Built %s %s)                           ║\n", __DATE__, __TIME__);
    printf("║ Architecture: %s                                                            ║\n",
           si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x86_64" : "x86");
    printf("║ CPU Cores: %ld                                                               ║\n", si.dwNumberOfProcessors);
    printf("║ Page Size: %ld bytes                                                        ║\n", si.dwPageSize);
    
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    
    printf("║ Total Physical RAM: %llu MB                                                 ║\n", memStatus.ullTotalPhys / (1024 * 1024));
    printf("║ Available RAM: %llu MB                                                      ║\n", memStatus.ullAvailPhys / (1024 * 1024));
    printf("║ Virtual Memory: %llu MB                                                     ║\n", memStatus.ullTotalVirtual / (1024 * 1024));
    
    char hostname[256];
    DWORD hostname_size = sizeof(hostname);
    GetComputerNameA(hostname, &hostname_size);
    printf("║ Hostname: %s                                                                ║\n", hostname);
    
    printf("║                                                                              ║\n");
    printf("║ Features:                                                                    ║\n");
    printf("║ • Unix-style file permissions and ownership                                 ║\n");
    printf("║ • Multi-user authentication system                                          ║\n");
    printf("║ • Virtual file system with persistence                                      ║\n");
    printf("║ • Sandboxed process execution                                                ║\n");
    printf("║ • Virtual networking stack                                                   ║\n");
    printf("║ • Lua, Python, and Perl scripting engines                                   ║\n");
    printf("║ • Package management (Tetra)                                                ║\n");
    printf("║ • Terminal customization and themes                                         ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
}

int system_monitor_add_process(const char* name, int priority) {
    if (process_count >= 64) return -1;
    
    ProcessInfo new_proc = {0};
    new_proc.pid = process_count + 1;
    strncpy(new_proc.name, name, sizeof(new_proc.name) - 1);
    new_proc.priority = priority;
    new_proc.cpu_usage = rand() % 10;
    new_proc.memory_usage = 512 + (rand() % 2048);
    new_proc.start_time = time(NULL);
    strcpy(new_proc.status, "running");
    
    system_processes[process_count++] = new_proc;
    return new_proc.pid;
}

int system_monitor_kill_process(int pid) {
    for (int i = 0; i < process_count; i++) {
        if (system_processes[i].pid == pid) {
            if (pid <= 5) {
                printf("Error: Cannot kill system process (PID %d)\n", pid);
                return -1;
            }
            
            // Move all processes after this one back
            for (int j = i; j < process_count - 1; j++) {
                system_processes[j] = system_processes[j + 1];
            }
            process_count--;
            return 0;
        }
    }
    return -1;
}

void system_monitor_display_filesystems(void) {
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              Mounted Filesystems                            ║\n");
    printf("╠═══════════════╤══════════════╤════════════╤════════════╤═══════════╤════════╣\n");
    printf("║ Filesystem    │ Mount Point  │ Type       │ Size       │ Used      │ Avail  ║\n");
    printf("╠═══════════════╪══════════════╪════════════╪════════════╪═══════════╪════════╣\n");
    printf("║ /dev/zora0    │ /            │ zorafs     │ 1.0G       │ 256M      │ 768M   ║\n");
    printf("║ /dev/zora1    │ /home        │ zorafs     │ 512M       │ 128M      │ 384M   ║\n");
    printf("║ /dev/zora2    │ /tmp         │ tmpfs      │ 256M       │ 32M       │ 224M   ║\n");
    printf("║ /dev/persist  │ /persistent  │ hostfs     │ 2.0G       │ 512M      │ 1.5G   ║\n");
    printf("║ /dev/scripts  │ /scripts     │ hostfs     │ 100M       │ 45M       │ 55M    ║\n");
    printf("╚═══════════════╧══════════════╧════════════╧════════════╧═══════════╧════════╝\n");
    
    printf("\nFilesystem Details:\n");
    printf("• zorafs: ZoraVM native virtual filesystem\n");
    printf("• tmpfs: Temporary filesystem (RAM-based)\n");
    printf("• hostfs: Host system filesystem bridge\n");
}

void system_monitor_display_network_status(void) {
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              Network Interfaces                             ║\n");
    printf("╠════════════╤══════════════╤═══════════╤════════════╤═════════════╤══════════╣\n");
    printf("║ Interface  │ IP Address   │ Status    │ RX Bytes   │ TX Bytes    │ MTU      ║\n");
    printf("╠════════════╪══════════════╪═══════════╪════════════╪═════════════╪══════════╣\n");
    printf("║ lo         │ 127.0.0.1    │ UP        │ 2.1K       │ 2.1K        │ 65536    ║\n");
    printf("║ veth0      │ 10.0.2.15    │ UP        │ 15.2M      │ 8.4M        │ 1500     ║\n");
    printf("║ vnet0      │ 192.168.1.10 │ UP        │ 45.8K      │ 23.1K       │ 1500     ║\n");
    printf("╚════════════╧══════════════╧═══════════╧════════════╧═════════════╧══════════╝\n");
    
    printf("\nNetwork Configuration:\n");
    printf("• Gateway: 10.0.2.1\n");
    printf("• DNS: 8.8.8.8, 8.8.4.4\n");
    printf("• Hostname: zora-vm\n");
    printf("• Domain: local\n");
}
