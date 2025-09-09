#include "shell_script.h"
#include "shell.h"
#include "user.h"
#include <ctype.h>

// Forward declaration
extern void handle_command(char *command);

// Global shell context
ShellContext shell_ctx;

// Initialize shell scripting system
void init_shell_scripting(void) {
    memset(&shell_ctx, 0, sizeof(ShellContext));
    shell_ctx.var_count = 0;
    shell_ctx.func_count = 0;
    shell_ctx.last_exit_code = 0;
    
    // Set default variables
    set_variable("PWD", "/", VAR_STRING);
    set_variable("USER", "guest", VAR_STRING);
    set_variable("HOME", "/home", VAR_STRING);
    set_variable("PATH", "/bin:/usr/bin", VAR_STRING);
    
    printf("Enhanced shell scripting initialized!\n");
}

// Set a variable
int set_variable(const char* name, const char* value, VarType type) {
    if (!name || !value) return -1;
    
    // Check if variable already exists
    Variable* existing = get_variable(name);
    if (existing) {
        // Update existing variable
        existing->type = type;
        switch (type) {
            case VAR_STRING:
                strncpy(existing->value.string_val, value, MAX_VAR_VALUE - 1);
                existing->value.string_val[MAX_VAR_VALUE - 1] = '\0';
                break;
            case VAR_INTEGER:
                existing->value.int_val = atoi(value);
                break;
            default:
                strncpy(existing->value.string_val, value, MAX_VAR_VALUE - 1);
                existing->value.string_val[MAX_VAR_VALUE - 1] = '\0';
                break;
        }
        return 0;
    }
    
    // Create new variable
    if (shell_ctx.var_count >= MAX_VARIABLES) return -1;
    
    Variable* var = &shell_ctx.variables[shell_ctx.var_count];
    strncpy(var->name, name, MAX_VAR_NAME - 1);
    var->name[MAX_VAR_NAME - 1] = '\0';
    var->type = type;
    
    switch (type) {
        case VAR_STRING:
            strncpy(var->value.string_val, value, MAX_VAR_VALUE - 1);
            var->value.string_val[MAX_VAR_VALUE - 1] = '\0';
            break;
        case VAR_INTEGER:
            var->value.int_val = atoi(value);
            break;
        default:
            strncpy(var->value.string_val, value, MAX_VAR_VALUE - 1);
            var->value.string_val[MAX_VAR_VALUE - 1] = '\0';
            break;
    }
    
    shell_ctx.var_count++;
    return 0;
}

// Get a variable
Variable* get_variable(const char* name) {
    if (!name) return NULL;
    
    for (int i = 0; i < shell_ctx.var_count; i++) {
        if (strcmp(shell_ctx.variables[i].name, name) == 0) {
            return &shell_ctx.variables[i];
        }
    }
    return NULL;
}

// Expand variables in a string (e.g., $VAR or ${VAR})
char* expand_variables(const char* input) {
    if (!input) return NULL;
    
    static char expanded[MAX_VAR_VALUE * 2];
    char* result = expanded;
    const char* ptr = input;
    
    *result = '\0';
    
    while (*ptr) {
        if (*ptr == '$') {
            ptr++; // Skip $
            
            char var_name[MAX_VAR_NAME];
            int name_len = 0;
            
            // Handle ${VAR} or $VAR
            int brace_mode = 0;
            if (*ptr == '{') {
                brace_mode = 1;
                ptr++; // Skip {
            }
            
            // Extract variable name
            while (*ptr && (isalnum(*ptr) || *ptr == '_') && name_len < MAX_VAR_NAME - 1) {
                var_name[name_len++] = *ptr++;
            }
            var_name[name_len] = '\0';
            
            if (brace_mode && *ptr == '}') {
                ptr++; // Skip }
            }
            
            // Look up variable
            Variable* var = get_variable(var_name);
            if (var) {
                switch (var->type) {
                    case VAR_STRING:
                        strcat(result, var->value.string_val);
                        break;
                    case VAR_INTEGER: {
                        char int_str[32];
                        sprintf(int_str, "%d", var->value.int_val);
                        strcat(result, int_str);
                        break;
                    }
                    default:
                        strcat(result, var->value.string_val);
                        break;
                }
            }
        } else {
            // Regular character
            size_t len = strlen(result);
            result[len] = *ptr++;
            result[len + 1] = '\0';
        }
    }
    
    return expanded;
}

