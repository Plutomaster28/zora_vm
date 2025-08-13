#include "perl_vm.h"
#include "sandbox.h"  // Add this missing include
#include "vfs/vfs.h"
#include "desktop/desktop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PerlVM perl_vm = {0};
static int perl_last_window_id = 0; // Track last created desktop window

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

// Add the missing execute_perl_statement function
static int execute_perl_statement(const char* line) {
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
    
    // Desktop integration commands (simple line-based DSL)
    if (strncmp(start, "desktop_create_window", 22) == 0) {
        // Format: desktop_create_window "Title" WIDTH HEIGHT
        const char* p = start + 22;
        while (*p == ' ' || *p == '\t') p++;
        char title[128] = "Window";
        int width = 640, height = 480;
        if (*p == '"') {
            p++;
            const char* q = strchr(p, '"');
            if (q) {
                size_t len = q - p; if (len >= sizeof(title)) len = sizeof(title)-1;
                strncpy(title, p, len); title[len] = '\0';
                p = q + 1;
            }
        }
        sscanf(p, "%d %d", &width, &height);
        perl_last_window_id = desktop_create_window(title, width, height);
        printf("Perl: Created window id=%d title='%s'\n", perl_last_window_id, title);
        free(trimmed); return 0;
    }
    if (strncmp(start, "desktop_add_label", 18) == 0) {
        // Format: desktop_add_label ID "Text"
        int id = perl_last_window_id; // default to last
        const char* p = start + 18; while (*p==' '||*p=='\t') p++;
        if (*p >= '0' && *p <= '9') { id = atoi(p); while (*p && *p!=' ' && *p!='\t') p++; }
        while (*p==' '||*p=='\t') p++;
        char text[256] = "Label";
        if (*p == '"') { p++; const char* q = strchr(p,'"'); if (q) { size_t len = q-p; if(len>=sizeof(text)) len=sizeof(text)-1; strncpy(text,p,len); text[len]='\0'; } }
        desktop_add_label(id, text);
        free(trimmed); return 0;
    }
    if (strncmp(start, "desktop_show_window", 20) == 0) {
        int id = perl_last_window_id;
        const char* p = start + 20; while (*p==' '||*p=='\t') p++;
        if (*p >= '0' && *p <= '9') id = atoi(p);
        desktop_show_window(id);
        free(trimmed); return 0;
    }
    if (strncmp(start, "desktop_run_loop", 16) == 0) {
        desktop_run_loop();
        free(trimmed); return 0;
    }
    if (strncmp(start, "desktop_theme", 13) == 0) {
        const char* p = start + 13; while (*p==' '||*p=='\t') p++;
        if (*p) { desktop_switch_theme(p); }
        free(trimmed); return 0;
    }
    if (strncmp(start, "desktop_list_themes", 20) == 0) {
        desktop_list_themes(); free(trimmed); return 0; }

    // Handle print statements
    if (strncmp(start, "print ", 6) == 0) {
        char* content = start + 6;
        // Simple print handling - remove quotes if present
        if (content[0] == '"' && content[strlen(content)-1] == '"') {
            content[strlen(content)-1] = '\0';
            content++;
        }
        printf("%s\n", content);
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
    
    // Handle variable assignments (placeholder, no storage)
    if (strchr(start, '=')) {
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
    
    // Handle subroutine definitions
    if (strncmp(start, "sub ", 4) == 0) {
        printf("Subroutine definition: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle use statements
    if (strncmp(start, "use ", 4) == 0) {
        printf("Use statement: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Handle function calls
    if (strchr(start, '(') && strchr(start, ')')) {
        printf("Function call: %s\n", start);
        free(trimmed);
        return 0;
    }
    
    // Default: just echo the statement
    printf("Perl statement: %s\n", start);
    free(trimmed);
    return 0;
}

// Add sandbox checks to Perl execution
int perl_vm_execute_string(const char* code) {
    if (!perl_vm.initialized) {
        return -1;
    }
    
    // Check sandbox restrictions
    if (sandbox_is_strict_mode()) {
        // Block dangerous Perl operations
        if (strstr(code, "system(") || strstr(code, "exec(") || strstr(code, "`")) {
            printf("Perl VM: System command blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "eval ") && strstr(code, "system")) {
            printf("Perl VM: Dangerous eval blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "open(") && strstr(code, "|")) {
            printf("Perl VM: Pipe operation blocked by sandbox\n");
            return -1;
        }
    }
    
    if (sandbox_is_filesystem_blocked()) {
        // Block file operations outside VFS
        if (strstr(code, "File::Copy") || strstr(code, "copy(")) {
            printf("Perl VM: File copy operation blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "glob(") && (strstr(code, "/etc/") || strstr(code, "C:/Windows/"))) {
            printf("Perl VM: Suspicious glob blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "open(") && (strstr(code, "C:\\") || strstr(code, "/etc/") || strstr(code, "../"))) {
            printf("Perl VM: Suspicious file access blocked by sandbox\n");
            return -1;
        }
    }
    
    if (sandbox_is_network_blocked()) {
        if (strstr(code, "Net::") || strstr(code, "LWP::") || strstr(code, "HTTP::")) {
            printf("Perl VM: Network module blocked by sandbox\n");
            return -1;
        }
        
        if (strstr(code, "Socket") || strstr(code, "IO::Socket")) {
            printf("Perl VM: Socket operation blocked by sandbox\n");
            return -1;
        }
    }
    
    // Continue with execution if not blocked
    printf("Executing Perl code: %s\n", code);
    
    // Process the Perl code line by line
    char* code_copy = strdup(code);
    char* line = strtok(code_copy, "\n");
    
    while (line != NULL) {
        if (execute_perl_statement(line) != 0) {
            free(code_copy);
            return -1;
        }
        line = strtok(NULL, "\n");
    }
    
    free(code_copy);
    return 0;
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
        printf("Perl VM executing...\n");
        return perl_vm_execute_string((char*)node->data);
    }
    
    return -1;
}