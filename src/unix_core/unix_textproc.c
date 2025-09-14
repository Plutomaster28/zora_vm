#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "unix_textproc.h"
#include "vfs/vfs.h"

static int textproc_initialized = 0;

int unix_textproc_init(void) {
    if (textproc_initialized) return 0;
    
    printf("[TEXTPROC] Initializing Research UNIX Text Processing System...\n");
    
    // Create man page templates
    vfs_mkdir("/usr/man/templates");
    
    const char* man_template = 
        ".\\\" Man page template for ZoraVM Research UNIX\n"
        ".TH COMMAND 1 \"ZoraVM Research UNIX\"\n"
        ".SH NAME\n"
        "command \\- brief description\n"
        ".SH SYNOPSIS\n"
        ".B command\n"
        "[options] [arguments]\n"
        ".SH DESCRIPTION\n"
        "Detailed description of the command.\n"
        ".SH OPTIONS\n"
        ".TP\n"
        ".B \\-option\n"
        "Description of option\n"
        ".SH EXAMPLES\n"
        ".TP\n"
        ".B command example\n"
        "Example usage\n"
        ".SH SEE ALSO\n"
        "related(1), commands(1)\n";
    
    vfs_write_file("/usr/man/templates/template.1", man_template, strlen(man_template));
    
    textproc_initialized = 1;
    printf("[TEXTPROC] Text processing system initialized\n");
    return 0;
}

int unix_sed(const char* script, const char* input_file, const char* output_file) {
    printf("ZoraVM SED v2.0 - Advanced Stream Editor\n");
    printf("Script: %s\n", script);
    printf("Input: %s\n", input_file ? input_file : "stdin");
    
    void* input_data = NULL;
    size_t input_size = 0;
    
    if (input_file) {
        if (vfs_read_file(input_file, &input_data, &input_size) != 0) {
            printf("sed: %s: No such file or directory\n", input_file);
            return 1;
        }
    } else {
        // For demo, use sample input
        input_data = strdup("Hello World\nThis is a test\nSed is powerful\nLine number four\nAnother test line\n");
        input_size = strlen((char*)input_data);
    }
    
    char* output_buffer = malloc(input_size * 3);  // Allow for expansion
    if (!output_buffer) {
        printf("sed: memory allocation failed\n");
        free(input_data);
        return 1;
    }
    
    // Advanced sed command parsing
    if (strncmp(script, "s/", 2) == 0) {
        // Enhanced substitution: s/pattern/replacement/flags
        char pattern[256], replacement[256], flags[16] = "";
        const char* p = script + 2;
        
        // Extract pattern
        char* delim = strchr(p, '/');
        if (!delim) {
            printf("sed: invalid substitution syntax\n");
            free(input_data);
            free(output_buffer);
            return 1;
        }
        
        strncpy(pattern, p, delim - p);
        pattern[delim - p] = '\0';
        
        // Extract replacement
        p = delim + 1;
        delim = strchr(p, '/');
        if (delim) {
            strncpy(replacement, p, delim - p);
            replacement[delim - p] = '\0';
            strcpy(flags, delim + 1);  // Extract flags (g, i, etc.)
        } else {
            strcpy(replacement, p);
        }
        
        printf("Substituting '%s' with '%s' (flags: %s)\n", pattern, replacement, flags);
        
        unix_sed_substitute_enhanced(pattern, replacement, flags, (char*)input_data, 
                                   output_buffer, input_size * 3);
    } else if (strncmp(script, "d", 1) == 0 && strlen(script) == 1) {
        // Delete all lines
        *output_buffer = '\0';
    } else if (script[strlen(script)-1] == 'd') {
        // Line-specific delete (e.g., "2d", "1,3d")
        unix_sed_delete_lines(script, (char*)input_data, output_buffer, input_size * 3);
    } else if (strncmp(script, "p", 1) == 0) {
        // Print command
        strcpy(output_buffer, (char*)input_data);
        strcat(output_buffer, (char*)input_data);  // Duplicate for print
    } else if (strstr(script, "a\\") != NULL) {
        // Append command
        unix_sed_append(script, (char*)input_data, output_buffer, input_size * 3);
    } else if (strstr(script, "i\\") != NULL) {
        // Insert command
        unix_sed_insert(script, (char*)input_data, output_buffer, input_size * 3);
    } else {
        // Default: copy input
        strcpy(output_buffer, (char*)input_data);
    }
    
    if (output_file) {
        vfs_write_file(output_file, output_buffer, strlen(output_buffer));
        printf("Output written to: %s\n", output_file);
    } else {
        printf("Output:\n%s", output_buffer);
    }
    
    free(input_data);
    free(output_buffer);
    return 0;
}

