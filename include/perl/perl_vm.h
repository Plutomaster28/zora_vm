#ifndef PERL_VM_H
#define PERL_VM_H

typedef struct {
    int initialized;
    char* current_script;
} PerlVM;

int perl_vm_init(void);
void perl_vm_cleanup(void);
int perl_vm_execute_string(const char* code);
int perl_vm_load_script(const char* vm_path);

#endif // PERL_VM_H