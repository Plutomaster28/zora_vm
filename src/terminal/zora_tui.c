#include "terminal/zora_tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// Global TUI instance
static ZoraTUI g_tui = {0};

// Predefined ZoraVM themes
ZoraTheme zora_theme_matrix = {
    .background = ZORA_BLACK,
    .foreground = ZORA_GREEN,
    .accent = ZORA_LIME,
    .warning = ZORA_YELLOW,
    .error = ZORA_RED,
    .success = ZORA_GREEN,
    .prompt = ZORA_CYAN,
    .command = ZORA_WHITE,
    .path = ZORA_BLUE,
    .time = ZORA_DARK_GRAY,
    .name = "Matrix"
};

ZoraTheme zora_theme_amber = {
    .background = ZORA_BLACK,
    .foreground = ZORA_AMBER,
    .accent = ZORA_YELLOW,
    .warning = ZORA_YELLOW,
    .error = ZORA_RED,
    .success = ZORA_AMBER,
    .prompt = ZORA_AMBER,
    .command = ZORA_WHITE,
    .path = ZORA_AMBER,
    .time = ZORA_DARK_GRAY,
    .name = "Amber"
};

ZoraTheme zora_theme_cyberpunk = {
    .background = ZORA_BLACK,
    .foreground = ZORA_CYAN,
    .accent = ZORA_MAGENTA,
    .warning = ZORA_YELLOW,
    .error = ZORA_RED,
    .success = ZORA_GREEN,
    .prompt = ZORA_PURPLE,
    .command = ZORA_WHITE,
    .path = ZORA_CYAN,
    .time = ZORA_BLUE,
    .name = "Cyberpunk"
};

ZoraTheme zora_theme_vintage = {
    .background = ZORA_BLACK,
    .foreground = ZORA_LIGHT_GRAY,
    .accent = ZORA_WHITE,
    .warning = ZORA_YELLOW,
    .error = ZORA_RED,
    .success = ZORA_GREEN,
    .prompt = ZORA_LIGHT_GRAY,
    .command = ZORA_WHITE,
    .path = ZORA_CYAN,
    .time = ZORA_DARK_GRAY,
    .name = "Vintage"
};

ZoraTheme zora_theme_zora = {
    .background = ZORA_BLACK,
    .foreground = ZORA_WHITE,
    .accent = ZORA_CYAN,
    .warning = ZORA_ORANGE,
    .error = ZORA_RED,
    .success = ZORA_LIME,
    .prompt = ZORA_CYAN,
    .command = ZORA_WHITE,
    .path = ZORA_BLUE,
    .time = ZORA_PURPLE,
    .name = "Zora"
};

// Current active theme
static ZoraTheme* g_current_theme = &zora_theme_zora;

// Windows color mapping for our custom colors
static WORD zora_color_to_windows(ZoraColor color) {
    switch (color) {
        case ZORA_BLACK: return 0;
        case ZORA_DARK_BLUE: return 1;
        case ZORA_DARK_GREEN: return 2;
        case ZORA_DARK_CYAN: return 3;
        case ZORA_DARK_RED: return 4;
        case ZORA_DARK_MAGENTA: return 5;
        case ZORA_DARK_YELLOW: return 6;
        case ZORA_LIGHT_GRAY: return 7;
        case ZORA_DARK_GRAY: return 8;
        case ZORA_BLUE: return 9;
        case ZORA_GREEN: return 10;
        case ZORA_CYAN: return 11;
        case ZORA_RED: return 12;
        case ZORA_MAGENTA: return 13;
        case ZORA_YELLOW: return 14;
        case ZORA_WHITE: return 15;
        // Map custom colors to closest equivalents
        case ZORA_AMBER: return 14; // Yellow
        case ZORA_LIME: return 10;  // Bright Green
        case ZORA_PURPLE: return 13; // Magenta
        case ZORA_ORANGE: return 12; // Red
        default: return 7;
    }
}

