#ifndef SHELL_SCRIPT_H
#define SHELL_SCRIPT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum limits
#define MAX_VARIABLES 100
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 512
#define MAX_FUNCTIONS 50
#define MAX_FUNCTION_NAME 64
#define MAX_FUNCTION_BODY 2048
#define MAX_PIPELINE_STAGES 10
#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 256

// Variable types
typedef enum {
    VAR_STRING,
    VAR_INTEGER,
    VAR_ARRAY,
    VAR_OBJECT
} VarType;

// Variable structure
typedef struct {
    char name[MAX_VAR_NAME];
    VarType type;
    union {
        char string_val[MAX_VAR_VALUE];
        int int_val;
        struct {
            char** items;
            int count;
        } array_val;
        struct {
            char** keys;
            char** values;
            int count;
        } object_val;
    } value;
} Variable;

// Function structure
typedef struct {
    char name[MAX_FUNCTION_NAME];
    char body[MAX_FUNCTION_BODY];
    char** params;
    int param_count;
} Function;

// Pipeline stage structure
typedef struct {
    char command[MAX_TOKEN_LEN];
    char** args;
    int argc;
} PipelineStage;

// Enhanced shell context
typedef struct {
    Variable variables[MAX_VARIABLES];
    int var_count;
    Function functions[MAX_FUNCTIONS];
    int func_count;
    int last_exit_code;
    char last_output[MAX_VAR_VALUE];
} ShellContext;

// Global shell context
extern ShellContext shell_ctx;

// Core functions
void init_shell_scripting(void);
int execute_script_line(const char* line);
int execute_script_file(const char* filename);

// Variable management
int set_variable(const char* name, const char* value, VarType type);
Variable* get_variable(const char* name);
char* expand_variables(const char* input);
void list_variables(void);

// Function management
int define_function(const char* name, const char* params[], int param_count, const char* body);
int call_function(const char* name, const char* args[], int argc);
void list_functions(void);

// Control structures
int execute_if_statement(const char* condition, const char* then_block, const char* else_block);
int execute_for_loop(const char* var, const char* range, const char* body);
int execute_while_loop(const char* condition, const char* body);
int execute_switch_statement(const char* variable, const char** cases, const char** actions, int case_count);

// Pipeline operations
int execute_pipeline(const char* pipeline_str);
PipelineStage* parse_pipeline(const char* pipeline_str, int* stage_count);
char* apply_pipeline_filter(const char* input, const char* filter);

// Conditional evaluation
int evaluate_condition(const char* condition);
int compare_values(const char* left, const char* operator, const char* right);

// Object operations
int create_object(const char* name, const char** keys, const char** values, int count);
char* get_object_property(const char* object_name, const char* property);
int set_object_property(const char* object_name, const char* property, const char* value);

// Array operations
int create_array(const char* name, const char** items, int count);
char* get_array_item(const char* array_name, int index);
int add_array_item(const char* array_name, const char* item);
int get_array_length(const char* array_name);

// Utility functions
char** tokenize(const char* input, const char* delimiters, int* count);
void free_tokens(char** tokens, int count);
char* trim_whitespace(char* str);
int is_numeric(const char* str);

#endif // SHELL_SCRIPT_H
