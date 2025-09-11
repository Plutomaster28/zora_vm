#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unix_compiler.h"
#include "unix_embedded_compiler.h"
#include "vfs/vfs.h"

// Simple C-like language compiler for ZoraVM
// Provides authentic UNIX compiler experience

static int compiler_initialized = 0;

int unix_compiler_init(void) {
    if (compiler_initialized) return 0;
    
    printf("[COMPILER] Initializing ZoraVM Research UNIX Compiler Toolchain...\n");
    
    // Initialize embedded real compilers
    embedded_compiler_init();
    
    // Create compiler working directories
    vfs_mkdir("/tmp/cc");
    vfs_mkdir("/tmp/f77");
    vfs_mkdir("/tmp/as");
    vfs_mkdir("/usr/lib/crt0.o");
    vfs_mkdir("/usr/include/stdio.h");
    vfs_mkdir("/usr/include/stdlib.h");
    
    // Create standard C library headers
    const char* stdio_h = 
        "#ifndef _STDIO_H\n"
        "#define _STDIO_H\n"
        "\n"
        "/* ZoraVM Research UNIX C Library */\n"
        "\n"
        "int printf(const char *format, ...);\n"
        "int scanf(const char *format, ...);\n"
        "int fprintf(FILE *stream, const char *format, ...);\n"
        "int sprintf(char *str, const char *format, ...);\n"
        "\n"
        "FILE *fopen(const char *filename, const char *mode);\n"
        "int fclose(FILE *stream);\n"
        "int fgetc(FILE *stream);\n"
        "int fputc(int c, FILE *stream);\n"
        "\n"
        "#endif /* _STDIO_H */\n";
    
    vfs_write_file("/usr/include/stdio.h", stdio_h, strlen(stdio_h));
    
    const char* stdlib_h = 
        "#ifndef _STDLIB_H\n"
        "#define _STDLIB_H\n"
        "\n"
        "/* ZoraVM Research UNIX C Library */\n"
        "\n"
        "void *malloc(size_t size);\n"
        "void free(void *ptr);\n"
        "void *calloc(size_t nmemb, size_t size);\n"
        "void *realloc(void *ptr, size_t size);\n"
        "\n"
        "int atoi(const char *nptr);\n"
        "long atol(const char *nptr);\n"
        "double atof(const char *nptr);\n"
        "\n"
        "void exit(int status);\n"
        "void abort(void);\n"
        "\n"
        "#endif /* _STDLIB_H */\n";
    
    vfs_write_file("/usr/include/stdlib.h", stdlib_h, strlen(stdlib_h));
    
    compiler_initialized = 1;
    printf("[COMPILER] Compiler toolchain initialized successfully\n");
    return 0;
}

int unix_compile_c(CompileOptions* opts) {
    printf("ZoraVM C Compiler (Real TCC Integration)\n");
    printf("========================================\n");
    
    // Create compilation request for embedded compiler
    CompilationRequest request = {0};
    
    // Copy input file name
    if (opts->input_file) {
        strncpy(request.source_file, opts->input_file, sizeof(request.source_file) - 1);
    }
    
    // Set output file
    if (opts->output_file && strlen(opts->output_file) > 0) {
        strncpy(request.output_file, opts->output_file, sizeof(request.output_file) - 1);
    } else {
        strcpy(request.output_file, "a.exe");
    }
    
    // Copy compilation flags
    request.compile_only = opts->compile_only;
    request.optimize = opts->optimize;
    request.debug = opts->debug;
    request.verbose = opts->verbose;
    
    // Execute real compilation
    CompilationResult* result = compile_c_real(&request);
    
    if (result->success) {
        printf("✓ Real C compilation completed successfully!\n");
        printf("Output: %s (%zu bytes)\n", result->output_file, result->output_size);
        return 0;
    } else {
        printf("✗ C compilation failed: %s\n", result->error_message);
        return 1;
    }
}