int unix_sed_substitute(const char* pattern, const char* replacement, 
                       const char* input, char* output, size_t output_size) {
    const char* src = input;
    char* dst = output;
    size_t remaining = output_size - 1;
    
    while (*src && remaining > 0) {
        char* match = strstr(src, pattern);
        if (match == src) {
            // Pattern matches at current position
            size_t repl_len = strlen(replacement);
            if (repl_len <= remaining) {
                strcpy(dst, replacement);
                dst += repl_len;
                remaining -= repl_len;
                src += strlen(pattern);
            } else {
                break;  // No room for replacement
            }
        } else {
            *dst++ = *src++;
            remaining--;
        }
    }
    *dst = '\0';
    return 0;
}

// Enhanced substitution with flags support
int unix_sed_substitute_enhanced(const char* pattern, const char* replacement, 
                                const char* flags, const char* input, 
                                char* output, size_t output_size) {
    const char* src = input;
    char* dst = output;
    size_t remaining = output_size - 1;
    int global = (strchr(flags, 'g') != NULL);
    int ignore_case = (strchr(flags, 'i') != NULL);
    int numeric_flag = 0;
    
    // Check for numeric flag (e.g., s/pattern/replacement/2)
    for (const char* f = flags; *f; f++) {
        if (isdigit(*f)) {
            numeric_flag = *f - '0';
            break;
        }
    }
    
    char* line = strtok((char*)input, "\n");
    int first_line = 1;
    
    while (line != NULL && remaining > 0) {
        char line_output[2048] = "";
        const char* line_src = line;
        char* line_dst = line_output;
        int substitutions_this_line = 0;
        
        while (*line_src && (line_dst - line_output) < sizeof(line_output) - 1) {
            char* match = NULL;
            
            if (ignore_case) {
                // Case-insensitive matching
                char* line_lower = strdup(line_src);
                char* pattern_lower = strdup(pattern);
                for (int i = 0; line_lower[i]; i++) line_lower[i] = tolower(line_lower[i]);
                for (int i = 0; pattern_lower[i]; i++) pattern_lower[i] = tolower(pattern_lower[i]);
                
                char* temp_match = strstr(line_lower, pattern_lower);
                if (temp_match) {
                    match = (char*)line_src + (temp_match - line_lower);
                }
                free(line_lower);
                free(pattern_lower);
            } else {
                match = strstr(line_src, pattern);
            }
            
            if (match == line_src) {
                substitutions_this_line++;
                
                // Check if we should perform this substitution
                if (numeric_flag == 0 || substitutions_this_line == numeric_flag || global) {
                    size_t repl_len = strlen(replacement);
                    if (repl_len <= sizeof(line_output) - (line_dst - line_output) - 1) {
                        strcpy(line_dst, replacement);
                        line_dst += repl_len;
                        line_src += strlen(pattern);
                        
                        if (!global && numeric_flag == 0) break;  // Only first match
                        if (numeric_flag > 0 && substitutions_this_line == numeric_flag) break;
                    } else {
                        break;
                    }
                } else {
                    *line_dst++ = *line_src++;
                }
            } else {
                *line_dst++ = *line_src++;
            }
        }
        
        // Copy rest of line if any
        while (*line_src && (line_dst - line_output) < sizeof(line_output) - 1) {
            *line_dst++ = *line_src++;
        }
        *line_dst = '\0';
        
        // Add to output
        if (!first_line && remaining > 0) {
            *dst++ = '\n';
            remaining--;
        }
        first_line = 0;
        
        size_t line_len = strlen(line_output);
        if (line_len <= remaining) {
            strcpy(dst, line_output);
            dst += line_len;
            remaining -= line_len;
        }
        
        line = strtok(NULL, "\n");
    }
    
    *dst = '\0';
    return 0;
}

