#ifndef VM_H
#define VM_H

// Core VM functions
int vm_init(void);
void vm_cleanup(void);
int vm_is_running(void);

// Crash protection control
void vm_init_crash_protection(void);
void vm_enable_crash_guard(void);
void vm_disable_crash_guard(void);

// VM control functions
void vm_trigger_reboot(void);
int vm_is_rebooting(void);

#endif // VM_H