int zora_tui_init(void) {
    if (g_tui.initialized) {
        return 0; // Already initialized
    }
    
    // Get console handle
    g_tui.console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_tui.console_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    // Get screen buffer info
    if (!GetConsoleScreenBufferInfo(g_tui.console_handle, &g_tui.buffer_info)) {
        return -1;
    }
    
    // Set up screen dimensions
    g_tui.width = g_tui.buffer_info.srWindow.Right - g_tui.buffer_info.srWindow.Left + 1;
    g_tui.height = g_tui.buffer_info.srWindow.Bottom - g_tui.buffer_info.srWindow.Top + 1;
    g_tui.screen_size.X = g_tui.width;
    g_tui.screen_size.Y = g_tui.height;
    
    // Allocate screen buffer
    g_tui.screen_buffer = calloc(g_tui.width * g_tui.height, sizeof(CHAR_INFO));
    if (!g_tui.screen_buffer) {
        return -1;
    }
    
    // Create new screen buffer for double buffering
    g_tui.buffer_handle = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
    
    if (g_tui.buffer_handle == INVALID_HANDLE_VALUE) {
        free(g_tui.screen_buffer);
        return -1;
    }
    
    // Initialize default state
    g_tui.cursor_x = 0;
    g_tui.cursor_y = 0;
    g_tui.fg_color = g_current_theme->foreground;
    g_tui.bg_color = g_current_theme->background;
    g_tui.style = ZORA_STYLE_NORMAL;
    
    // Set console title
    SetConsoleTitle(TEXT("ZoraVM - Advanced Virtual Machine"));
    
    // Enable extended console features
    DWORD mode;
    GetConsoleMode(g_tui.console_handle, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
    SetConsoleMode(g_tui.console_handle, mode);
    
    g_tui.initialized = 1;
    
    // Clear screen and draw initial interface
    zora_tui_clear_screen();
    
    return 0;
}

void zora_tui_cleanup(void) {
    if (!g_tui.initialized) return;
    
    if (g_tui.screen_buffer) {
        free(g_tui.screen_buffer);
        g_tui.screen_buffer = NULL;
    }
    
    if (g_tui.buffer_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_tui.buffer_handle);
        g_tui.buffer_handle = INVALID_HANDLE_VALUE;
    }
    
    // Restore original console state
    SetConsoleActiveScreenBuffer(g_tui.console_handle);
    
    g_tui.initialized = 0;
}

void zora_tui_clear_screen(void) {
    if (!g_tui.initialized) return;
    
    // Fill screen buffer with background color
    WORD attr = zora_color_to_windows(g_current_theme->background);
    for (int i = 0; i < g_tui.width * g_tui.height; i++) {
        g_tui.screen_buffer[i].Char.AsciiChar = ' ';
        g_tui.screen_buffer[i].Attributes = attr;
    }
    
    g_tui.cursor_x = 0;
    g_tui.cursor_y = 0;
}

void zora_tui_refresh(void) {
    if (!g_tui.initialized) return;
    
    // Write screen buffer to console
    COORD bufferSize = {g_tui.width, g_tui.height};
    COORD bufferCoord = {0, 0};
    SMALL_RECT writeRegion = {0, 0, g_tui.width - 1, g_tui.height - 1};
    
    WriteConsoleOutput(g_tui.console_handle, g_tui.screen_buffer, 
                      bufferSize, bufferCoord, &writeRegion);
    
    // Set cursor position
    COORD cursorPos = {g_tui.cursor_x, g_tui.cursor_y};
    SetConsoleCursorPosition(g_tui.console_handle, cursorPos);
}

void zora_tui_set_theme(ZoraTheme* theme) {
    if (!theme) return;
    g_current_theme = theme;
    g_tui.fg_color = theme->foreground;
    g_tui.bg_color = theme->background;
}

void zora_tui_set_color(ZoraColor fg, ZoraColor bg) {
    g_tui.fg_color = fg;
    g_tui.bg_color = bg;
}

void zora_tui_set_style(ZoraStyle style) {
    g_tui.style = style;
}

void zora_tui_move_cursor(int x, int y) {
    if (x >= 0 && x < g_tui.width) g_tui.cursor_x = x;
    if (y >= 0 && y < g_tui.height) g_tui.cursor_y = y;
}

