#ifndef UNIX_TEXTPROC_H
#define UNIX_TEXTPROC_H

// Research UNIX Text Processing System
// Advanced sed, awk, troff, nroff implementations

#include <stddef.h>

// Text processing engine types
typedef enum {
    TEXTPROC_SED,
    TEXTPROC_AWK,
    TEXTPROC_TROFF,
    TEXTPROC_NROFF,
    TEXTPROC_GREP
} TextProcType;

// Sed command structure
typedef struct {
    char pattern[256];
    char replacement[256];
    char flags[16];
    int line_number;
    int global;
} SedCommand;

// AWK script structure
typedef struct {
    char pattern[256];
    char action[512];
    int field_separator;
    char variables[16][64];
    int var_count;
} AwkScript;

// Document formatting structure
typedef struct {
    int page_length;
    int page_width;
    int left_margin;
    int right_margin;
    int line_spacing;
    char font[32];
    char title[256];
    char header[256];
    char footer[256];
} DocFormat;

// Function prototypes
int unix_textproc_init(void);
void unix_textproc_cleanup(void);

// Sed implementation
int unix_sed(const char* script, const char* input_file, const char* output_file);
int unix_sed_substitute(const char* pattern, const char* replacement, 
                       const char* input, char* output, size_t output_size);
int unix_sed_substitute_enhanced(const char* pattern, const char* replacement, 
                                const char* flags, const char* input, 
                                char* output, size_t output_size);
int unix_sed_delete_lines(const char* script, const char* input, char* output, size_t output_size);
int unix_sed_append(const char* script, const char* input, char* output, size_t output_size);
int unix_sed_insert(const char* script, const char* input, char* output, size_t output_size);
int unix_sed_delete(int line_number, const char* input, char* output, size_t output_size);
int unix_sed_print(const char* pattern, const char* input);

// AWK implementation
int unix_awk(const char* script, const char* input_file);
int unix_awk_process_line(AwkScript* script, const char* line, int line_number);
int unix_awk_split_fields(const char* line, char fields[][256], int max_fields);
int unix_awk_split_fields_enhanced(const char* line, char fields[][256], int max_fields);
int unix_awk_match_pattern(const char* pattern, const char* text);
void unix_awk_match_pattern_enhanced(const char* script, const char* line, int line_number);
void unix_awk_toupper(const char* input, char* output);
void unix_awk_tolower(const char* input, char* output);
void unix_awk_gsub(const char* script, const char* line);
void unix_awk_sub(const char* script, const char* line);
void unix_awk_conditional(const char* script, char fields[][256], int num_fields, int line_number);
void unix_awk_split_demo(const char* line);

// TROFF/NROFF implementation
int unix_troff(const char* input_file, const char* output_file, DocFormat* format);
int unix_nroff(const char* input_file, const char* output_file, DocFormat* format);
int unix_format_man_page(const char* input_file, const char* output_file);
void unix_format_paragraph(const char* input, char* output, int width);

// Enhanced GREP
int unix_grep_extended(const char* pattern, const char* input_file, 
                      int line_numbers, int ignore_case, int invert_match);

// Text analysis utilities
int unix_word_count(const char* input, int* lines, int* words, int* chars);
int unix_sort_lines(const char* input, char* output, size_t output_size, int reverse);
int unix_unique_lines(const char* input, char* output, size_t output_size, int count_mode);

#endif // UNIX_TEXTPROC_H
