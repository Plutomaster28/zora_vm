#ifndef ZORAPERL_H
#define ZORAPERL_H

#include <stdint.h>
#include <stdio.h>

// ZoraPerl VM integration
typedef struct {
    void* interpreter;
    void* vm_context;
    char* current_dir;
    int running;
} zoraperl_vm_t;

// Core ZoraPerl functions
int zoraperl_init(void);
void zoraperl_cleanup(void);
int zoraperl_start_shell(void);

// VM-specific ZoraPerl functions
int zoraperl_execute_script(const char* script);
int zoraperl_execute_file(const char* filename);
void zoraperl_set_vm_context(void* cpu, void* memory, void* devices);

// Built-in "OS" commands that ZoraPerl can use
int zoraperl_cmd_ls(const char* path);
int zoraperl_cmd_cat(const char* filename);
int zoraperl_cmd_mkdir(const char* dirname);
int zoraperl_cmd_rm(const char* filename);
int zoraperl_cmd_ps(void);
int zoraperl_cmd_shutdown(void);

// Virtual filesystem for the VM
int zoraperl_vfs_init(void);
void zoraperl_vfs_cleanup(void);

extern zoraperl_vm_t* g_zoraperl_vm;

#endif // ZORAPERL_H