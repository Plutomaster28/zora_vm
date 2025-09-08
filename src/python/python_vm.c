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
    if (!line) return 0;
    
    size_t line_len = strlen(line);
    if (line_len == 0 || line_len > 1024) return 0; // Skip empty or too long lines
    
    char* trimmed = malloc(line_len + 1);
    if (!trimmed) {
        printf("Python VM: Memory allocation failed for statement\n");
        return -1;
    }
    
    strcpy(trimmed, line);
    
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
    if (strchr(start, '(') && strchr(start, ')') && strstr(start, "print(")) {
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
        printf("Python VM: Not initialized\n");
        return -1;
    }
    
    if (!code) {
        printf("Python VM: No code provided\n");
        return -1;
    }
    
    // Check code length
    size_t code_len = strlen(code);
    if (code_len == 0) {
        printf("Python VM: Empty code string\n");
        return 0; // Empty is OK
    }
    
    if (code_len > 1024*1024) { // Limit to 1MB
        printf("Python VM: Code too large (limit: 1MB)\n");
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
    printf("Python VM: Executing Python code (length: %zu)\n", code_len);
    
    // Split code into lines and execute each
    char* code_copy = malloc(code_len + 1);
    if (!code_copy) {
        printf("Python VM: Memory allocation failed\n");
        return -1;
    }
    
    strcpy(code_copy, code);
    char* line = strtok(code_copy, "\n");
    
    int result = 0;
    while (line != NULL && result == 0) {
        if (execute_python_statement(line) != 0) {
            result = -1;
            break;
        }
        line = strtok(NULL, "\n");
    }
    
    free(code_copy);
    return result;
}

// Load and execute script from VFS
int python_vm_load_script(const char* vm_path) {
    if (!python_vm.initialized) {
        printf("Python VM not initialized\n");
        return -1;
    }
    
    if (!vm_path) {
        printf("Python VM: Invalid script path\n");
        return -1;
    }
    
    VNode* node = vfs_find_node(vm_path);
    if (!node || node->is_directory) {
        printf("Python script not found: %s\n", vm_path);
        return -1;
    }
    
    // Use VFS to read file content
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(vm_path, &data, &size) != 0 || !data) {
        printf("Python VM: Failed to read script: %s\n", vm_path);
        return -1;
    }
    
    printf("Python VM: Executing script %s (size: %zu bytes)\n", vm_path, size);
    return python_vm_execute_string((char*)data);
    
    printf("Python VM: No data available for script: %s\n", vm_path);
    return -1;
}