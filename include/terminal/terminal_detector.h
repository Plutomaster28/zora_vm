#ifndef TERMINAL_DETECTOR_H
#define TERMINAL_DETECTOR_H

// Terminal detection and compatibility layer

typedef enum {
    BOX_TOP_LEFT,
    BOX_TOP_RIGHT,
    BOX_BOTTOM_LEFT,
    BOX_BOTTOM_RIGHT,
    BOX_HORIZONTAL,
    BOX_VERTICAL,
    BOX_CROSS,
    BOX_T_DOWN,
    BOX_T_UP,
    BOX_T_RIGHT,
    BOX_T_LEFT
} BoxChar;

typedef enum {
    BOX_LINE_TOP,
    BOX_LINE_BOTTOM,
    BOX_LINE_MIDDLE
} BoxLineType;

// Core functions
int detect_windows_terminal(void);
int try_launch_windows_terminal(const char* executable_path);
const char* get_box_char(BoxChar char_type);
void print_box_line(int width, BoxLineType type);
void print_terminal_info(void);

// Convenience macros for common operations
#define PRINT_BOX_TOP(width) print_box_line(width, BOX_LINE_TOP)
#define PRINT_BOX_BOTTOM(width) print_box_line(width, BOX_LINE_BOTTOM)
#define PRINT_BOX_MIDDLE(width) print_box_line(width, BOX_LINE_MIDDLE)

#endif // TERMINAL_DETECTOR_H
