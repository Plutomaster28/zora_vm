// include/python/python_vm.h
#ifndef PYTHON_VM_H
#define PYTHON_VM_H

typedef struct {
    int initialized;
    char* current_script;
} PythonVM;

int python_vm_init(void);
void python_vm_cleanup(void);
int python_vm_execute_string(const char* code);
int python_vm_load_script(const char* vm_path);
int python_vm_load_script_with_args(const char* vm_path, int argc, char** argv);

#endif // PYTHON_VM_H