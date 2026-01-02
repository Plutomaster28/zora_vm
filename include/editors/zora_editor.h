#ifndef ZORA_EDITOR_H
#define ZORA_EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Editor configuration
#define EDITOR_MAX_LINES 10000
#define EDITOR_MAX_LINE_LENGTH 1024
#define EDITOR_DEFAULT_MAX_LINES 10000
#define EDITOR_TAB_WIDTH 4

// Editor modes
typedef enum {
    EDITOR_MODE_NORMAL,
    EDITOR_MODE_INSERT,
    EDITOR_MODE_COMMAND
} EditorMode;

// Editor state structure
typedef struct {
    char** lines;           // Array of line buffers
    int line_count;         // Current number of lines
    int cursor_row;         // Current cursor row (0-based) - alias for cursor_y
    int cursor_col;         // Current cursor column (0-based) - alias for cursor_x
    int cursor_y;           // Alternative name for cursor_row
    int cursor_x;           // Alternative name for cursor_col
    int view_offset;        // First visible line - alias for scroll_offset
    int scroll_offset;      // Alternative name for view_offset
    int max_lines;          // Maximum capacity
    char filename[256];     // Current file being edited
    int modified;           // Has the buffer been modified?
    EditorMode mode;        // Current editor mode
    char status_msg[256];   // Status line message
    int quit_requested;     // User wants to quit
} EditorState;

// Editor initialization and cleanup
EditorState* editor_create(int max_lines);
void editor_destroy(EditorState* editor);

// File operations
int editor_load_file(EditorState* editor, const char* filename);
int editor_save_file(EditorState* editor, const char* filename);
int editor_save_as(EditorState* editor, const char* filename);

// Buffer operations  
int editor_insert_char(EditorState* editor, int line_num, int position, char ch);
int editor_delete_char(EditorState* editor, int line_num, int position);
int editor_split_line(EditorState* editor, int line_num, int position);
int editor_new_line(EditorState* editor);
int editor_delete_line(EditorState* editor, int line_num);
int editor_insert_line(EditorState* editor, int line_num);
int editor_move_cursor(EditorState* editor, int dx, int dy);
void editor_move_cursor_up(EditorState* editor);
void editor_move_cursor_down(EditorState* editor);
void editor_move_cursor_left(EditorState* editor);
void editor_move_cursor_right(EditorState* editor);
void editor_move_to_line_start(EditorState* editor);
void editor_move_to_line_end(EditorState* editor);
void editor_move_to_file_start(EditorState* editor);
void editor_move_to_file_end(EditorState* editor);
void editor_page_up(EditorState* editor);
void editor_page_down(EditorState* editor);

// Display operations
void editor_draw_screen(EditorState* editor, int screen_height, int screen_width);
void editor_draw_status_bar(EditorState* editor);
void editor_refresh_screen(EditorState* editor);
void editor_draw_help_bar(EditorState* editor);

// Main editor loop
int editor_run(EditorState* editor);

// Simplified nano-like interface
int editor_nano(const char* filename);

// Search and replace
int editor_search(EditorState* editor, const char* query);
int editor_replace(EditorState* editor, const char* find, const char* replace);

// Command processing
void editor_process_command(EditorState* editor, const char* command);

// Main editor loop
int editor_run(EditorState* editor);

// Helper functions
void editor_set_status(EditorState* editor, const char* format, ...);
int editor_confirm(const char* prompt);

#endif // ZORA_EDITOR_H
