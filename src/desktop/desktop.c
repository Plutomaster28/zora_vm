#include "desktop/desktop.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char theme_root_vm[256] = "/etc/themes/cde";  // default
static char current_theme_name[64] = "cde";
static char prefs_path_vm[256] = "/home/.zora-desktop";
static char prefs_file_vm[256] = "/home/.zora-desktop/config";

int desktop_set_theme_root(const char* vm_path) {
    if (!vm_path) return -1;
    strncpy(theme_root_vm, vm_path, sizeof(theme_root_vm)-1);
    theme_root_vm[sizeof(theme_root_vm)-1] = '\0';
    return 0;
}

const char* desktop_get_theme_gtkrc_vm(void) {
    // Derive gtkrc relative to theme_root_vm
    static char path[256];
    snprintf(path, sizeof(path), "%s/gtk-2.0/gtkrc", theme_root_vm);
    return path;
}

const char* desktop_current_theme(void) { return current_theme_name; }

static int ensure_desktop_dirs(void) {
    vfs_create_directory("/home");
    vfs_create_directory(prefs_path_vm); // ensure container dir
    return 0;
}

// Forward decl
int desktop_switch_theme(const char* theme_name);

// Persistence helpers
static void prefs_append_line(const char* line) {
    VNode* cfg = vfs_find_node(prefs_file_vm);
    if (!cfg) {
        vfs_create_file(prefs_file_vm);
        cfg = vfs_find_node(prefs_file_vm);
    }
    if (!cfg) return;
    size_t old = cfg->data ? cfg->size : 0;
    size_t add = strlen(line);
    char* newbuf = malloc(old + add + 1);
    if (old && cfg->data) memcpy(newbuf, cfg->data, old);
    memcpy(newbuf + old, line, add);
    newbuf[old+add] = '\0';
    if (cfg->data) free(cfg->data);
    cfg->data = newbuf;
    cfg->size = old + add;
}

int desktop_load_prefs(void) {
    ensure_desktop_dirs();
    VNode* cfg = vfs_find_node(prefs_file_vm);
    if (cfg && cfg->data) {
        char* text = (char*)cfg->data;
        // Make a copy for tokenization to avoid modifying original until we re-save
        char* copy = strdup(text);
        char* line = strtok(copy, "\n");
        while (line) {
            if (strncmp(line, "theme=", 6) == 0) {
                desktop_switch_theme(line + 6);
            }
            line = strtok(NULL, "\n");
        }
        free(copy);
    }
    return 0;
}

int desktop_save_prefs(void) {
    // Rewrite config
    VNode* cfg = vfs_find_node(prefs_file_vm);
    if (cfg && cfg->data) { free(cfg->data); cfg->data = NULL; cfg->size = 0; }
    char line[128];
    snprintf(line, sizeof(line), "theme=%s\n", current_theme_name);
    prefs_append_line(line);
    return 0;
}

int desktop_record_window(int id, int x, int y, int w, int h, const char* title) {
    char line[256];
    snprintf(line, sizeof(line), "WIN %d %d %d %d %d %s\n", id, x, y, w, h, title?title:"");
    prefs_append_line(line);
    return 0;
}

int desktop_apply_theme(void) {
    VNode* gtkrc = vfs_find_node(desktop_get_theme_gtkrc_vm());
    if (!gtkrc) {
        printf("Desktop: gtkrc not found at %s\n", desktop_get_theme_gtkrc_vm());
        return -1;
    }
    if (gtkrc->host_path) {
    /* Cross-platform environment variable set */
#if defined(_WIN32) || defined(PLATFORM_WINDOWS)
    _putenv_s("GTK2_RC_FILES", gtkrc->host_path);
#else
    setenv("GTK2_RC_FILES", gtkrc->host_path, 1);
#endif
    printf("Desktop: GTK2_RC_FILES=%s\n", gtkrc->host_path);
    }
    printf("Desktop: Theme applied (%s)\n", current_theme_name);
    return 0;
}

int desktop_switch_theme(const char* theme_name) {
    if (!theme_name || !*theme_name) return -1;
    char new_root[256];
    snprintf(new_root, sizeof(new_root), "/etc/themes/%s", theme_name);
    VNode* node = vfs_find_node(new_root);
    if (!node || !node->is_directory) {
        printf("Desktop: Theme '%s' not found under /etc/themes\n", theme_name);
        return -1;
    }
    strncpy(current_theme_name, theme_name, sizeof(current_theme_name)-1);
    current_theme_name[sizeof(current_theme_name)-1] = '\0';
    desktop_set_theme_root(new_root);
    desktop_apply_theme();
    desktop_save_prefs();
    return 0;
}

int desktop_init(void) {
    printf("Desktop subsystem initializing...\n");
    desktop_load_prefs();
    desktop_apply_theme();
    return 0;
}

void desktop_shutdown(void) {
    desktop_save_prefs();
    printf("Desktop subsystem shutdown.\n");
}

int desktop_create_window(const char* title, int width, int height) {
    static int next_id = 1;
    int id = next_id++;
    printf("[UI] create_window id=%d title='%s' %dx%d (placeholder)\n", id, title?title:"", width, height);
    desktop_record_window(id, 10, 10, width, height, title);
    return id;
}

int desktop_add_label(int window_id, const char* text) {
    printf("[UI] window %d add_label '%s'\n", window_id, text?text:"");
    return 0;
}

int desktop_show_window(int window_id) {
    printf("[UI] show_window %d\n", window_id);
    return 0;
}

int desktop_run_loop(void) {
    printf("[UI] entering desktop run loop (placeholder)\n");
    return 0;
}

int desktop_restart(void) {
    printf("Desktop: Restarting...\n");
    desktop_shutdown();
    desktop_init();
    return 0;
}

int desktop_list_themes(void) {
    VNode* themes = vfs_find_node("/etc/themes");
    if (!themes) { printf("/etc/themes missing\n"); return -1; }
    printf("Available themes:\n");
    VNode* child = themes->children;
    while (child) {
        if (child->is_directory) {
            printf("  %s%s\n", child->name, strcmp(child->name,current_theme_name)==0?" (current)":"");
        }
        child = child->next;
    }
    return 0;
}

int desktop_script_bootstrap(void) {
    desktop_init();
    return 0;
}
