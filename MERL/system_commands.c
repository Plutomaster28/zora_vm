// Enhanced system command implementations for ZoraVM
// These commands provide mature system utility functionality

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editors/zora_editor.h"
#include "system/process.h"
#include "system/disk.h"
#include "vfs/vfs.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Text editor commands (nano-like)
void nano_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: nano <filename>\n");
        printf("  Simplified nano-like text editor\n");
        printf("  Commands shown in editor interface\n");
        return;
    }
    
    const char* filename = argv[1];
    
    // Initialize editor
    EditorState* editor = editor_create(10000);
    if (!editor) {
        printf("Error: Failed to initialize editor\n");
        return;
    }
    
    // Try to load file if it exists
    VNode* node = vfs_find_node(filename);
    if (node && !node->is_directory) {
        if (editor_load_file(editor, filename) != 0) {
            printf("Warning: Could not load file %s, creating new file\n", filename);
        } else {
            printf("Loaded file: %s\n", filename);
        }
    } else {
        printf("Creating new file: %s\n", filename);
        strncpy(editor->filename, filename, sizeof(editor->filename) - 1);
    }
    
    // Run editor
    printf("\nStarting editor...\n");
#ifdef _WIN32
    Sleep(500); // Brief pause before editor starts
#else
    sleep(1);
#endif
    editor_run(editor);
    
    // Cleanup
    editor_destroy(editor);
    
    printf("\nEditor closed.\n");
}

void editor_command(int argc, char **argv) {
    // Alias for nano
    nano_command(argc, argv);
}

// Enhanced disk information command
void diskinfo_command(int argc, char **argv) {
    printf("=== Disk Information ===\n\n");
    
    const char* path = (argc > 1) ? argv[1] : "/";
    
    DiskInfo info;
    if (disk_get_info(path, &info) == 0) {
        char total_buf[64], used_buf[64], avail_buf[64];
        
        disk_format_size(info.total_size, total_buf, sizeof(total_buf));
        disk_format_size(info.used_size, used_buf, sizeof(used_buf));
        disk_format_size(info.available_size, avail_buf, sizeof(avail_buf));
        
        printf("Mount Point:    %s\n", info.mount_point);
        printf("Filesystem:     %s\n", info.filesystem_type);
        printf("Total Size:     %s\n", total_buf);
        printf("Used:           %s\n", used_buf);
        printf("Available:      %s\n", avail_buf);
        printf("Usage:          %d%%\n", info.usage_percent);
        printf("Total Inodes:   %llu\n", (unsigned long long)info.total_inodes);
        printf("Used Inodes:    %llu\n", (unsigned long long)info.used_inodes);
        printf("Free Inodes:    %llu\n", (unsigned long long)info.available_inodes);
        printf("Read-only:      %s\n", info.readonly ? "Yes" : "No");
    } else {
        printf("Error: Could not get disk information for %s\n", path);
    }
}

// Disk quota management
void quota_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: quota [user]\n");
        printf("  quota <username>     - Show quota for user\n");
        return;
    }
    
    const char* username = argv[1];
    uint64_t quota_bytes = 0, used_bytes = 0;
    
    if (disk_get_quota(username, &quota_bytes, &used_bytes) == 0) {
        char used_buf[64], limit_buf[64];
        
        disk_format_size(used_bytes, used_buf, sizeof(used_buf));
        disk_format_size(quota_bytes, limit_buf, sizeof(limit_buf));
        
        printf("=== Disk Quota for %s ===\n", username);
        printf("Usage:    %s / %s\n", used_buf, limit_buf);
        printf("Percent:  %.1f%%\n", (used_bytes * 100.0) / quota_bytes);
        
        if (disk_check_quota(username) != 0) {
            printf("\n*** WARNING: Quota exceeded! ***\n");
        }
    } else {
        printf("Error: Could not get quota information for %s\n", username);
    }
}

