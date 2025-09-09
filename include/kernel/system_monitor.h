#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdint.h>
#include <time.h>

// System monitoring functions
void system_monitor_init(void);
void system_monitor_update(void);
void system_monitor_display_processes(void);
void system_monitor_display_system_info(void);
void system_monitor_display_filesystems(void);
void system_monitor_display_network_status(void);

// Process management
int system_monitor_add_process(const char* name, int priority);
int system_monitor_kill_process(int pid);

#endif // SYSTEM_MONITOR_H
