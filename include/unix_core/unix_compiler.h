#ifndef UNIX_COMPILER_H
#define UNIX_COMPILER_H

// Research UNIX Compiler Toolchain for ZoraVM
// Complete C, Fortran, and Assembly compilation system

#include <stddef.h>

// Compiler types
typedef enum {
    COMPILER_C,
    COMPILER_FORTRAN,
    COMPILER_ASSEMBLY,
    COMPILER_YACC,
    COMPILER_LEX
} CompilerType;

// Compilation options
typedef struct {
    char* input_file;
    char* output_file;
    int compile_only;       // -c flag
    int optimize;           // -O flag
    int debug;              // -g flag
    int verbose;            // -v flag
    char* include_dirs[16]; // -I flags
    char* library_dirs[16]; // -L flags
    char* libraries[16];    // -l flags
    CompilerType type;
} CompileOptions;

// Function prototypes
int unix_compiler_init(void);
int unix_compile_c(CompileOptions* opts);
int unix_compile_fortran(CompileOptions* opts);
int unix_compile_assembly(CompileOptions* opts);
int unix_run_yacc(CompileOptions* opts);
int unix_run_lex(CompileOptions* opts);
int unix_link_objects(char** object_files, int count, char* output);
int unix_parse_compile_args(int argc, char** argv, CompileOptions* opts);
void unix_print_compiler_help(CompilerType type);
void unix_compiler_cleanup(void);

// Debugger functions
int unix_debugger_init(void);
int unix_debug_process(int pid);
int unix_trace_syscalls(int pid);
void unix_print_backtrace(void);
void unix_debugger_cleanup(void);

#endif // UNIX_COMPILER_H
