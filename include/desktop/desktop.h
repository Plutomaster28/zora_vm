#ifndef ZORA_DESKTOP_H
#define ZORA_DESKTOP_H

#include <stddef.h>

// Initialize desktop subsystem (theme load, prefs)
int desktop_init(void);
void desktop_shutdown(void);

// Theme handling
int desktop_set_theme_root(const char* vm_path);             // e.g. /etc/themes/cde
int desktop_apply_theme(void);                               // Apply current theme (sets env vars)
const char* desktop_get_theme_gtkrc_vm(void);                // /etc/themes/cde/gtk-2.0/gtkrc
int desktop_switch_theme(const char* theme_name);            // Switch to another theme under /etc/themes
const char* desktop_current_theme(void);

// Preferences persistence
int desktop_load_prefs(void);                                // Loads /home/.zora-desktop/config if exists
int desktop_save_prefs(void);                                // Saves current prefs

// Simplified UI API (placeholder until real GTK binding)
int desktop_create_window(const char* title, int width, int height);
int desktop_add_label(int window_id, const char* text);
int desktop_show_window(int window_id);
int desktop_run_loop(void);                                  // Placeholder event loop
int desktop_restart(void);                                   // Shutdown + init

// Window persistence
int desktop_record_window(int id, int x, int y, int w, int h, const char* title);

// Theme enumeration
int desktop_list_themes(void);                               // Print available theme names

// Scripting integration helpers
int desktop_script_bootstrap(void);                          // Called before launching desktop.pl

#endif // ZORA_DESKTOP_H