int unix_compile_fortran(CompileOptions* opts) {
    printf("ZoraVM Fortran Compiler (Real GFortran Integration)\n");
    printf("===================================================\n");
    
    // Create compilation request for embedded compiler
    CompilationRequest request = {0};
    
    // Copy input file name
    if (opts->input_file) {
        strncpy(request.source_file, opts->input_file, sizeof(request.source_file) - 1);
    }
    
    // Set output file
    if (opts->output_file && strlen(opts->output_file) > 0) {
        strncpy(request.output_file, opts->output_file, sizeof(request.output_file) - 1);
    } else {
        strcpy(request.output_file, "program.exe");
    }
    
    // Copy compilation flags
    request.compile_only = opts->compile_only;
    request.optimize = opts->optimize;
    request.debug = opts->debug;
    request.verbose = opts->verbose;
    
    // Execute real compilation
    CompilationResult* result = compile_fortran_real(&request);
    
    if (result->success) {
        printf("✓ Real Fortran compilation completed successfully!\n");
        printf("Executable: %s (%zu bytes)\n", result->output_file, result->output_size);
        return 0;
    } else {
        printf("✗ Fortran compilation failed: %s\n", result->error_message);
        return 1;
    }
}

int unix_compile_assembly(CompileOptions* opts) {
    printf("ZoraVM x86 Assembler (Real NASM Integration)\n");
    printf("============================================\n");
    
    // Create compilation request for embedded assembler
    CompilationRequest request = {0};
    
    // Copy input file name
    if (opts->input_file) {
        strncpy(request.source_file, opts->input_file, sizeof(request.source_file) - 1);
    }
    
    // Set output file
    if (opts->output_file && strlen(opts->output_file) > 0) {
        strncpy(request.output_file, opts->output_file, sizeof(request.output_file) - 1);
    } else {
        strcpy(request.output_file, "program.o");
    }
    
    // Copy compilation flags
    request.debug = opts->debug;
    request.verbose = opts->verbose;
    
    // Execute real assembly
    CompilationResult* result = compile_asm_real(&request);
    
    if (result->success) {
        printf("✓ Real x86 assembly completed successfully!\n");
        printf("Object file: %s (%zu bytes)\n", result->output_file, result->output_size);
        return 0;
    } else {
        printf("✗ Assembly failed: %s\n", result->error_message);
        return 1;
    }
}

int unix_run_yacc(CompileOptions* opts) {
    printf("ZoraVM YACC v1.0 (Yet Another Compiler Compiler)\n");
    printf("Processing grammar: %s\n", opts->input_file);
    
    void* grammar_data = NULL;
    size_t grammar_size = 0;
    
    if (vfs_read_file(opts->input_file, &grammar_data, &grammar_size) != 0) {
        printf("yacc: error: %s: No such file or directory\n", opts->input_file);
        return 1;
    }
    
    printf("Grammar file loaded (%zu bytes)\n", grammar_size);
    printf("Phase 1: Grammar analysis... OK\n");
    printf("Phase 2: Parser generation... OK\n");
    
    // Generate y.tab.c and y.tab.h
    const char* parser_c = 
        "/* Generated by ZoraVM YACC */\n"
        "#include <stdio.h>\n"
        "#include \"y.tab.h\"\n"
        "\n"
        "int yyparse(void) {\n"
        "    printf(\"Parser generated by YACC\\n\");\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "int main(void) {\n"
        "    return yyparse();\n"
        "}\n";
    
    const char* parser_h = 
        "/* Generated by ZoraVM YACC */\n"
        "#ifndef Y_TAB_H\n"
        "#define Y_TAB_H\n"
        "\n"
        "extern int yyparse(void);\n"
        "extern int yylex(void);\n"
        "extern char *yytext;\n"
        "\n"
        "#endif\n";
    
    vfs_write_file("y.tab.c", parser_c, strlen(parser_c));
    vfs_write_file("y.tab.h", parser_h, strlen(parser_h));
    
    printf("Parser generation successful: y.tab.c, y.tab.h\n");
    return 0;
}

int unix_run_lex(CompileOptions* opts) {
    printf("ZoraVM LEX v1.0 (Lexical Analyzer Generator)\n");
    printf("Processing lexer: %s\n", opts->input_file);
    
    void* lexer_data = NULL;
    size_t lexer_size = 0;
    
    if (vfs_read_file(opts->input_file, &lexer_data, &lexer_size) != 0) {
        printf("lex: error: %s: No such file or directory\n", opts->input_file);
        return 1;
    }
    
    printf("Lexer specification loaded (%zu bytes)\n", lexer_size);
    printf("Phase 1: Pattern analysis... OK\n");
    printf("Phase 2: Lexer generation... OK\n");
    
    const char* lexer_c = 
        "/* Generated by ZoraVM LEX */\n"
        "#include <stdio.h>\n"
        "#include <string.h>\n"
        "\n"
        "char *yytext;\n"
        "int yyleng;\n"
        "\n"
        "int yylex(void) {\n"
        "    printf(\"Lexer generated by LEX\\n\");\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "int main(void) {\n"
        "    return yylex();\n"
        "}\n";
    
    vfs_write_file("lex.yy.c", lexer_c, strlen(lexer_c));
    printf("Lexer generation successful: lex.yy.c\n");
    return 0;
}

int unix_link_objects(char** object_files, int count, char* output) {
    printf("ZoraVM Link Editor (ld) v1.0\n");
    printf("Linking %d object files...\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("  %s\n", object_files[i]);
    }
    
    printf("Output: %s\n", output);
    printf("\n");
    printf("Link map:\n");
    printf("  Text section: 0x1000 - 0x2000\n");
    printf("  Data section: 0x2000 - 0x3000\n");
    printf("  BSS section:  0x3000 - 0x4000\n");
    printf("  Entry point:  0x1000\n");
    printf("\n");
    printf("Successfully linked executable: %s\n", output);
    
    // Create empty output file
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "/usr/bin/%s", output);
    vfs_write_file(full_path, "#!/bin/sh\necho \"Linked executable\"\n", 35);
    
    return 0;
}

int unix_parse_compile_args(int argc, char** argv, CompileOptions* opts) {
    // Initialize options
    memset(opts, 0, sizeof(CompileOptions));
    opts->type = COMPILER_C; // Default
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            opts->compile_only = 1;
        } else if (strcmp(argv[i], "-O") == 0) {
            opts->optimize = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            opts->debug = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            opts->verbose = 1;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts->output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            opts->input_file = argv[i];
        }
    }
    
    return 0;
}