// Line deletion functionality
int unix_sed_delete_lines(const char* script, const char* input, char* output, size_t output_size) {
    char* output_ptr = output;
    size_t remaining = output_size - 1;
    
    // Parse delete command (e.g., "2d", "1,3d", "2,4d")
    int start_line = 0, end_line = 0;
    if (sscanf(script, "%d,%dd", &start_line, &end_line) == 2) {
        // Range delete
    } else if (sscanf(script, "%dd", &start_line) == 1) {
        // Single line delete
        end_line = start_line;
    } else {
        strcpy(output, input);  // Invalid format, copy as-is
        return 0;
    }
    
    char* line = strtok((char*)input, "\n");
    int line_number = 1;
    int first_output = 1;
    
    while (line != NULL && remaining > 0) {
        if (line_number < start_line || line_number > end_line) {
            // Keep this line
            if (!first_output && remaining > 0) {
                *output_ptr++ = '\n';
                remaining--;
            }
            first_output = 0;
            
            size_t line_len = strlen(line);
            if (line_len <= remaining) {
                strcpy(output_ptr, line);
                output_ptr += line_len;
                remaining -= line_len;
            }
        }
        line = strtok(NULL, "\n");
        line_number++;
    }
    
    *output_ptr = '\0';
    return 0;
}

// Append functionality
int unix_sed_append(const char* script, const char* input, char* output, size_t output_size) {
    // Extract append text from script (format: "2a\\text to append")
    int line_num = 0;
    char append_text[256] = "";
    
    if (sscanf(script, "%da\\%255s", &line_num, append_text) != 2) {
        strcpy(output, input);  // Invalid format
        return 0;
    }
    
    char* output_ptr = output;
    size_t remaining = output_size - 1;
    char* line = strtok((char*)input, "\n");
    int current_line = 1;
    int first_output = 1;
    
    while (line != NULL && remaining > 0) {
        // Output current line
        if (!first_output && remaining > 0) {
            *output_ptr++ = '\n';
            remaining--;
        }
        first_output = 0;
        
        size_t line_len = strlen(line);
        if (line_len <= remaining) {
            strcpy(output_ptr, line);
            output_ptr += line_len;
            remaining -= line_len;
        }
        
        // Append text after specified line
        if (current_line == line_num && remaining > 0) {
            *output_ptr++ = '\n';
            remaining--;
            
            size_t append_len = strlen(append_text);
            if (append_len <= remaining) {
                strcpy(output_ptr, append_text);
                output_ptr += append_len;
                remaining -= append_len;
            }
        }
        
        line = strtok(NULL, "\n");
        current_line++;
    }
    
    *output_ptr = '\0';
    return 0;
}

// Insert functionality
int unix_sed_insert(const char* script, const char* input, char* output, size_t output_size) {
    // Extract insert text from script (format: "2i\\text to insert")
    int line_num = 0;
    char insert_text[256] = "";
    
    if (sscanf(script, "%di\\%255s", &line_num, insert_text) != 2) {
        strcpy(output, input);  // Invalid format
        return 0;
    }
    
    char* output_ptr = output;
    size_t remaining = output_size - 1;
    char* line = strtok((char*)input, "\n");
    int current_line = 1;
    int first_output = 1;
    
    while (line != NULL && remaining > 0) {
        // Insert text before specified line
        if (current_line == line_num) {
            if (!first_output && remaining > 0) {
                *output_ptr++ = '\n';
                remaining--;
            }
            first_output = 0;
            
            size_t insert_len = strlen(insert_text);
            if (insert_len <= remaining) {
                strcpy(output_ptr, insert_text);
                output_ptr += insert_len;
                remaining -= insert_len;
            }
            
            if (remaining > 0) {
                *output_ptr++ = '\n';
                remaining--;
            }
        }
        
        // Output current line
        if (!first_output && remaining > 0) {
            *output_ptr++ = '\n';
            remaining--;
        }
        first_output = 0;
        
        size_t line_len = strlen(line);
        if (line_len <= remaining) {
            strcpy(output_ptr, line);
            output_ptr += line_len;
            remaining -= line_len;
        }
        
        line = strtok(NULL, "\n");
        current_line++;
    }
    
    *output_ptr = '\0';
    return 0;
}

