#ifndef UNIX_CORE_H
#define UNIX_CORE_H

// Research UNIX Tenth Edition Core System
// Provides complete UNIX environment within ZoraVM

#include "unix_core/unix_directories.h"
#include "unix_core/unix_compiler.h"
#include "unix_core/unix_ipc.h"
#include "unix_core/unix_textproc.h"
#include "unix_core/unix_games.h"

// Main initialization function for the entire UNIX core
int unix_core_init(void);

// Main cleanup function for the entire UNIX core
void unix_core_cleanup(void);

// Show UNIX system information
void unix_show_system_info(void);

#endif // UNIX_CORE_H
