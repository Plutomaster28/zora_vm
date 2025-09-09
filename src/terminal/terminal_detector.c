#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "terminal_detector.h"

static int is_windows_terminal = 0;
static int terminal_capabilities_checked = 0;

// Check if we're running in Windows Terminal vs Console Host
int detect_windows_terminal(void) {
    if (terminal_capabilities_checked) {
        return is_windows_terminal;
    }
    
    // Method 1: Check WT_SESSION environment variable (Windows Terminal sets this)
    char* wt_session = getenv("WT_SESSION");
    if (wt_session != NULL) {
        is_windows_terminal = 1;
        terminal_capabilities_checked = 1;
        return 1;
    }
    
    // Method 2: Check TERM_PROGRAM environment variable
    char* term_program = getenv("TERM_PROGRAM");
    if (term_program != NULL && strstr(term_program, "Windows Terminal") != NULL) {
        is_windows_terminal = 1;
        terminal_capabilities_checked = 1;
        return 1;
    }
    
    // Method 3: Check if console supports virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            // Try to enable virtual terminal processing
            DWORD newMode = dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, newMode)) {
                // If we can set VT mode, likely modern terminal
                is_windows_terminal = 1;
            } else {
                // Fallback to old console host
                is_windows_terminal = 0;
            }
        }
    }
    
    terminal_capabilities_checked = 1;
    return is_windows_terminal;
}

// Try to launch Windows Terminal if available
int try_launch_windows_terminal(const char* executable_path) {
    char wt_command[1024];
    char current_dir[512];
    
    // Get current directory
    GetCurrentDirectoryA(sizeof(current_dir), current_dir);
    
    // Try different Windows Terminal launch methods
    const char* wt_attempts[] = {
        "wt.exe -d \"%s\" \"%s\"",                    // Windows Terminal with directory
        "wt -d \"%s\" \"%s\"",                        // Windows Terminal short form
        "start wt.exe -d \"%s\" \"%s\"",              // Start command
        "\"C:\\Program Files\\WindowsApps\\Microsoft.WindowsTerminal_*\\wt.exe\" -d \"%s\" \"%s\"",  // Full path attempt
        NULL
    };
    
    for (int i = 0; wt_attempts[i] != NULL; i++) {
        snprintf(wt_command, sizeof(wt_command), wt_attempts[i], current_dir, executable_path);
        
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        
        if (CreateProcessA(NULL, wt_command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 1; // Successfully launched in Windows Terminal
        }
    }
    
    return 0; // Failed to launch Windows Terminal
}

// Get appropriate box drawing characters based on terminal capabilities
const char* get_box_char(BoxChar char_type) {
    if (detect_windows_terminal()) {
        // Use Unicode box drawing characters for modern terminals
        switch (char_type) {
            case BOX_TOP_LEFT:     return "╔";
            case BOX_TOP_RIGHT:    return "╗";
            case BOX_BOTTOM_LEFT:  return "╚";
            case BOX_BOTTOM_RIGHT: return "╝";
            case BOX_HORIZONTAL:   return "═";
            case BOX_VERTICAL:     return "║";
            case BOX_CROSS:        return "╬";
            case BOX_T_DOWN:       return "╦";
            case BOX_T_UP:         return "╩";
            case BOX_T_RIGHT:      return "╠";
            case BOX_T_LEFT:       return "╣";
            default:               return "?";
        }
    } else {
        // Use ASCII-safe characters for old console host
        switch (char_type) {
            case BOX_TOP_LEFT:     return "+";
            case BOX_TOP_RIGHT:    return "+";
            case BOX_BOTTOM_LEFT:  return "+";
            case BOX_BOTTOM_RIGHT: return "+";
            case BOX_HORIZONTAL:   return "-";
            case BOX_VERTICAL:     return "|";
            case BOX_CROSS:        return "+";
            case BOX_T_DOWN:       return "+";
            case BOX_T_UP:         return "+";
            case BOX_T_RIGHT:      return "+";
            case BOX_T_LEFT:       return "+";
            default:               return "?";
        }
    }
}

// Print a horizontal line with appropriate characters
void print_box_line(int width, BoxLineType type) {
    const char* left, *middle, *right;
    
    switch (type) {
        case BOX_LINE_TOP:
            left = get_box_char(BOX_TOP_LEFT);
            middle = get_box_char(BOX_HORIZONTAL);
            right = get_box_char(BOX_TOP_RIGHT);
            break;
        case BOX_LINE_BOTTOM:
            left = get_box_char(BOX_BOTTOM_LEFT);
            middle = get_box_char(BOX_HORIZONTAL);
            right = get_box_char(BOX_BOTTOM_RIGHT);
            break;
        case BOX_LINE_MIDDLE:
            left = get_box_char(BOX_T_RIGHT);
            middle = get_box_char(BOX_HORIZONTAL);
            right = get_box_char(BOX_T_LEFT);
            break;
        default:
            left = middle = right = get_box_char(BOX_HORIZONTAL);
            break;
    }
    
    printf("%s", left);
    for (int i = 0; i < width - 2; i++) {
        printf("%s", middle);
    }
    printf("%s\n", right);
}

// Print terminal compatibility information
void print_terminal_info(void) {
    int is_wt = detect_windows_terminal();
    
    printf("\n=== Terminal Compatibility Information ===\n");
    printf("Terminal Type: %s\n", is_wt ? "Windows Terminal (Modern)" : "Console Host (Legacy)");
    printf("UTF-8 Support: %s\n", is_wt ? "Full" : "Limited");
    printf("Box Drawing: %s\n", is_wt ? "Unicode" : "ASCII Fallback");
    
    if (!is_wt) {
        printf("\nFor better visual experience, install Windows Terminal:\n");
        printf("- Microsoft Store: Search 'Windows Terminal'\n");
        printf("- Or run: winget install Microsoft.WindowsTerminal\n");
        printf("- GitHub: https://github.com/microsoft/terminal\n");
    }
    
    printf("\nTesting box drawing characters:\n");
    print_box_line(50, BOX_LINE_TOP);
    printf("%s Terminal Test - ZoraVM Box Drawing Test %s\n", 
           get_box_char(BOX_VERTICAL), get_box_char(BOX_VERTICAL));
    print_box_line(50, BOX_LINE_BOTTOM);
    printf("\n");
}
