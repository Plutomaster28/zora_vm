#include "editors/zora_editor.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// Create editor state
EditorState* editor_create(int max_lines) {
    EditorState* editor = (EditorState*)calloc(1, sizeof(EditorState));
    if (!editor) return NULL;
    
    editor->max_lines = max_lines > 0 ? max_lines : EDITOR_DEFAULT_MAX_LINES;
    editor->lines = (char**)calloc(editor->max_lines, sizeof(char*));
    if (!editor->lines) {
        free(editor);
        return NULL;
    }
    
    // Create first empty line
    editor->lines[0] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->scroll_offset = 0;
    editor->modified = 0;
    editor->filename[0] = '\0';
    
    return editor;
}

// Destroy editor state
void editor_destroy(EditorState* editor) {
    if (!editor) return;
    
    if (editor->lines) {
        for (int i = 0; i < editor->line_count; i++) {
            free(editor->lines[i]);
        }
        free(editor->lines);
    }
    free(editor);
}

// File operations
int editor_load_file(EditorState* editor, const char* filename) {
    if (!editor || !filename) return -1;
    
    // Try to read from VFS first
    void* data = NULL;
    size_t size = 0;
    
    if (vfs_read_file(filename, &data, &size) == 0 && data) {
        // Clear existing lines
        for (int i = 0; i < editor->line_count; i++) {
            free(editor->lines[i]);
            editor->lines[i] = NULL;
        }
        editor->line_count = 0;
        
        // Parse content into lines
        char* content = (char*)data;
        char* line_start = content;
        char* line_end;
        
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            if (editor->line_count >= editor->max_lines) break;
            
            int line_len = (int)(line_end - line_start);
            if (line_len >= EDITOR_MAX_LINE_LENGTH) line_len = EDITOR_MAX_LINE_LENGTH - 1;
            
            editor->lines[editor->line_count] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
            strncpy(editor->lines[editor->line_count], line_start, line_len);
            editor->lines[editor->line_count][line_len] = '\0';
            editor->line_count++;
            
            line_start = line_end + 1;
        }
        
        // Handle last line (no newline at end)
        if (*line_start && editor->line_count < editor->max_lines) {
            editor->lines[editor->line_count] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
            strncpy(editor->lines[editor->line_count], line_start, EDITOR_MAX_LINE_LENGTH - 1);
            editor->line_count++;
        }
        
        // Ensure at least one line
        if (editor->line_count == 0) {
            editor->lines[0] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
            editor->line_count = 1;
        }
        
        strncpy(editor->filename, filename, sizeof(editor->filename) - 1);
        editor->modified = 0;
        
        free(data);
        return 0;
    }
    
    // File doesn't exist or empty - create blank editor
    if (editor->line_count == 0) {
        editor->lines[0] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
        editor->line_count = 1;
    }
    
    strncpy(editor->filename, filename, sizeof(editor->filename) - 1);
    editor->modified = 0;
    return 0;
}

int editor_save_file(EditorState* editor, const char* filename) {
    if (!editor) return -1;
    
    const char* save_path = filename ? filename : editor->filename;
    if (!save_path[0]) return -1;
    
    // Build content string
    size_t total_size = 0;
    for (int i = 0; i < editor->line_count; i++) {
        total_size += strlen(editor->lines[i]) + 1; // +1 for newline
    }
    
    char* content = (char*)malloc(total_size + 1);
    if (!content) return -1;
    
    char* ptr = content;
    for (int i = 0; i < editor->line_count; i++) {
        size_t line_len = strlen(editor->lines[i]);
        memcpy(ptr, editor->lines[i], line_len);
        ptr += line_len;
        *ptr++ = '\n';
    }
    *ptr = '\0';
    
    // Write to VFS
    int result = vfs_write_file(save_path, content, total_size);
    free(content);
    
    if (result == 0) {
        strncpy(editor->filename, save_path, sizeof(editor->filename) - 1);
        editor->modified = 0;
        return 0;
    }
    
    return -1;
}

// Line operations
int editor_insert_line(EditorState* editor, int line_num) {
    if (!editor || line_num < 0 || line_num > editor->line_count) return -1;
    if (editor->line_count >= editor->max_lines) return -1;
    
    // Shift lines down
    for (int i = editor->line_count; i > line_num; i--) {
        editor->lines[i] = editor->lines[i - 1];
    }
    
    // Create new line
    editor->lines[line_num] = (char*)calloc(EDITOR_MAX_LINE_LENGTH, sizeof(char));
    editor->line_count++;
    editor->modified = 1;
    
    return 0;
}

