#ifndef CONFIG_H
#define CONFIG_H

// System configuration constants
#define FIRMWARE_VERSION "0.0.4"
#define TOTAL_MEMORY_MB 500
#define BIOS "Droplet"

// Additional compile-time configuration
#define OS_VERSION "MERL v0.1"
#define ENABLE_KAIROS 1 //might go unused
#define MAX_USERS 8
#define TETRA_REPO_LOCATION "./tetra_repos"
#define SYSTEM_NAME "Micreon-16"

#include "crash.h"

// Redirect standard memory allocation functions to safe versions
#define malloc(size) safe_malloc(size)
#define calloc(num, size) safe_calloc(num, size)
#define realloc(ptr, size) safe_realloc(ptr, size)

#endif // CONFIG_H
