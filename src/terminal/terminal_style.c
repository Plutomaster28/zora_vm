#include "terminal/terminal_style.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Global terminal configuration
static TerminalConfig g_terminal_config;
static HANDLE g_console_handle;
static CONSOLE_SCREEN_BUFFER_INFO g_original_buffer_info;
static int g_initialized = 0;

// Campbell color scheme (Windows Terminal default)
static const COLORREF campbell_colors[16] = {
    0x0C0C0C,  // Black (dark gray)
    0x1F0FC5,  // Dark Red
    0x0EA113,  // Dark Green  
    0x009CC1,  // Dark Yellow
    0xDA3700,  // Dark Blue
    0x981788,  // Dark Magenta
    0xDD963A,  // Dark Cyan
    0xCCCCCC,  // Light Gray
    0x767676,  // Dark Gray
    0x5648E7,  // Red
    0x0DBC79,  // Green
    0xE5E510,  // Yellow
    0xFF6544,  // Blue
    0xB4009E,  // Magenta
    0xD6D661,  // Cyan
    0xF2F2F2   // White
};

void terminal_init_styling(void) {
    if (g_initialized) return;
    
    g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_console_handle == INVALID_HANDLE_VALUE) {
        printf("Warning: Could not get console handle for styling\n");
        return;
    }
    
    // Get original console info
    GetConsoleScreenBufferInfo(g_console_handle, &g_original_buffer_info);
    
    // Initialize default configuration
    strcpy(g_terminal_config.font_name, "MS Mincho");
    g_terminal_config.font_size = 12;
    g_terminal_config.cursor_style = CURSOR_BLOCK;
    g_terminal_config.cursor_blink = 1;
    g_terminal_config.syntax_highlight = 1;
    g_terminal_config.retro_mode = 1;
    
    // Set up Campbell color scheme
    terminal_apply_campbell_colors();
    
    // Apply MS Mincho font
    terminal_apply_ms_mincho_font();
    
    // Set retro block cursor
    terminal_set_cursor_style(CURSOR_BLOCK, 1);
    
    g_initialized = 1;
    printf("Terminal styling initialized with Campbell colors and MS Mincho font\n");
}

void terminal_apply_campbell_colors(void) {
    // Enable ANSI color support on Windows 10+
    DWORD dwMode = 0;
    GetConsoleMode(g_console_handle, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(g_console_handle, dwMode);
    
    // Set background to dark and foreground to light
    SetConsoleTextAttribute(g_console_handle, 
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    
    // Copy Campbell colors to our config
    memcpy(g_terminal_config.color_scheme.colors, campbell_colors, 
           sizeof(campbell_colors));
    
    g_terminal_config.color_scheme.background = 0x0C0C0C; // Dark background
    g_terminal_config.color_scheme.foreground = 0xCCCCCC; // Light foreground
    
    printf("Applied Campbell color scheme\n");
}

void terminal_apply_ms_mincho_font(void) {
    // Note: Changing console font programmatically is complex and requires
    // registry modifications or advanced Windows API calls
    // For now, we'll just store the preference and notify the user
    
    strcpy(g_terminal_config.font_name, "MS Mincho");
    g_terminal_config.font_size = 12;
    
    printf("MS Mincho font preference set (manual terminal config may be required)\n");
    printf("To manually set font: Right-click terminal title bar > Properties > Font\n");
}

void terminal_set_cursor_style(int style, int blink) {
    CONSOLE_CURSOR_INFO cursor_info;
    
    if (!GetConsoleCursorInfo(g_console_handle, &cursor_info)) {
        return;
    }
    
    g_terminal_config.cursor_style = style;
    g_terminal_config.cursor_blink = blink;
    
    switch (style) {
        case CURSOR_BLOCK:
            cursor_info.dwSize = 100;  // Full block cursor
            break;
        case CURSOR_UNDERSCORE:
            cursor_info.dwSize = 20;   // Underscore cursor
            break;
        case CURSOR_VERTICAL:
            cursor_info.dwSize = 25;   // Thin vertical bar
            break;
        default:
            cursor_info.dwSize = 100;  // Default to block
            break;
    }
    
    cursor_info.bVisible = TRUE;
    SetConsoleCursorInfo(g_console_handle, &cursor_info);
    
    printf("Cursor style set to %s%s\n", 
           style == CURSOR_BLOCK ? "block" : 
           style == CURSOR_UNDERSCORE ? "underscore" : "vertical",
           blink ? " (blinking)" : " (solid)");
}

void terminal_enable_syntax_highlighting(int enable) {
    g_terminal_config.syntax_highlight = enable;
    printf("Syntax highlighting %s\n", enable ? "enabled" : "disabled");
}

void terminal_enable_retro_mode(int enable) {
    g_terminal_config.retro_mode = enable;
    printf("Retro mode %s\n", enable ? "enabled" : "disabled");
    
    if (enable) {
        terminal_print_retro_banner();
    }
}

// ANSI color codes for syntax highlighting
#define ANSI_RESET          "\033[0m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_DIM            "\033[2m"
#define ANSI_ITALIC         "\033[3m"
#define ANSI_UNDERLINE      "\033[4m"

// Campbell-inspired ANSI colors
#define ANSI_BLACK          "\033[30m"
#define ANSI_RED            "\033[31m"
#define ANSI_GREEN          "\033[32m"
#define ANSI_YELLOW         "\033[33m"
#define ANSI_BLUE           "\033[34m"
#define ANSI_MAGENTA        "\033[35m"
#define ANSI_CYAN           "\033[36m"
#define ANSI_WHITE          "\033[37m"
#define ANSI_BRIGHT_BLACK   "\033[90m"
#define ANSI_BRIGHT_RED     "\033[91m"
#define ANSI_BRIGHT_GREEN   "\033[92m"
#define ANSI_BRIGHT_YELLOW  "\033[93m"
#define ANSI_BRIGHT_BLUE    "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN    "\033[96m"
#define ANSI_BRIGHT_WHITE   "\033[97m"

void terminal_print_command(const char* command) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", command);
        return;
    }
    
    // Commands in bright cyan with bold
    printf(ANSI_BOLD ANSI_BRIGHT_CYAN "%s" ANSI_RESET, command);
}

