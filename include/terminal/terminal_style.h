#ifndef TERMINAL_STYLE_H
#define TERMINAL_STYLE_H

#include <windows.h>
#include <stdio.h>

// Campbell PowerShell color scheme (RGB values)
typedef struct {
    COLORREF foreground;
    COLORREF background;
    COLORREF colors[16];  // Standard 16 colors
} TerminalColorScheme;

// Terminal styling configuration
typedef struct {
    char font_name[64];
    int font_size;
    int cursor_style;  // 0 = block, 1 = underscore, 2 = vertical bar
    int cursor_blink;
    TerminalColorScheme color_scheme;
    int syntax_highlight;  // Enable command syntax highlighting
    int retro_mode;        // Enable retro terminal features
} TerminalConfig;

// Global terminal configuration (defined in terminal_style.c)
extern TerminalConfig g_terminal_config;

// Cursor styles
#define CURSOR_BLOCK     0
#define CURSOR_UNDERSCORE 1
#define CURSOR_VERTICAL  2

// Syntax highlighting styles
#define SYNTAX_COMMAND   0
#define SYNTAX_ARGUMENT  1
#define SYNTAX_PATH      2
#define SYNTAX_OPERATOR  3
#define SYNTAX_STRING    4
#define SYNTAX_COMMENT   5
#define SYNTAX_ERROR     6

// Function prototypes
void terminal_init_styling(void);
void terminal_set_font(const char* font_name, int size);
void terminal_set_cursor_style(int style, int blink);
void terminal_set_color_scheme(const char* scheme_name);
void terminal_apply_campbell_colors(void);
void terminal_apply_ms_mincho_font(void);
void terminal_enable_syntax_highlighting(int enable);
void terminal_enable_retro_mode(int enable);

// Syntax highlighting functions
void terminal_print_command(const char* command);
void terminal_print_argument(const char* arg);
void terminal_print_path(const char* path);
void terminal_print_operator(const char* op);
void terminal_print_string(const char* str);
void terminal_print_error(const char* error);

// Retro styling functions
void terminal_print_retro_banner(void);
void terminal_print_retro_prompt(const char* user, const char* host, const char* path);
void terminal_animate_cursor(void);

// Utility functions
void terminal_reset_colors(void);
void terminal_save_config(void);
void terminal_load_config(void);
const char* terminal_get_current_font(void);
int terminal_get_cursor_style(void);

// Advanced features
void terminal_set_transparency(int level);  // 0-100%
void terminal_set_blur_effect(int enable);
void terminal_typewriter_effect(const char* text, int delay_ms);

#endif // TERMINAL_STYLE_H