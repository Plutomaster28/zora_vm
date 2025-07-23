#include "utils.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  // Add missing include

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <sys/timeb.h>
#else
    #include <sys/time.h>
#endif

void print_colored(const char* text, int color) {
#ifdef PLATFORM_WINDOWS
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);
    SetConsoleTextAttribute(hConsole, 7); // Reset to default
#else
    // Use ANSI colors on Linux
    const char* ansi_colors[] = {
        "\033[30m", // Black
        "\033[34m", // Blue  
        "\033[32m", // Green
        "\033[36m", // Cyan
        "\033[31m", // Red
        "\033[35m", // Magenta
        "\033[33m", // Yellow
        "\033[37m"  // White
    };
    
    if (color >= 0 && color < 8) {
        printf("%s%s\033[0m", ansi_colors[color], text);
    } else {
        printf("%s", text);
    }
#endif
}

char* trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    return str;
}

// ... rest of utils.c