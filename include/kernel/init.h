#ifndef INIT_H
#define INIT_H

// ZoraVM Init System - provides OS-like boot experience

void init_system_start(void);
void init_display_progress(const char* service, int stage, int total);
void init_start_services(void);
void init_display_boot_logo(void);
int init_get_boot_stage(void);

#endif // INIT_H
