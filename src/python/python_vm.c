#include "python_vm.h"
#include "sandbox.h"
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

// Simple Python-like statement executor
static int execute_python_statement(const char* line) {
    char* trimmed = strdup(line);
    
    // Remove leading/trailing whitespace
    char* start = trimmed;
    while (*start == ' ' || *start == '\t') start++;
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end + 1) = '\0';
    
    // Skip empty lines and comments
    if (*start == '\0' || *start == '#') {
        free(trimmed);
        return 0;
    }
    
    // Handle print statements
    if (strncmp(start, "print(", 6) == 0) {
        char* content = start + 6;
        char* end_paren = strrchr(content, ')');
        if (end_paren) {
            *end_paren = '\0';
            
            // Handle string literals
            if (content[0] == '"' || content[0] == '\'') {
                char quote = content[0];
                content++;
                char* end_quote = strrchr(content, quote);
                if (end_quote) *end_quote = '\0';
            }
            
            // Handle f-strings and variables
            if (strncmp(content, "f\"", 2) == 0 || strncmp(content, "f'", 2) == 0) {
                content += 2;
                char* end_quote = strrchr(content, content[-1]);
                if (end_quote) *end_quote = '\0';
                
                // Simple f-string processing
                printf("%s\n", content);
            } else {
                printf("%s\n", content);
            }
        }
        free(trimmed);
        return 0;
    }
    
    // Handle vm_print function
    if (strncmp(start, "vm_print(", 9) == 0) {
        char* content = start + 9;
        char* end_paren = strrchr(content, ')');
        if (end_paren) {
            *end_paren = '\0';
            
            // Handle string literals
            if (content[0] == '"' || content[0] == '\'') {
                char quote = content[0];
                content++;
                char* end_quote = strrchr(content, quote);
                if (end_quote) *end_quote = '\0';
            }
            
            printf("%s\n", content);
        }
        free(trimmed);
        return 0;
    }
    
    // Handle variable assignments
    if (strchr(start, '=')) {
        // Simple variable assignment handling
        printf("Variable assignment: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle for loops
    if (strncmp(start, "for ", 4) == 0) {
        printf("For loop: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle if statements
    if (strncmp(start, "if ", 3) == 0) {
        printf("If statement: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle try/except
    if (strncmp(start, "try:", 4) == 0) {
        printf("Try block: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle function calls
    if (strchr(start, '(') && strchr(start, ')')) {
        printf("Function call: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle import statements
    if (strncmp(start, "import ", 7) == 0) {
        printf("Import statement: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Default: just echo the statement
    printf("Python statement: %s\n", start);
    free(trimmed);
    return 0;
}

// Update the sandbox checking to be more selective
int python_vm_execute_string(const char* code) {
    if (!python_vm.initialized) {
        return -1;
    }
    
    // Check sandbox restrictions with more granular control
    if (sandbox_is_strict_mode()) {
        // Only block truly dangerous operations, not basic ones
        if (strstr(code, "subprocess.run") && strstr(code, "shell=True")) {
            printf("Python VM: Shell subprocess blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "os.system(")) {
            printf("Python VM: os.system() blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "eval(") && strstr(code, "__import__")) {
            printf("Python VM: Dangerous eval blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "exec(")) {
            printf("Python VM: exec() blocked by sandbox\n");
            return -1;
        }
    }
    
    if (sandbox_is_filesystem_blocked()) {
        // Block file operations that go outside VFS
        if (strstr(code, "shutil.copy") || strstr(code, "shutil.move")) {
            printf("Python VM: File copy operation blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "open(") && (strstr(code, "C:\\") || strstr(code, "/etc/") || strstr(code, "../"))) {
            printf("Python VM: Suspicious file access blocked by sandbox\n");
            return -1;
        }
    }
    
    if (sandbox_is_network_blocked()) {
        if (strstr(code, "socket.") || strstr(code, "urllib.") || 
            strstr(code, "requests.") || strstr(code, "http.client")) {
            printf("Python VM: Network operation blocked by sandbox\n");
            return -1;
        }
    }
    
    // Allow the code to execute if not blocked
    printf("Executing Python code: %s\n", code);
    
    // Split code into lines and execute each
    char* code_copy = strdup(code);
    char* line = strtok(code_copy, "\n");
    
    while (line != NULL) {
        if (execute_python_statement(line) != 0) {
            free(code_copy);
            return -1;
        }
        line = strtok(NULL, "\n");
    }
    
    free(code_copy);
    return 0;
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
        printf("Python VM executing...\n");
        return python_vm_execute_string((char*)node->data);
    }
    
    return -1;
}