// Enhanced process listing
void ps_enhanced_command(int argc, char **argv) {
    printf("=== Process List ===\n");
    printf("%-6s %-6s %-20s %-10s %-8s %-6s %s\n",
           "PID", "PPID", "NAME", "STATE", "MEMORY", "CPU%%", "ARGS");
    printf("────────────────────────────────────────────────────────────────────────────────\n");
    
    Process** proc_list;
    int count = process_list(&proc_list);
    
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            Process* p = proc_list[i];
            
            const char* state_str;
            switch (p->state) {
                case PROC_STATE_RUNNING:  state_str = "Running"; break;
                case PROC_STATE_SLEEPING: state_str = "Sleeping"; break;
                case PROC_STATE_STOPPED:  state_str = "Stopped"; break;
                case PROC_STATE_ZOMBIE:   state_str = "Zombie"; break;
                default:                  state_str = "Unknown"; break;
            }
            
            char mem_buf[32];
            disk_format_size(p->memory_used, mem_buf, sizeof(mem_buf));
            
            printf("%-6d %-6d %-20s %-10s %-8s %5.1f%% %s\n",
                   p->pid, p->ppid, p->name, state_str, mem_buf,
                   p->cpu_percent, p->args);
        }
        
        free(proc_list);
        
        printf("\nTotal processes: %d\n", count);
        
        char total_mem[64];
        disk_format_size(process_get_total_memory(), total_mem, sizeof(total_mem));
        printf("Total memory used: %s\n", total_mem);
    } else {
        printf("No processes found.\n");
    }
}

// Enhanced kill command
void kill_enhanced_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: kill [signal] <pid>\n");
        printf("Signals:\n");
        printf("  -TERM, -15    Terminate process (default)\n");
        printf("  -KILL, -9     Force kill process\n");
        printf("  -STOP, -19    Stop process\n");
        printf("  -CONT, -18    Continue stopped process\n");
        return;
    }
    
    int signal = PROC_SIG_TERM;
    int pid;
    
    if (argv[1][0] == '-') {
        // Parse signal
        if (strcmp(argv[1], "-KILL") == 0 || strcmp(argv[1], "-9") == 0) {
            signal = PROC_SIG_KILL;
        } else if (strcmp(argv[1], "-TERM") == 0 || strcmp(argv[1], "-15") == 0) {
            signal = PROC_SIG_TERM;
        } else if (strcmp(argv[1], "-STOP") == 0 || strcmp(argv[1], "-19") == 0) {
            signal = PROC_SIG_STOP;
        } else if (strcmp(argv[1], "-CONT") == 0 || strcmp(argv[1], "-18") == 0) {
            signal = PROC_SIG_CONT;
        }
        
        if (argc < 3) {
            printf("Error: PID required\n");
            return;
        }
        pid = atoi(argv[2]);
    } else {
        pid = atoi(argv[1]);
    }
    
    if (process_kill(pid, signal) == 0) {
        printf("Signal %d sent to process %d\n", signal, pid);
    } else {
        printf("Error: Could not send signal to process %d\n", pid);
    }
}

// Process kill by name
void pkill_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pkill <process_name>\n");
        printf("  Kill all processes matching name\n");
        return;
    }
    
    const char* pattern = argv[1];
    int* pid_list;
    int count = process_find_by_name_pattern(pattern, &pid_list);
    
    if (count > 0) {
        printf("Found %d processes matching '%s'\n", count, pattern);
        
        for (int i = 0; i < count; i++) {
            Process* p = process_get(pid_list[i]);
            if (p) {
                printf("  Killing: %d (%s)\n", p->pid, p->name);
                process_kill(p->pid, PROC_SIG_TERM);
            }
        }
        
        free(pid_list);
        printf("Done.\n");
    } else {
        printf("No processes found matching '%s'\n", pattern);
    }
}

// Process grep (find by name)
void pgrep_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pgrep <process_name>\n");
        printf("  List PIDs of processes matching name\n");
        return;
    }
    
    const char* pattern = argv[1];
    int* pid_list;
    int count = process_find_by_name_pattern(pattern, &pid_list);
    
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            printf("%d\n", pid_list[i]);
        }
        free(pid_list);
    }
}