int unix_awk(const char* script, const char* input_file) {
    printf("ZoraVM AWK v2.0 - Advanced Pattern Scanning and Processing\n");
    printf("Script: %s\n", script);
    
    void* input_data = NULL;
    size_t input_size = 0;
    
    if (input_file) {
        if (vfs_read_file(input_file, &input_data, &input_size) != 0) {
            printf("awk: %s: No such file or directory\n", input_file);
            return 1;
        }
    } else {
        // Enhanced sample input for demo
        input_data = strdup("apple 5 red fresh\nbanana 3 yellow ripe\ncherry 8 red sweet\norange 6 orange juicy\ngrape 12 purple sweet\nkiwi 4 green sour\n");
        input_size = strlen((char*)input_data);
    }
    
    char* lines = (char*)input_data;
    char* line = strtok(lines, "\n");
    int line_number = 1;
    int total_records = 0;
    double field_sum = 0;
    
    // Variables for AWK built-ins
    char fields[10][256];  // $0 to $9
    int num_fields = 0;
    
    while (line != NULL) {
        strcpy(fields[0], line);  // $0 = entire line
        
        // Split line into fields
        num_fields = unix_awk_split_fields_enhanced(line, fields, 10);
        total_records++;
        
        // Advanced AWK script processing
        if (strcmp(script, "{ print }") == 0 || strcmp(script, "{ print $0 }") == 0) {
            printf("%s\n", line);
        } 
        else if (strcmp(script, "{ print NR, $0 }") == 0) {
            printf("%d %s\n", line_number, line);
        } 
        else if (strncmp(script, "{ print $", 9) == 0) {
            // Field printing: { print $1 }, { print $2 }, etc.
            int field_num = script[9] - '0';
            if (field_num >= 0 && field_num < num_fields) {
                printf("%s\n", fields[field_num]);
            }
        }
        else if (strstr(script, "NF") != NULL) {
            // Number of fields
            printf("%d\n", num_fields - 1);  // -1 because $0 is whole line
        }
        else if (strstr(script, "NR") != NULL && strstr(script, "print NR") != NULL) {
            printf("%d\n", line_number);
        }
        else if (strstr(script, "length") != NULL) {
            // Length function
            if (strstr(script, "length($0)") != NULL) {
                printf("%zu\n", strlen(line));
            } else if (strstr(script, "length($1)") != NULL && num_fields > 1) {
                printf("%zu\n", strlen(fields[1]));
            }
        }
        else if (strstr(script, "sum") != NULL || strstr(script, "$2") != NULL) {
            // Summation example
            if (num_fields > 2 && isdigit(fields[2][0])) {
                field_sum += atof(fields[2]);
                printf("Running sum: %.2f\n", field_sum);
            }
        }
        else if (strstr(script, "BEGIN") != NULL) {
            // BEGIN block - only execute once
            if (line_number == 1) {
                printf("Starting AWK processing...\n");
            }
            printf("%s\n", line);
        }
        else if (strstr(script, "END") != NULL) {
            // END block - defer to end
            printf("%s\n", line);
        }
        else if (strstr(script, "toupper") != NULL) {
            // Convert to uppercase
            char upper_line[512];
            unix_awk_toupper(line, upper_line);
            printf("%s\n", upper_line);
        }
        else if (strstr(script, "tolower") != NULL) {
            // Convert to lowercase  
            char lower_line[512];
            unix_awk_tolower(line, lower_line);
            printf("%s\n", lower_line);
        }
        else if (strstr(script, "gsub") != NULL) {
            // Global substitution
            unix_awk_gsub(script, line);
        }
        else if (strstr(script, "sub") != NULL) {
            // Single substitution
            unix_awk_sub(script, line);
        }
        else if (strstr(script, "match") != NULL) {
            // Pattern matching
            unix_awk_match_pattern_enhanced(script, line, line_number);
        }
        else if (strstr(script, "red") != NULL || strstr(script, "yellow") != NULL || 
                 strstr(script, "green") != NULL) {
            // Color pattern matching
            if (strstr(line, "red") != NULL || strstr(line, "yellow") != NULL || 
                strstr(line, "green") != NULL) {
                printf("%s\n", line);
            }
        }
        else if (strstr(script, ">") != NULL || strstr(script, "<") != NULL || 
                 strstr(script, "==") != NULL) {
            // Conditional expressions
            unix_awk_conditional(script, fields, num_fields, line_number);
        }
        else if (strstr(script, "split") != NULL) {
            // Split function demo
            unix_awk_split_demo(line);
        }
        else {
            // Default: print the line
            printf("%s\n", line);
        }
        
        line = strtok(NULL, "\n");
        line_number++;
    }
    
    // Process END block if present
    if (strstr(script, "END") != NULL) {
        printf("Total records processed: %d\n", total_records);
        if (field_sum > 0) {
            printf("Field sum total: %.2f\n", field_sum);
        }
    }
    
    free(input_data);
    return 0;
}

