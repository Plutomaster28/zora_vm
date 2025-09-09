#ifndef MERL_H
#define MERL_H

#include <stdint.h>
#include <stdio.h>

// MERL shell integration with VM
typedef struct {
    void* cpu_context;
    void* memory_context;
    void* device_context;
    void* vm_context;
    int running;
} merl_vm_context_t;

// Core MERL functions
int merl_init(void);
void merl_cleanup(void);
int merl_run(void);
int merl_execute_command(const char* command);

// VM integration functions
void merl_set_vm_context(void* vm);
void merl_set_hardware_context(void* cpu, void* memory, void* devices);

// Enhanced commands for VM environment
int merl_cmd_vmstat(void);
int merl_cmd_cpuinfo(void);
int merl_cmd_meminfo(void);
int merl_cmd_devices(void);
int merl_cmd_reboot(void);

// UTF-8 fix functions
void ghost_pipe_utf8_fix(void);

extern merl_vm_context_t* g_merl_vm_ctx;

#endif // MERL_H