#ifndef ZORA_TUI_H
#define ZORA_TUI_H

#include <windows.h>
#include <stdint.h>

// ZoraVM Custom TUI Colors (inspired by vintage terminals)
typedef enum {
    ZORA_BLACK = 0,
    ZORA_DARK_BLUE = 1,
    ZORA_DARK_GREEN = 2,
    ZORA_DARK_CYAN = 3,
    ZORA_DARK_RED = 4,
    ZORA_DARK_MAGENTA = 5,
    ZORA_DARK_YELLOW = 6,
    ZORA_LIGHT_GRAY = 7,
    ZORA_DARK_GRAY = 8,
    ZORA_BLUE = 9,
    ZORA_GREEN = 10,
    ZORA_CYAN = 11,
    ZORA_RED = 12,
    ZORA_MAGENTA = 13,
    ZORA_YELLOW = 14,
    ZORA_WHITE = 15,
    // Custom ZoraVM colors
    ZORA_AMBER = 16,
    ZORA_LIME = 17,
    ZORA_PURPLE = 18,
    ZORA_ORANGE = 19
} ZoraColor;

// ZoraVM Style attributes
typedef enum {
    ZORA_STYLE_NORMAL = 0,
    ZORA_STYLE_BOLD = 1,
    ZORA_STYLE_DIM = 2,
    ZORA_STYLE_ITALIC = 4,
    ZORA_STYLE_UNDERLINE = 8,
    ZORA_STYLE_BLINK = 16,
    ZORA_STYLE_REVERSE = 32,
    ZORA_STYLE_STRIKETHROUGH = 64
} ZoraStyle;

// Screen buffer for custom rendering
typedef struct {
    HANDLE console_handle;
    HANDLE buffer_handle;
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    COORD screen_size;
    CHAR_INFO* screen_buffer;
    int width;
    int height;
    int cursor_x;
    int cursor_y;
    ZoraColor fg_color;
    ZoraColor bg_color;
    ZoraStyle style;
    int initialized;
} ZoraTUI;

// ZoraVM Terminal Theme
typedef struct {
    ZoraColor background;
    ZoraColor foreground;
    ZoraColor accent;
    ZoraColor warning;
    ZoraColor error;
    ZoraColor success;
    ZoraColor prompt;
    ZoraColor command;
    ZoraColor path;
    ZoraColor time;
    char name[32];
} ZoraTheme;

// Predefined themes
extern ZoraTheme zora_theme_matrix;     // Green-on-black matrix style
extern ZoraTheme zora_theme_amber;      // Amber monochrome
extern ZoraTheme zora_theme_cyberpunk;  // Neon colors
extern ZoraTheme zora_theme_vintage;    // Classic terminal
extern ZoraTheme zora_theme_zora;       // Custom ZoraVM theme

// Core TUI functions
int zora_tui_init(void);
void zora_tui_cleanup(void);
void zora_tui_clear_screen(void);
void zora_tui_refresh(void);
void zora_tui_set_theme(ZoraTheme* theme);

// Drawing functions
void zora_tui_set_color(ZoraColor fg, ZoraColor bg);
void zora_tui_set_style(ZoraStyle style);
void zora_tui_move_cursor(int x, int y);
void zora_tui_print(const char* text);
void zora_tui_printf(const char* format, ...);
void zora_tui_print_colored(const char* text, ZoraColor fg, ZoraColor bg);
void zora_tui_print_styled(const char* text, ZoraColor fg, ZoraColor bg, ZoraStyle style);

// Box drawing and borders
void zora_tui_draw_box(int x, int y, int width, int height, ZoraColor color);
void zora_tui_draw_border(int x, int y, int width, int height, const char* title);
void zora_tui_draw_separator(int y, ZoraColor color);

// ZoraVM specific UI elements
void zora_tui_draw_header(void);
void zora_tui_draw_prompt(const char* user, const char* host, const char* path);
void zora_tui_draw_status_bar(const char* status);
void zora_tui_draw_boot_splash(void);

// Input handling
char zora_tui_get_char(void);
int zora_tui_get_key(void);
void zora_tui_read_line(char* buffer, int max_length);

// Advanced features
void zora_tui_enable_mouse(void);
void zora_tui_disable_mouse(void);
void zora_tui_set_title(const char* title);
void zora_tui_show_cursor(int visible);

#endif // ZORA_TUI_H