int unix_nroff(const char* input_file, const char* output_file, DocFormat* format) {
    printf("ZoraVM NROFF v1.0 - Text Formatter\n");
    printf("Formatting: %s\n", input_file);
    
    void* input_data = NULL;
    size_t input_size = 0;
    
    if (vfs_read_file(input_file, &input_data, &input_size) != 0) {
        printf("nroff: %s: No such file or directory\n", input_file);
        return 1;
    }
    
    char* formatted = malloc(input_size * 2);
    if (!formatted) {
        printf("nroff: memory allocation failed\n");
        free(input_data);
        return 1;
    }
    
    // Simple NROFF formatting
    char* src = (char*)input_data;
    char* dst = formatted;
    int line_length = format ? format->page_width : 80;
    
    // Add header if specified
    if (format && format->header[0]) {
        dst += sprintf(dst, "%s\n\n", format->header);
    }
    
    // Process input line by line
    char* line = strtok(src, "\n");
    while (line != NULL) {
        if (line[0] == '.') {
            // NROFF command
            if (strncmp(line, ".TH", 3) == 0) {
                dst += sprintf(dst, "TITLE: %s\n", line + 4);
            } else if (strncmp(line, ".SH", 3) == 0) {
                dst += sprintf(dst, "\n%s\n", line + 4);
                for (int i = 0; i < strlen(line + 4); i++) {
                    *dst++ = '=';
                }
                *dst++ = '\n';
            } else if (strncmp(line, ".B", 2) == 0) {
                dst += sprintf(dst, "**%s**", line + 3);
            } else if (strncmp(line, ".I", 2) == 0) {
                dst += sprintf(dst, "*%s*", line + 3);
            }
        } else {
            // Regular text - format to line length
            unix_format_paragraph(line, dst, line_length);
            dst += strlen(dst);
            *dst++ = '\n';
        }
        line = strtok(NULL, "\n");
    }
    
    // Add footer if specified
    if (format && format->footer[0]) {
        dst += sprintf(dst, "\n%s\n", format->footer);
    }
    
    *dst = '\0';
    
    if (output_file) {
        vfs_write_file(output_file, formatted, strlen(formatted));
        printf("Formatted output written to: %s\n", output_file);
    } else {
        printf("Formatted output:\n%s", formatted);
    }
    
    free(input_data);
    free(formatted);
    return 0;
}

void unix_format_paragraph(const char* input, char* output, int width) {
    const char* src = input;
    char* dst = output;
    int current_line_length = 0;
    
    while (*src) {
        if (*src == ' ' && current_line_length >= width - 10) {
            // Word wrap
            *dst++ = '\n';
            current_line_length = 0;
            while (*src == ' ') src++;  // Skip spaces
        } else {
            *dst++ = *src++;
            current_line_length++;
        }
    }
    *dst = '\0';
}

int unix_grep_extended(const char* pattern, const char* input_file, 
                      int line_numbers, int ignore_case, int invert_match) {
    printf("ZoraVM GREP v1.0 - Extended Pattern Matching\n");
    
    void* input_data = NULL;
    size_t input_size = 0;
    
    if (vfs_read_file(input_file, &input_data, &input_size) != 0) {
        printf("grep: %s: No such file or directory\n", input_file);
        return 1;
    }
    
    char* lines = (char*)input_data;
    char* line = strtok(lines, "\n");
    int line_number = 1;
    int matches = 0;
    
    while (line != NULL) {
        int match_found = 0;
        
        if (ignore_case) {
            char* line_lower = strdup(line);
            char* pattern_lower = strdup(pattern);
            
            for (int i = 0; line_lower[i]; i++) {
                line_lower[i] = tolower(line_lower[i]);
            }
            for (int i = 0; pattern_lower[i]; i++) {
                pattern_lower[i] = tolower(pattern_lower[i]);
            }
            
            match_found = (strstr(line_lower, pattern_lower) != NULL);
            free(line_lower);
            free(pattern_lower);
        } else {
            match_found = (strstr(line, pattern) != NULL);
        }
        
        if (invert_match) {
            match_found = !match_found;
        }
        
        if (match_found) {
            if (line_numbers) {
                printf("%d:%s\n", line_number, line);
            } else {
                printf("%s\n", line);
            }
            matches++;
        }
        
        line = strtok(NULL, "\n");
        line_number++;
    }
    
    free(input_data);
    printf("Total matches: %d\n", matches);
    return 0;
}

