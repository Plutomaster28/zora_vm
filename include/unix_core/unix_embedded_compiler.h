#ifndef UNIX_EMBEDDED_COMPILER_H
#define UNIX_EMBEDDED_COMPILER_H

// ZoraVM Embedded Real Compiler Toolchain
// GCC (GNU C Compiler), NASM x86 Assembler, GFortran Compiler

#include <stddef.h>

// Embedded compiler configuration
typedef struct {
    char* gcc_path;         // Path to embedded GCC
    char* nasm_path;        // Path to embedded NASM
    char* fortran_path;     // Path to embedded GFortran
    char* temp_dir;         // Temporary compilation directory
    char* output_dir;       // Output directory for executables
    int initialized;        // Initialization status
} EmbeddedCompilerConfig;

// Compilation request structure
typedef struct {
    char source_file[512];
    char output_file[512];
    char* source_code;      // In-memory source code
    size_t source_length;
    int compile_only;       // Generate object file only
    int optimize;           // Optimization level
    int debug;              // Debug information
    int verbose;            // Verbose output
    char include_paths[16][256];
    char library_paths[16][256];
    char libraries[16][64];
    int include_count;
    int library_path_count;
    int library_count;
} CompilationRequest;

// Compilation result
typedef struct {
    int success;
    int exit_code;
    char output_file[512];
    char error_message[1024];
    char compiler_output[4096];
    size_t output_size;
} CompilationResult;

// Real compiler functions
int embedded_compiler_init(void);
void embedded_compiler_cleanup(void);

// C compilation using GCC
CompilationResult* compile_c_real(CompilationRequest* request);

// Assembly compilation using NASM
CompilationResult* compile_asm_real(CompilationRequest* request);

// Fortran compilation using GFortran
CompilationResult* compile_fortran_real(CompilationRequest* request);

// Utility functions
int download_embedded_compilers(void);
int extract_embedded_compilers(void);
int verify_compiler_installation(void);
char* create_temp_source_file(const char* source_code, const char* extension);
int cleanup_temp_files(void);

// Embedded compiler paths and management
char* get_gcc_executable_path(void);
char* get_nasm_executable_path(void);
char* get_fortran_executable_path(void);
int setup_compiler_environment(void);

// Real compilation execution
int execute_gcc_compilation(CompilationRequest* request, CompilationResult* result);
int execute_nasm_compilation(CompilationRequest* request, CompilationResult* result);
int execute_fortran_compilation(CompilationRequest* request, CompilationResult* result);

// Compiler output parsing
void parse_compiler_errors(const char* compiler_output, CompilationResult* result);
void format_compilation_output(CompilationResult* result);

#endif // UNIX_EMBEDDED_COMPILER_H