void zora_tui_print(const char* text) {
    if (!g_tui.initialized || !text) return;
    
    WORD attr = zora_color_to_windows(g_tui.fg_color) | 
                (zora_color_to_windows(g_tui.bg_color) << 4);
    
    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            g_tui.cursor_y++;
            g_tui.cursor_x = 0;
            if (g_tui.cursor_y >= g_tui.height) {
                g_tui.cursor_y = g_tui.height - 1;
                // TODO: Implement scrolling
            }
        } else if (*p == '\r') {
            g_tui.cursor_x = 0;
        } else if (*p == '\t') {
            g_tui.cursor_x = (g_tui.cursor_x + 8) & ~7; // Tab to next 8-char boundary
            if (g_tui.cursor_x >= g_tui.width) {
                g_tui.cursor_x = 0;
                g_tui.cursor_y++;
            }
        } else {
            if (g_tui.cursor_x < g_tui.width && g_tui.cursor_y < g_tui.height) {
                int pos = g_tui.cursor_y * g_tui.width + g_tui.cursor_x;
                g_tui.screen_buffer[pos].Char.AsciiChar = *p;
                g_tui.screen_buffer[pos].Attributes = attr;
                g_tui.cursor_x++;
                
                if (g_tui.cursor_x >= g_tui.width) {
                    g_tui.cursor_x = 0;
                    g_tui.cursor_y++;
                }
            }
        }
    }
}

void zora_tui_printf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    zora_tui_print(buffer);
}

void zora_tui_print_colored(const char* text, ZoraColor fg, ZoraColor bg) {
    ZoraColor old_fg = g_tui.fg_color;
    ZoraColor old_bg = g_tui.bg_color;
    
    zora_tui_set_color(fg, bg);
    zora_tui_print(text);
    zora_tui_set_color(old_fg, old_bg);
}

void zora_tui_print_styled(const char* text, ZoraColor fg, ZoraColor bg, ZoraStyle style) {
    ZoraColor old_fg = g_tui.fg_color;
    ZoraColor old_bg = g_tui.bg_color;
    ZoraStyle old_style = g_tui.style;
    
    zora_tui_set_color(fg, bg);
    zora_tui_set_style(style);
    zora_tui_print(text);
    zora_tui_set_color(old_fg, old_bg);
    zora_tui_set_style(old_style);
}

void zora_tui_draw_box(int x, int y, int width, int height, ZoraColor color) {
    if (!g_tui.initialized) return;
    
    ZoraColor old_fg = g_tui.fg_color;
    zora_tui_set_color(color, g_tui.bg_color);
    
    // Box drawing characters
    const char* box_chars[] = {
        "┌", "─", "┐",  // Top
        "│", " ", "│",  // Middle
        "└", "─", "┘"   // Bottom
    };
    
    // Draw top border
    zora_tui_move_cursor(x, y);
    zora_tui_print(box_chars[0]);
    for (int i = 1; i < width - 1; i++) {
        zora_tui_print(box_chars[1]);
    }
    zora_tui_print(box_chars[2]);
    
    // Draw side borders
    for (int row = 1; row < height - 1; row++) {
        zora_tui_move_cursor(x, y + row);
        zora_tui_print(box_chars[3]);
        zora_tui_move_cursor(x + width - 1, y + row);
        zora_tui_print(box_chars[5]);
    }
    
    // Draw bottom border
    zora_tui_move_cursor(x, y + height - 1);
    zora_tui_print(box_chars[6]);
    for (int i = 1; i < width - 1; i++) {
        zora_tui_print(box_chars[7]);
    }
    zora_tui_print(box_chars[8]);
    
    zora_tui_set_color(old_fg, g_tui.bg_color);
}

void zora_tui_draw_border(int x, int y, int width, int height, const char* title) {
    zora_tui_draw_box(x, y, width, height, g_current_theme->accent);
    
    if (title && strlen(title) > 0) {
        int title_len = strlen(title);
        int title_x = x + (width - title_len - 2) / 2;
        zora_tui_move_cursor(title_x, y);
        zora_tui_print_colored(" ", g_current_theme->accent, g_current_theme->background);
        zora_tui_print_colored(title, g_current_theme->foreground, g_current_theme->background);
        zora_tui_print_colored(" ", g_current_theme->accent, g_current_theme->background);
    }
}

