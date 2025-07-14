#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// System call interception
int syscall_init(void);
void syscall_cleanup(void);

// Intercepted system calls
int vm_fclose(FILE* stream);
int vm_rename(const char* old_name, const char* new_name);

// Process operations
int vm_fork(void);
int vm_exec(const char* command);
void vm_exit(int status);

// Note: The following functions are declared in vfs.h to avoid conflicts:
// - vm_system
// - vm_fopen
// - vm_remove
// - vm_getcwd
// - vm_chdir
// - vm_mkdir
// - vm_rmdir
// - vm_ls
// - vm_clear
// - vm_pwd
// - vm_ps (now only in vfs.h)

#endif // SYSCALL_H