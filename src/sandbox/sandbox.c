#include "sandbox.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>

#include "platform/platform.h"

// Only include the platform-specific implementation, don't define functions here
#ifdef PLATFORM_WINDOWS
    #include "windows/sandbox_win.c"
#elif defined(PLATFORM_LINUX)
    #include "linux/sandbox_linux.c"
#else
    #error "Unsupported platform for sandbox"
#endif

// No other code here - the platform files contain all implementations