void zora_tui_draw_separator(int y, ZoraColor color) {
    zora_tui_move_cursor(0, y);
    for (int i = 0; i < g_tui.width; i++) {
        zora_tui_print_colored("─", color, g_current_theme->background);
    }
}

void zora_tui_draw_header(void) {
    // ZoraVM header with ASCII art
    zora_tui_move_cursor(0, 0);
    zora_tui_print_colored("╔══════════════════════════════════════════════════════════════════════════════╗\n", 
                          g_current_theme->accent, g_current_theme->background);
    zora_tui_print_colored("║                           ███████╗ ██████╗ ██████╗  █████╗                   ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored("║                           ╚══███╔╝██╔═══██╗██╔══██╗██╔══██╗                  ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored("║                             ███╔╝ ██║   ██║██████╔╝███████║                  ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored("║                            ███╔╝  ██║   ██║██╔══██╗██╔══██║                  ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored("║                           ███████╗╚██████╔╝██║  ██║██║  ██║                  ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored("║                           ╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝                  ║\n", 
                          g_current_theme->foreground, g_current_theme->background);
    zora_tui_printf("║                                "); 
    zora_tui_print_colored("Virtual Machine v2.1.0", g_current_theme->accent, g_current_theme->background);
    zora_tui_print_colored("                               ║\n", g_current_theme->accent, g_current_theme->background);
    zora_tui_print_colored("╚══════════════════════════════════════════════════════════════════════════════╝\n", 
                          g_current_theme->accent, g_current_theme->background);
}

void zora_tui_draw_prompt(const char* user, const char* host, const char* path) {
    // Colorful prompt: user@host:path$ 
    zora_tui_print_colored(user ? user : "guest", g_current_theme->prompt, g_current_theme->background);
    zora_tui_print_colored("@", g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored(host ? host : "zora-vm", g_current_theme->prompt, g_current_theme->background);
    zora_tui_print_colored(":", g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored(path ? path : "/", g_current_theme->path, g_current_theme->background);
    zora_tui_print_colored("> ", g_current_theme->accent, g_current_theme->background);
}

void zora_tui_draw_status_bar(const char* status) {
    int status_y = g_tui.height - 1;
    zora_tui_move_cursor(0, status_y);
    
    // Fill status bar with background
    for (int i = 0; i < g_tui.width; i++) {
        zora_tui_print_colored(" ", g_current_theme->background, g_current_theme->accent);
    }
    
    zora_tui_move_cursor(1, status_y);
    zora_tui_print_colored(status ? status : "ZoraVM Ready", 
                          g_current_theme->background, g_current_theme->accent);
    
    // Show current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    int time_x = g_tui.width - strlen(time_str) - 1;
    zora_tui_move_cursor(time_x, status_y);
    zora_tui_print_colored(time_str, g_current_theme->background, g_current_theme->accent);
}

void zora_tui_draw_boot_splash(void) {
    zora_tui_clear_screen();
    
    // Center the splash screen
    int center_y = g_tui.height / 2 - 5;
    
    zora_tui_move_cursor(0, center_y);
    zora_tui_draw_header();
    
    zora_tui_move_cursor(0, center_y + 10);
    zora_tui_print_colored("                            Theme: ", g_current_theme->foreground, g_current_theme->background);
    zora_tui_print_colored(g_current_theme->name, g_current_theme->accent, g_current_theme->background);
    zora_tui_print_colored("\n                            Loading ZoraVM...\n", g_current_theme->foreground, g_current_theme->background);
    
    zora_tui_refresh();
}

char zora_tui_get_char(void) {
    if (!g_tui.initialized) return 0;
    
    DWORD events_read;
    INPUT_RECORD input_record;
    
    while (1) {
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input_record, 1, &events_read);
        
        if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown) {
            return input_record.Event.KeyEvent.uChar.AsciiChar;
        }
    }
}

void zora_tui_set_title(const char* title) {
    if (title) {
        SetConsoleTitle(TEXT(title));
    }
}

void zora_tui_show_cursor(int visible) {
    CONSOLE_CURSOR_INFO cursor_info;
    GetConsoleCursorInfo(g_tui.console_handle, &cursor_info);
    cursor_info.bVisible = visible;
    SetConsoleCursorInfo(g_tui.console_handle, &cursor_info);
}