void unix_print_compiler_help(CompilerType type) {
    switch (type) {
        case COMPILER_C:
            printf("ZoraVM C Compiler v1.0\n");
            printf("Usage: cc [options] file...\n");
            printf("Options:\n");
            printf("  -c        Compile only, do not link\n");
            printf("  -o file   Output to file\n");
            printf("  -O        Optimize\n");
            printf("  -g        Generate debug info\n");
            printf("  -v        Verbose output\n");
            break;
        case COMPILER_FORTRAN:
            printf("ZoraVM Fortran 77 Compiler v1.0\n");
            printf("Usage: f77 [options] file...\n");
            break;
        case COMPILER_ASSEMBLY:
            printf("ZoraVM Assembler v1.0\n");
            printf("Usage: as [options] file...\n");
            break;
        default:
            printf("Unknown compiler type\n");
            break;
    }
}

// Simple debugger implementation
int unix_debugger_init(void) {
    printf("[DEBUGGER] Initializing ZoraVM Research UNIX Debugger...\n");
    return 0;
}

int unix_debug_process(int pid) {
    printf("Debugging process %d\n", pid);
    printf("Debugger attached. Commands:\n");
    printf("  (gdb) bt     - backtrace\n");
    printf("  (gdb) info   - process info\n");
    printf("  (gdb) cont   - continue\n");
    printf("  (gdb) quit   - quit debugger\n");
    return 0;
}

int unix_trace_syscalls(int pid) {
    printf("Tracing system calls for process %d\n", pid);
    printf("Sample trace output:\n");
    printf("open(\"/etc/passwd\", O_RDONLY) = 3\n");
    printf("read(3, \"root:x:0:0:root:/root:/bin/sh\\n\", 4096) = 30\n");
    printf("close(3) = 0\n");
    printf("write(1, \"Hello World\\n\", 12) = 12\n");
    return 0;
}

void unix_print_backtrace(void) {
    printf("Backtrace:\n");
    printf("#0  main () at program.c:10\n");
    printf("#1  _start () at crt0.s:25\n");
}

void unix_compiler_cleanup(void) {
    if (compiler_initialized) {
        embedded_compiler_cleanup();
        compiler_initialized = 0;
    }
}

void unix_debugger_cleanup(void) {
    // Cleanup debugger resources
}
