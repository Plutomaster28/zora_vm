#include "meisei/virtual_silicon.h"
#include "lua/lua_vm.h"
#include "python/python_vm.h"
#include "perl/perl_vm.h"
#include <string.h>  // Add this missing include

// Universal script execution with auto-acceleration
int meisei_execute_script(const char* script, const char* language) {
    printf("Meisei Virtual Silicon: Auto-accelerating %s script\n", language);
    
    // Step 1: Try JIT compilation
    void* optimized_code = NULL;
    if (meisei_jit_compile(script, language, &optimized_code) == 0) {
        // Step 2: Execute with Meisei acceleration
        return meisei_silicon_execute_optimized(script, NULL);
    }
    
    // Step 3: Fallback to native execution if JIT fails
    printf("Meisei: JIT failed, using native %s execution\n", language);
    
    // Use the correct function names (as suggested by compiler)
    if (strcmp(language, "lua") == 0) {
        return lua_vm_execute_string(script);
    } else if (strcmp(language, "python") == 0) {
        return python_vm_execute_string(script);
    } else if (strcmp(language, "perl") == 0) {
        return perl_vm_execute_string(script);
    }
    
    printf("Meisei: Unsupported language: %s\n", language);
    return -1;
}

// Helper function to determine language from file extension
const char* meisei_detect_language(const char* filename) {
    if (!filename) return "unknown";
    
    const char* ext = strrchr(filename, '.');
    if (!ext) return "unknown";
    
    if (strcmp(ext, ".lua") == 0) return "lua";
    if (strcmp(ext, ".py") == 0) return "python";
    if (strcmp(ext, ".pl") == 0 || strcmp(ext, ".perl") == 0) return "perl";
    
    return "unknown";
}

// Convenience functions for direct language execution
int meisei_execute_lua(const char* script) {
    return meisei_execute_script(script, "lua");
}

int meisei_execute_python(const char* script) {
    return meisei_execute_script(script, "python");
}

int meisei_execute_perl(const char* script) {
    return meisei_execute_script(script, "perl");
}

// Execute script file with auto-detection
int meisei_execute_file(const char* filename) {
    const char* language = meisei_detect_language(filename);
    
    if (strcmp(language, "unknown") == 0) {
        printf("Meisei: Cannot determine language for file: %s\n", filename);
        return -1;
    }
    
    // Read file content (simplified - you might want to add file reading logic)
    printf("Meisei: Auto-detected %s script: %s\n", language, filename);
    
    // For now, just indicate the file would be processed
    printf("Meisei: Would execute %s file with acceleration\n", filename);
    return 0;
}