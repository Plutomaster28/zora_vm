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
    printf("ZoraVM SED v1.0 - Stream Editor\n");
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
        input_data = strdup("Hello World\nThis is a test\nSed is powerful\n");
        input_size = strlen((char*)input_data);
    }
    
    char* output_buffer = malloc(input_size * 2);  // Allow for expansion
    if (!output_buffer) {
        printf("sed: memory allocation failed\n");
        free(input_data);
        return 1;
    }
    
    // Parse simple sed commands
    if (strncmp(script, "s/", 2) == 0) {
        // Substitution command: s/pattern/replacement/flags
        char pattern[256], replacement[256];
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
        } else {
            strcpy(replacement, p);
        }
        
        printf("Substituting '%s' with '%s'\n", pattern, replacement);
        
        unix_sed_substitute(pattern, replacement, (char*)input_data, 
                           output_buffer, input_size * 2);
    } else {
        // For other commands, just copy input
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

int unix_awk(const char* script, const char* input_file) {
    printf("ZoraVM AWK v1.0 - Pattern Scanning and Processing\n");
    printf("Script: %s\n", script);
    
    void* input_data = NULL;
    size_t input_size = 0;
    
    if (input_file) {
        if (vfs_read_file(input_file, &input_data, &input_size) != 0) {
            printf("awk: %s: No such file or directory\n", input_file);
            return 1;
        }
    } else {
        // Sample input for demo
        input_data = strdup("apple 5 red\nbanana 3 yellow\ncherry 8 red\norange 6 orange\n");
        input_size = strlen((char*)input_data);
    }
    
    char* lines = (char*)input_data;
    char* line = strtok(lines, "\n");
    int line_number = 1;
    
    while (line != NULL) {
        // Simple AWK script processing
        if (strcmp(script, "{ print }") == 0) {
            printf("%s\n", line);
        } else if (strcmp(script, "{ print NR, $0 }") == 0) {
            printf("%d %s\n", line_number, line);
        } else if (strcmp(script, "{ print $1 }") == 0) {
            // Print first field
            char* field = strtok(line, " \t");
            if (field) printf("%s\n", field);
        } else if (strcmp(script, "{ print $2 }") == 0) {
            // Print second field
            char* field1 = strtok(line, " \t");
            char* field2 = strtok(NULL, " \t");
            if (field2) printf("%s\n", field2);
        } else if (strstr(script, "red") != NULL) {
            // Pattern matching example
            if (strstr(line, "red") != NULL) {
                printf("%s\n", line);
            }
        } else {
            // Default: print the line
            printf("%s\n", line);
        }
        
        line = strtok(NULL, "\n");
        line_number++;
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

void unix_textproc_cleanup(void) {
    textproc_initialized = 0;
}
