#ifndef PLATFORM_H
#define PLATFORM_H

// Windows-only platform header
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

// Common includes
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

// Windows function declarations
#define SAFE_SPRINTF sprintf_s

#endif // PLATFORM_H