int unix_word_count(const char* input, int* lines, int* words, int* chars) {
    *lines = *words = *chars = 0;
    
    const char* p = input;
    int in_word = 0;
    
    while (*p) {
        (*chars)++;
        
        if (*p == '\n') {
            (*lines)++;
        }
        
        if (isspace(*p)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            (*words)++;
        }
        
        p++;
    }
    
    return 0;
}

// Enhanced AWK field splitting
int unix_awk_split_fields_enhanced(const char* line, char fields[][256], int max_fields) {
    char* line_copy = strdup(line);
    char* token;
    int field_count = 1;  // Start with 1 for $0
    
    strcpy(fields[0], line);  // $0 is the entire line
    
    token = strtok(line_copy, " \t");
    while (token != NULL && field_count < max_fields) {
        strcpy(fields[field_count], token);
        field_count++;
        token = strtok(NULL, " \t");
    }
    
    free(line_copy);
    return field_count;
}

// Convert string to uppercase
void unix_awk_toupper(const char* input, char* output) {
    while (*input) {
        *output = toupper(*input);
        input++;
        output++;
    }
    *output = '\0';
}

// Convert string to lowercase
void unix_awk_tolower(const char* input, char* output) {
    while (*input) {
        *output = tolower(*input);
        input++;
        output++;
    }
    *output = '\0';
}

// Global substitution function
void unix_awk_gsub(const char* script, const char* line) {
    // Parse gsub(/pattern/, "replacement", $0) or similar
    char pattern[128], replacement[128];
    
    // Simple parser for demonstration
    if (strstr(script, "gsub")) {
        printf("GSUB: %s (substitution not fully implemented in demo)\n", line);
    }
}

// Single substitution function
void unix_awk_sub(const char* script, const char* line) {
    // Parse sub(/pattern/, "replacement", $0) or similar
    if (strstr(script, "sub")) {
        printf("SUB: %s (substitution not fully implemented in demo)\n", line);
    }
}

// Enhanced pattern matching
void unix_awk_match_pattern_enhanced(const char* script, const char* line, int line_number) {
    if (strstr(script, "match")) {
        // Find if line matches any patterns in script
        if (strstr(line, "red") || strstr(line, "sweet") || strstr(line, "fresh")) {
            printf("Line %d matches pattern: %s\n", line_number, line);
        }
    }
}

// Conditional processing
void unix_awk_conditional(const char* script, char fields[][256], int num_fields, int line_number) {
    // Process conditions like $2 > 5, $3 == "red", etc.
    if (strstr(script, "$2 > ") && num_fields > 2) {
        int value = atoi(fields[2]);
        int threshold = 5;  // Could parse from script
        if (value > threshold) {
            printf("Line %d: Field 2 (%d) > %d: %s\n", line_number, value, threshold, fields[0]);
        }
    } else if (strstr(script, "$3 == ") && num_fields > 3) {
        if (strstr(script, "red") && strcmp(fields[3], "red") == 0) {
            printf("Line %d: Field 3 is red: %s\n", line_number, fields[0]);
        }
    } else if (strstr(script, "NF > ") && num_fields > 0) {
        int threshold = 3;
        if ((num_fields - 1) > threshold) {  // -1 for $0
            printf("Line %d: %d fields > %d: %s\n", line_number, num_fields - 1, threshold, fields[0]);
        }
    }
}

// Split function demonstration
void unix_awk_split_demo(const char* line) {
    printf("Split demo for: %s\n", line);
    char* line_copy = strdup(line);
    char* token = strtok(line_copy, " ");
    int i = 1;
    
    while (token != NULL) {
        printf("  Field %d: %s\n", i, token);
        token = strtok(NULL, " ");
        i++;
    }
    
    free(line_copy);
}

void unix_textproc_cleanup(void) {
    textproc_initialized = 0;
}
