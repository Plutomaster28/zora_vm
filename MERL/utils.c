#include "utils.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  // Add missing include
#include <windows.h>

void print_colored(const char* text, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);
    SetConsoleTextAttribute(hConsole, 7); // Reset to default
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