int editor_delete_line(EditorState* editor, int line_num) {
    if (!editor || line_num < 0 || line_num >= editor->line_count) return -1;
    if (editor->line_count <= 1) {
        // Don't delete the last line, just clear it
        editor->lines[0][0] = '\0';
        editor->modified = 1;
        return 0;
    }
    
    // Free the line
    free(editor->lines[line_num]);
    
    // Shift lines up
    for (int i = line_num; i < editor->line_count - 1; i++) {
        editor->lines[i] = editor->lines[i + 1];
    }
    
    editor->line_count--;
    editor->modified = 1;
    
    return 0;
}

int editor_split_line(EditorState* editor, int line_num, int position) {
    if (!editor || line_num < 0 || line_num >= editor->line_count) return -1;
    if (position < 0 || position > (int)strlen(editor->lines[line_num])) return -1;
    if (editor->line_count >= editor->max_lines) return -1;
    
    // Create new line below
    if (editor_insert_line(editor, line_num + 1) != 0) return -1;
    
    // Copy text after cursor to new line
    strcpy(editor->lines[line_num + 1], &editor->lines[line_num][position]);
    
    // Truncate current line
    editor->lines[line_num][position] = '\0';
    
    editor->modified = 1;
    return 0;
}

// Text operations
int editor_insert_char(EditorState* editor, int line_num, int position, char ch) {
    if (!editor || line_num < 0 || line_num >= editor->line_count) return -1;
    if (position < 0 || position > EDITOR_MAX_LINE_LENGTH - 2) return -1;
    
    char* line = editor->lines[line_num];
    int len = (int)strlen(line);
    
    if (len >= EDITOR_MAX_LINE_LENGTH - 1) return -1;
    
    // Shift characters right
    for (int i = len; i >= position; i--) {
        line[i + 1] = line[i];
    }
    
    line[position] = ch;
    editor->modified = 1;
    
    return 0;
}

int editor_delete_char(EditorState* editor, int line_num, int position) {
    if (!editor || line_num < 0 || line_num >= editor->line_count) return -1;
    if (position < 0) return -1;
    
    char* line = editor->lines[line_num];
    int len = (int)strlen(line);
    
    if (position >= len) return -1;
    
    // Shift characters left
    for (int i = position; i < len; i++) {
        line[i] = line[i + 1];
    }
    
    editor->modified = 1;
    return 0;
}

// Cursor operations
int editor_move_cursor(EditorState* editor, int dx, int dy) {
    if (!editor) return -1;
    
    editor->cursor_y += dy;
    editor->cursor_x += dx;
    
    // Clamp Y
    if (editor->cursor_y < 0) editor->cursor_y = 0;
    if (editor->cursor_y >= editor->line_count) editor->cursor_y = editor->line_count - 1;
    
    // Clamp X to line length
    int line_len = (int)strlen(editor->lines[editor->cursor_y]);
    if (editor->cursor_x < 0) editor->cursor_x = 0;
    if (editor->cursor_x > line_len) editor->cursor_x = line_len;
    
    // Adjust scroll
    int screen_height = 20; // Assume 20 visible lines
    if (editor->cursor_y < editor->scroll_offset) {
        editor->scroll_offset = editor->cursor_y;
    } else if (editor->cursor_y >= editor->scroll_offset + screen_height) {
        editor->scroll_offset = editor->cursor_y - screen_height + 1;
    }
    
    return 0;
}

