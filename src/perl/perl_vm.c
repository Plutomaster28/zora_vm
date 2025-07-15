// Ensure src/perl/perl_vm.c exists with this content:

#include "perl_vm.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PerlVM perl_vm = {0};

// Initialize Perl VM (simplified)
int perl_vm_init(void) {
    if (perl_vm.initialized) {
        return 0;
    }
    
    perl_vm.initialized = 1;
    printf("Perl VM initialized successfully (simplified mode)\n");
    return 0;
}

// Cleanup Perl VM
void perl_vm_cleanup(void) {
    if (perl_vm.initialized) {
        if (perl_vm.current_script) {
            free(perl_vm.current_script);
        }
        perl_vm.initialized = 0;
    }
}

// Execute Perl string (simplified)
int perl_vm_execute_string(const char* code) {
    if (!perl_vm.initialized) {
        return -1;
    }
    
    // Create a temporary file for the Perl code
    char temp_file[] = "temp_perl.pl";
    FILE* f = fopen(temp_file, "w");
    if (!f) {
        return -1;
    }
    
    // Add safe Perl code wrapper
    fprintf(f, "#!/usr/bin/perl\n");
    fprintf(f, "use strict;\n");
    fprintf(f, "use warnings;\n");
    fprintf(f, "print \"Perl VM executing...\\n\";\n");
    fprintf(f, "%s\n", code);
    fclose(f);
    
    // Execute via system perl (in sandbox)
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "perl %s", temp_file);
    
    printf("Executing Perl code: %s\n", code);
    int result = system(cmd);
    
    // Clean up
    remove(temp_file);
    
    return result;
}

// Load and execute script from VFS
int perl_vm_load_script(const char* vm_path) {
    if (!perl_vm.initialized) {
        return -1;
    }
    
    VNode* node = vfs_find_node(vm_path);
    if (!node || node->is_directory) {
        printf("Perl script not found: %s\n", vm_path);
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
        return perl_vm_execute_string((char*)node->data);
    }
    
    return -1;
}