// List all variables
void list_variables(void) {
    printf("=== Shell Variables ===\n");
    for (int i = 0; i < shell_ctx.var_count; i++) {
        Variable* var = &shell_ctx.variables[i];
        printf("%-15s = ", var->name);
        
        switch (var->type) {
            case VAR_STRING:
                printf("\"%s\" (string)\n", var->value.string_val);
                break;
            case VAR_INTEGER:
                printf("%d (integer)\n", var->value.int_val);
                break;
            case VAR_ARRAY:
                printf("[array with %d items]\n", var->value.array_val.count);
                break;
            case VAR_OBJECT:
                printf("{object with %d properties}\n", var->value.object_val.count);
                break;
        }
    }
    printf("Total variables: %d\n", shell_ctx.var_count);
}

// Evaluate a condition (supports ==, !=, -gt, -lt, -eq, -ne, etc.)
int evaluate_condition(const char* condition) {
    if (!condition) return 0;
    
    char* expanded = expand_variables(condition);
    if (!expanded) return 0;
    
    // Parse condition: left operator right
    char** tokens = tokenize(expanded, " \t", NULL);
    if (!tokens) return 0;
    
    int token_count = 0;
    while (tokens[token_count]) token_count++;
    
    if (token_count >= 3) {
        char* left = tokens[0];
        char* op = tokens[1];
        char* right = tokens[2];
        
        int result = compare_values(left, op, right);
        free_tokens(tokens, token_count);
        return result;
    }
    
    free_tokens(tokens, token_count);
    return 0;
}

// Compare two values with an operator
int compare_values(const char* left, const char* operator, const char* right) {
    if (!left || !operator || !right) return 0;
    
    // String comparisons
    if (strcmp(operator, "==") == 0 || strcmp(operator, "-eq") == 0) {
        return strcmp(left, right) == 0;
    }
    if (strcmp(operator, "!=") == 0 || strcmp(operator, "-ne") == 0) {
        return strcmp(left, right) != 0;
    }
    
    // Numeric comparisons
    if (is_numeric(left) && is_numeric(right)) {
        int left_val = atoi(left);
        int right_val = atoi(right);
        
        if (strcmp(operator, "-gt") == 0 || strcmp(operator, ">") == 0) {
            return left_val > right_val;
        }
        if (strcmp(operator, "-lt") == 0 || strcmp(operator, "<") == 0) {
            return left_val < right_val;
        }
        if (strcmp(operator, "-ge") == 0 || strcmp(operator, ">=") == 0) {
            return left_val >= right_val;
        }
        if (strcmp(operator, "-le") == 0 || strcmp(operator, "<=") == 0) {
            return left_val <= right_val;
        }
    }
    
    // Pattern matching
    if (strcmp(operator, "-like") == 0) {
        // Simple wildcard matching (basic implementation)
        return strstr(left, right) != NULL;
    }
    
    return 0;
}

// Execute if statement
int execute_if_statement(const char* condition, const char* then_block, const char* else_block) {
    if (!condition || !then_block) return -1;
    
    if (evaluate_condition(condition)) {
        return execute_script_line(then_block);
    } else if (else_block) {
        return execute_script_line(else_block);
    }
    
    return 0;
}

// Execute for loop
int execute_for_loop(const char* var, const char* range, const char* body) {
    if (!var || !range || !body) return -1;
    
    // Simple range implementation: for i in 1..10 or for file in *.txt
    if (strstr(range, "..")) {
        // Numeric range: 1..10
        char* range_copy = strdup(range);
        char* dot_pos = strstr(range_copy, "..");
        if (dot_pos) {
            *dot_pos = '\0';
            int start = atoi(range_copy);
            int end = atoi(dot_pos + 2);
            
            for (int i = start; i <= end; i++) {
                char val[32];
                sprintf(val, "%d", i);
                set_variable(var, val, VAR_INTEGER);
                execute_script_line(body);
            }
        }
        free(range_copy);
    } else {
        // File glob or array (simplified)
        // For now, treat as space-separated list
        char** items = tokenize(range, " \t", NULL);
        if (items) {
            int i = 0;
            while (items[i]) {
                set_variable(var, items[i], VAR_STRING);
                execute_script_line(body);
                i++;
            }
            free_tokens(items, i);
        }
    }
    
    return 0;
}

