#include "platform/platform.h"

#ifdef PLATFORM_WINDOWS
    #include "windows/syscall_filter_win.c"
#elif defined(PLATFORM_LINUX)
    #include "linux/syscall_filter_linux.c"
#else
    #error "Unsupported platform for syscall filtering"
#endif

// No other code here