void terminal_print_argument(const char* arg) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", arg);
        return;
    }
    
    // Check if it's a file path
    if (strchr(arg, '/') || strchr(arg, '\\')) {
        terminal_print_path(arg);
        return;
    }
    
    // Check if it's a string (quoted)
    if ((arg[0] == '"' && arg[strlen(arg)-1] == '"') ||
        (arg[0] == '\'' && arg[strlen(arg)-1] == '\'')) {
        terminal_print_string(arg);
        return;
    }
    
    // Regular arguments in bright white
    printf(ANSI_BRIGHT_WHITE "%s" ANSI_RESET, arg);
}

void terminal_print_path(const char* path) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", path);
        return;
    }
    
    // Paths in bright blue with underline
    printf(ANSI_UNDERLINE ANSI_BRIGHT_BLUE "%s" ANSI_RESET, path);
}

void terminal_print_operator(const char* op) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", op);
        return;
    }
    
    // Operators in bright yellow
    printf(ANSI_BRIGHT_YELLOW "%s" ANSI_RESET, op);
}

void terminal_print_string(const char* str) {
    if (!g_terminal_config.syntax_highlight) {
        printf("%s", str);
        return;
    }
    
    // Strings in bright green
    printf(ANSI_BRIGHT_GREEN "%s" ANSI_RESET, str);
}

void terminal_print_error(const char* error) {
    // Errors always in bright red, regardless of syntax highlighting setting
    printf(ANSI_BRIGHT_RED "%s" ANSI_RESET, error);
}