// Display operations
void editor_draw_screen(EditorState* editor, int screen_height, int screen_width) {
    if (!editor) return;
    
    // Clear screen
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    
    // Draw status bar at top
    printf("=== Zora Editor === File: %s %s\n", 
           editor->filename[0] ? editor->filename : "[No Name]",
           editor->modified ? "[Modified]" : "");
    printf("Ctrl+S: Save | Ctrl+Q: Quit | Line %d/%d, Col %d\n", 
           editor->cursor_y + 1, editor->line_count, editor->cursor_x + 1);
    printf("========================================\n");
    
    // Draw visible lines
    int start_line = editor->scroll_offset;
    int end_line = start_line + screen_height - 4; // -4 for status bars
    if (end_line > editor->line_count) end_line = editor->line_count;
    
    for (int i = start_line; i < end_line; i++) {
        printf("%4d | %s\n", i + 1, editor->lines[i]);
    }
    
    // Fill remaining lines
    for (int i = end_line; i < start_line + screen_height - 4; i++) {
        printf("     |\n");
    }
    
    printf("========================================\n");
    
    // Position cursor (approximate)
    // In a real implementation, we'd use ANSI codes or Windows console API
    printf("[Cursor at line %d, column %d]\n", editor->cursor_y + 1, editor->cursor_x + 1);
}

void editor_draw_status_bar(EditorState* editor) {
    if (!editor) return;
    
    printf("File: %s %s | Line %d/%d | Col %d | Ctrl+S=Save Ctrl+Q=Quit\n",
           editor->filename[0] ? editor->filename : "[No Name]",
           editor->modified ? "[*]" : "",
           editor->cursor_y + 1,
           editor->line_count,
           editor->cursor_x + 1);
}

// Main editor loop
int editor_run(EditorState* editor) {
    if (!editor) return -1;
    
    int running = 1;
    int screen_height = 24;
    int screen_width = 80;
    
    while (running) {
        editor_draw_screen(editor, screen_height, screen_width);
        
        printf("\nCommand (h=help): ");
        char cmd[100];
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        
        // Remove newline
        cmd[strcspn(cmd, "\n")] = 0;
        
        if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
            if (editor->modified) {
                printf("File modified. Save? (y/n): ");
                char response[10];
                if (fgets(response, sizeof(response), stdin)) {
                    if (response[0] == 'y' || response[0] == 'Y') {
                        if (editor_save_file(editor, NULL) == 0) {
                            printf("Saved successfully.\n");
                        } else {
                            printf("Save failed!\n");
                            continue;
                        }
                    }
                }
            }
            running = 0;
        } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "save") == 0) {
            if (editor_save_file(editor, NULL) == 0) {
                printf("Saved successfully. Press Enter...");
                getchar();
            } else {
                printf("Save failed! Press Enter...");
                getchar();
            }
        } else if (strcmp(cmd, "w") == 0) {
            editor_move_cursor(editor, 0, -1);
        } else if (strcmp(cmd, "s") == 0) {
            editor_move_cursor(editor, 0, 1);
        } else if (strcmp(cmd, "a") == 0) {
            editor_move_cursor(editor, -1, 0);
        } else if (strcmp(cmd, "d") == 0) {
            editor_move_cursor(editor, 1, 0);
        } else if (strncmp(cmd, "i ", 2) == 0) {
            // Insert text: i <text>
            char* text = cmd + 2;
            for (char* p = text; *p; p++) {
                editor_insert_char(editor, editor->cursor_y, editor->cursor_x, *p);
                editor->cursor_x++;
            }
        } else if (strcmp(cmd, "newline") == 0 || strcmp(cmd, "n") == 0) {
            editor_split_line(editor, editor->cursor_y, editor->cursor_x);
            editor->cursor_y++;
            editor->cursor_x = 0;
        } else if (strcmp(cmd, "backspace") == 0 || strcmp(cmd, "b") == 0) {
            if (editor->cursor_x > 0) {
                editor_delete_char(editor, editor->cursor_y, editor->cursor_x - 1);
                editor->cursor_x--;
            }
        } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
            printf("\nEditor Commands:\n");
            printf("  w/s/a/d     - Move cursor up/down/left/right\n");
            printf("  i <text>    - Insert text at cursor\n");
            printf("  n/newline   - Insert new line\n");
            printf("  b/backspace - Delete character before cursor\n");
            printf("  save/s      - Save file\n");
            printf("  quit/q      - Quit editor\n");
            printf("  h/help      - Show this help\n");
            printf("\nPress Enter...");
            getchar();
        }
    }
    
    return 0;
}

// Simplified nano-like interface
int editor_nano(const char* filename) {
    EditorState* editor = editor_create(10000);
    if (!editor) {
        printf("Failed to create editor!\n");
        return -1;
    }
    
    if (filename) {
        editor_load_file(editor, filename);
    }
    
    int result = editor_run(editor);
    
    editor_destroy(editor);
    return result;
}