// Execute while loop
int execute_while_loop(const char* condition, const char* body) {
    if (!condition || !body) return -1;
    
    while (evaluate_condition(condition)) {
        if (execute_script_line(body) != 0) {
            break; // Exit on error
        }
    }
    
    return 0;
}

// Execute a script line
int execute_script_line(const char* line) {
    if (!line) return -1;
    
    char* expanded = expand_variables(line);
    if (!expanded) return -1;
    
    // Trim whitespace
    char* trimmed = trim_whitespace(strdup(expanded));
    if (!trimmed) return -1;
    
    // Skip empty lines and comments
    if (strlen(trimmed) == 0 || trimmed[0] == '#') {
        free(trimmed);
        return 0;
    }
    
    // Check for control structures
    if (strncmp(trimmed, "if ", 3) == 0) {
        // Parse if statement: if [condition]; then [command]; else [command]; fi
        // Simplified parsing for now
        free(trimmed);
        return 0;
    }
    
    if (strncmp(trimmed, "for ", 4) == 0) {
        // Parse for loop: for var in range; do [body]; done
        free(trimmed);
        return 0;
    }
    
    if (strncmp(trimmed, "while ", 6) == 0) {
        // Parse while loop: while [condition]; do [body]; done
        free(trimmed);
        return 0;
    }
    
    // Check for variable assignment
    char* equals = strchr(trimmed, '=');
    if (equals && equals > trimmed) {
        *equals = '\0';
        char* var_name = trim_whitespace(trimmed);
        char* var_value = trim_whitespace(equals + 1);
        
        // Remove quotes if present
        if (var_value[0] == '"' || var_value[0] == '\'') {
            int len = strlen(var_value);
            if (len > 1 && var_value[len-1] == var_value[0]) {
                var_value[len-1] = '\0';
                var_value++;
            }
        }
        
        set_variable(var_name, var_value, is_numeric(var_value) ? VAR_INTEGER : VAR_STRING);
        free(trimmed);
        return 0;
    }
    
    // Check for pipeline
    if (strchr(trimmed, '|')) {
        int result = execute_pipeline(trimmed);
        free(trimmed);
        return result;
    }
    
    // Execute as regular command
    handle_command(trimmed);
    free(trimmed);
    return 0;
}

// Tokenize a string
char** tokenize(const char* input, const char* delimiters, int* count) {
    if (!input || !delimiters) return NULL;
    
    char* input_copy = strdup(input);
    if (!input_copy) return NULL;
    
    char** tokens = malloc(MAX_TOKENS * sizeof(char*));
    if (!tokens) {
        free(input_copy);
        return NULL;
    }
    
    int token_count = 0;
    char* token = strtok(input_copy, delimiters);
    
    while (token && token_count < MAX_TOKENS - 1) {
        tokens[token_count] = strdup(token);
        token_count++;
        token = strtok(NULL, delimiters);
    }
    
    tokens[token_count] = NULL;
    free(input_copy);
    
    if (count) *count = token_count;
    return tokens;
}

// Free tokenized array
void free_tokens(char** tokens, int count) {
    if (!tokens) return;
    
    for (int i = 0; i < count && tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// Trim whitespace from string
char* trim_whitespace(char* str) {
    if (!str) return NULL;
    
    // Trim leading whitespace
    while (isspace(*str)) str++;
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }
    
    return str;
}

// Check if string is numeric
int is_numeric(const char* str) {
    if (!str || *str == '\0') return 0;
    
    char* endptr;
    strtol(str, &endptr, 10);
    return *endptr == '\0';
}

// Basic pipeline execution
int execute_pipeline(const char* pipeline_str) {
    if (!pipeline_str) return -1;
    
    printf("Pipeline execution: %s\n", pipeline_str);
    // TODO: Implement full pipeline processing
    return 0;
}

// Execute script file
int execute_script_file(const char* filename) {
    if (!filename) return -1;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open script file '%s'\n", filename);
        return -1;
    }
    
    char line[1024];
    int line_number = 0;
    int errors = 0;
    
    printf("Executing script: %s\n", filename);
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines and comments
        char* trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }
        
        printf("[%d] %s\n", line_number, trimmed);
        
        if (execute_script_line(trimmed) != 0) {
            printf("Error on line %d: %s\n", line_number, trimmed);
            errors++;
        }
    }
    
    fclose(file);
    
    if (errors > 0) {
        printf("Script completed with %d error(s)\n", errors);
        return -1;
    }
    
    return 0;
}
