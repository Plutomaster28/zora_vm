#ifndef PERL_VM_H
#define PERL_VM_H

typedef struct {
    int initialized;
    char* current_script;
} PerlVM;

// Initialize and cleanup
int perl_vm_init(void);
void perl_vm_cleanup(void);

// Execution functions
int perl_vm_execute_string(const char* code);
int perl_vm_load_script(const char* vm_path);
int perl_vm_load_script_with_args(const char* vm_path, int argc, char** argv);

#endif // PERL_VM_H