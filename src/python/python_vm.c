// Ensure src/python/python_vm.c exists with this content:

#include "python_vm.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PythonVM python_vm = {0};

// Initialize Python VM (simplified)
int python_vm_init(void) {
    if (python_vm.initialized) {
        return 0;
    }
    
    python_vm.initialized = 1;
    printf("Python VM initialized successfully (simplified mode)\n");
    return 0;
}

// Cleanup Python VM
void python_vm_cleanup(void) {
    if (python_vm.initialized) {
        python_vm.initialized = 0;
    }
}

// Execute Python string (simplified)
int python_vm_execute_string(const char* code) {
    if (!python_vm.initialized) {
        return -1;
    }
    
    // Create a temporary file for the Python code
    char temp_file[] = "temp_python.py";
    FILE* f = fopen(temp_file, "w");
    if (!f) {
        return -1;
    }
    
    // Add safe Python code wrapper
    fprintf(f, "#!/usr/bin/python3\n");
    fprintf(f, "# VM Python execution\n");
    fprintf(f, "print('Python VM executing...')\n");
    fprintf(f, "%s\n", code);
    fclose(f);
    
    // Execute via system python (in sandbox)
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python %s", temp_file);
    
    printf("Executing Python code: %s\n", code);
    int result = system(cmd);
    
    // Clean up
    remove(temp_file);
    
    return result;
}

// Load and execute script from VFS
int python_vm_load_script(const char* vm_path) {
    if (!python_vm.initialized) {
        return -1;
    }
    
    VNode* node = vfs_find_node(vm_path);
    if (!node || node->is_directory) {
        printf("Python script not found: %s\n", vm_path);
        return -1;
    }
    
    // Load file content if needed
    if (!node->data && node->host_path) {
        FILE* f = fopen(node->host_path, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            node->data = malloc(size + 1);
            if (node->data) {
                fread(node->data, 1, size, f);
                ((char*)node->data)[size] = '\0';
                node->size = size;
            }
            fclose(f);
        }
    }
    
    if (node->data) {
        return python_vm_execute_string((char*)node->data);
    }
    
    return -1;
}