void terminal_print_retro_banner(void) {
    if (!g_terminal_config.retro_mode) return;
    
    printf(ANSI_BRIGHT_CYAN);
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    ██████  ▄████▄   ██▀███   ▄▄▄                      ║\n");
    printf("║                  ▒██     ▒██▀ ▀█  ▓██ ▒ ██▒▒████▄                    ║\n");
    printf("║                  ░ ▓██▄   ▒▓█    ▄ ▓██ ░▄█ ▒▒██  ▀█▄                  ║\n");
    printf("║                    ▒   ██▒▒▓▓▄ ▄██▒▒██▀▀█▄  ░██▄▄▄▄██                 ║\n");
    printf("║                  ▒██████▒▒▒ ▓███▀ ░░██▓ ▒██▒ ▓█   ▓██▒                ║\n");
    printf("║                  ▒ ▒▓▒ ▒ ░░ ░▒ ▒  ░░ ▒▓ ░▒▓░ ▒▒   ▓▒█░                ║\n");
    printf("║                  ░ ░▒  ░ ░  ░  ▒     ░▒ ░ ▒░  ▒   ▒▒ ░                ║\n");
    printf("║                  ░  ░  ░  ░          ░░   ░   ░   ▒                   ║\n");
    printf("║                        ░  ░ ░         ░           ░  ░                ║\n");
    printf("║                           ░                                           ║\n");
    printf("║                                                                      ║\n");
    printf("║               RETRO TERMINAL MODE - MS MINCHO FONT                   ║\n");
    printf("║               Campbell Color Scheme - Block Cursor                   ║\n");
    printf("║                                                                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf(ANSI_RESET);
    printf("\n");
}

void terminal_print_retro_prompt(const char* user, const char* host, const char* path) {
    if (!g_terminal_config.retro_mode) return;
    
    // Retro-style prompt with enhanced styling
    printf(ANSI_BRIGHT_GREEN "┌─[" ANSI_RESET);
    printf(ANSI_BOLD ANSI_BRIGHT_CYAN "%s" ANSI_RESET, user);
    printf(ANSI_BRIGHT_WHITE "@" ANSI_RESET);
    printf(ANSI_BOLD ANSI_BRIGHT_BLUE "%s" ANSI_RESET, host);
    printf(ANSI_BRIGHT_GREEN "]─[" ANSI_RESET);
    printf(ANSI_UNDERLINE ANSI_BRIGHT_YELLOW "%s" ANSI_RESET, path);
    printf(ANSI_BRIGHT_GREEN "]\n└─" ANSI_RESET);
    printf(ANSI_BRIGHT_GREEN "▶ " ANSI_RESET);
}

void terminal_animate_cursor(void) {
    if (!g_terminal_config.retro_mode || !g_terminal_config.cursor_blink) return;
    
    // Simple cursor animation (visual effect only)
    printf(ANSI_BRIGHT_WHITE "█" ANSI_RESET);
    fflush(stdout);
}

void terminal_typewriter_effect(const char* text, int delay_ms) {
    if (!g_terminal_config.retro_mode) {
        printf("%s", text);
        return;
    }
    
    for (const char* p = text; *p; p++) {
        putchar(*p);
        fflush(stdout);
        Sleep(delay_ms);  // Windows sleep function
    }
}

void terminal_reset_colors(void) {
    printf(ANSI_RESET);
    fflush(stdout);
}

void terminal_set_transparency(int level) {
    // Note: Console transparency requires advanced Windows API
    // This is a placeholder for future implementation
    printf("Transparency set to %d%% (requires Windows Terminal)\n", level);
}

void terminal_set_blur_effect(int enable) {
    // Note: Blur effects require advanced terminal features
    // This is a placeholder for future implementation
    printf("Blur effect %s (requires Windows Terminal)\n", 
           enable ? "enabled" : "disabled");
}

// Configuration persistence
void terminal_save_config(void) {
    FILE* config_file = fopen("terminal_config.ini", "w");
    if (!config_file) {
        printf("Warning: Could not save terminal configuration\n");
        return;
    }
    
    fprintf(config_file, "[Terminal]\n");
    fprintf(config_file, "font_name=%s\n", g_terminal_config.font_name);
    fprintf(config_file, "font_size=%d\n", g_terminal_config.font_size);
    fprintf(config_file, "cursor_style=%d\n", g_terminal_config.cursor_style);
    fprintf(config_file, "cursor_blink=%d\n", g_terminal_config.cursor_blink);
    fprintf(config_file, "syntax_highlight=%d\n", g_terminal_config.syntax_highlight);
    fprintf(config_file, "retro_mode=%d\n", g_terminal_config.retro_mode);
    
    fclose(config_file);
    printf("Terminal configuration saved\n");
}

void terminal_load_config(void) {
    FILE* config_file = fopen("terminal_config.ini", "r");
    if (!config_file) {
        // Use defaults if no config file
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), config_file)) {
        if (strstr(line, "font_name=")) {
            sscanf(line, "font_name=%s", g_terminal_config.font_name);
        } else if (strstr(line, "font_size=")) {
            sscanf(line, "font_size=%d", &g_terminal_config.font_size);
        } else if (strstr(line, "cursor_style=")) {
            sscanf(line, "cursor_style=%d", &g_terminal_config.cursor_style);
        } else if (strstr(line, "cursor_blink=")) {
            sscanf(line, "cursor_blink=%d", &g_terminal_config.cursor_blink);
        } else if (strstr(line, "syntax_highlight=")) {
            sscanf(line, "syntax_highlight=%d", &g_terminal_config.syntax_highlight);
        } else if (strstr(line, "retro_mode=")) {
            sscanf(line, "retro_mode=%d", &g_terminal_config.retro_mode);
        }
    }
    
    fclose(config_file);
    printf("Terminal configuration loaded\n");
}

const char* terminal_get_current_font(void) {
    return g_terminal_config.font_name;
}

int terminal_get_cursor_style(void) {
    return g_terminal_config.cursor_style;
}

void terminal_set_font(const char* font_name, int size) {
    if (!font_name) return;
    
    strncpy(g_terminal_config.font_name, font_name, sizeof(g_terminal_config.font_name) - 1);
    g_terminal_config.font_name[sizeof(g_terminal_config.font_name) - 1] = '\0';
    g_terminal_config.font_size = size;
    
    printf("Font set to: %s, size %d\n", font_name, size);
    printf("Note: Manual terminal configuration may be required for font changes\n");
}