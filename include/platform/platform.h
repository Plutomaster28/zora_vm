#ifndef PLATFORM_H
#define PLATFORM_H

// Platform detection - use #ifdef for cleaner logic
#ifdef _WIN32
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include <time.h>
    
    #ifndef MAX_PATH
        #define MAX_PATH 260
    #endif
    
    // Windows-specific types
    typedef HANDLE ProcessHandle;
    typedef DWORD ProcessId;
    typedef DWORD ThreadReturn;
    typedef LPVOID ThreadParam;
    
    // Windows-specific macros
    #define MKDIR(path) _mkdir(path)
    #define PATH_SEP "\\"
    #define THREAD_CALL WINAPI
    
#elif defined(__linux__)
    #ifndef PLATFORM_LINUX
        #define PLATFORM_LINUX
    #endif
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <time.h>
    #include <limits.h>
    #include <pthread.h>
    
    // Linux compatibility types for Windows APIs
    typedef void* HANDLE;
    typedef unsigned int DWORD;
    typedef int BOOL;
    typedef pid_t ProcessHandle;
    typedef pid_t ProcessId;
    typedef void* ThreadReturn;
    typedef void* ThreadParam;
    
    // Linux-specific macros
    #define MKDIR(path) mkdir(path, 0755)
    #define PATH_SEP "/"
    #define MAX_PATH PATH_MAX
    #define THREAD_CALL
    #define TRUE 1
    #define FALSE 0
    #define INVALID_HANDLE_VALUE ((HANDLE)(long)-1)
    
#else
    #error "Unsupported platform - only Windows and Linux are supported"
#endif

// Common includes for all platforms
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

// Cross-platform function declarations
#ifdef PLATFORM_WINDOWS
    // Windows-specific function declarations
    #define SAFE_SPRINTF sprintf_s
#else
    // Linux-specific function declarations  
    #define SAFE_SPRINTF snprintf
#endif

#endif // PLATFORM_H