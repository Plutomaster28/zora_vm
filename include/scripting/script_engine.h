#ifndef ZORA_SCRIPT_ENGINE_H
#define ZORA_SCRIPT_ENGINE_H

#include <stdint.h>

// Script engine types
typedef enum {
    SCRIPT_ENGINE_LUA,
    SCRIPT_ENGINE_PYTHON,
    SCRIPT_ENGINE_PERL,
    SCRIPT_ENGINE_SHELL,
    SCRIPT_ENGINE_UNKNOWN
} ScriptEngineType;

// Script execution result
typedef struct {
    int exit_code;
    char* output;
    char* error;
    int execution_time_ms;
    uint64_t memory_used;
} ScriptResult;

// Script engine configuration
typedef struct {
    int max_execution_time_ms;  // Maximum execution time
    uint64_t max_memory_bytes;  // Maximum memory usage
    int sandbox_enabled;         // Enable sandboxing
    int allow_file_io;          // Allow file I/O
    int allow_network;          // Allow network access
    int allow_system_calls;     // Allow system calls
    char working_directory[256]; // Script working directory
} ScriptConfig;

// Script engine management
int script_engine_init(void);
void script_engine_cleanup(void);

// Script execution
int script_execute_file(ScriptEngineType type, const char* filepath, 
                        const char* args[], int arg_count,
                        const ScriptConfig* config, ScriptResult* result);

int script_execute_string(ScriptEngineType type, const char* code,
                          const ScriptConfig* config, ScriptResult* result);

// Script detection
ScriptEngineType script_detect_type(const char* filepath);
ScriptEngineType script_detect_type_from_content(const char* content);
ScriptEngineType script_detect_type_from_shebang(const char* filepath);

// Script validation
int script_validate_syntax(ScriptEngineType type, const char* code, 
                           char** error_message);
int script_check_file_syntax(ScriptEngineType type, const char* filepath,
                             char** error_message);

// Engine availability
int script_engine_is_available(ScriptEngineType type);
const char* script_engine_get_version(ScriptEngineType type);
const char* script_engine_get_name(ScriptEngineType type);

// Result management
void script_result_init(ScriptResult* result);
void script_result_free(ScriptResult* result);

// Configuration helpers
void script_config_init_default(ScriptConfig* config);
void script_config_set_sandbox(ScriptConfig* config, int enabled);
void script_config_set_timeout(ScriptConfig* config, int timeout_ms);

// Script library path management
int script_add_library_path(ScriptEngineType type, const char* path);
int script_remove_library_path(ScriptEngineType type, const char* path);
int script_list_library_paths(ScriptEngineType type, char*** paths, int* count);

// Script environment variables
int script_set_env_var(const char* name, const char* value);
int script_get_env_var(const char* name, char* value, size_t size);
int script_unset_env_var(const char* name);

// Error handling
const char* script_get_last_error(void);
void script_clear_error(void);

// Script caching (for performance)
int script_enable_caching(int enabled);
int script_clear_cache(void);
int script_precompile_script(ScriptEngineType type, const char* filepath);

// Multi-threaded execution support
typedef void (*script_callback_t)(ScriptResult* result, void* user_data);

int script_execute_async(ScriptEngineType type, const char* filepath,
                         const char* args[], int arg_count,
                         const ScriptConfig* config,
                         script_callback_t callback, void* user_data);

int script_cancel_execution(int execution_id);

#endif // ZORA_SCRIPT_ENGINE_H
