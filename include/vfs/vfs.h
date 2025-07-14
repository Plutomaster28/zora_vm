#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// Virtual file system node
typedef struct VNode {
    char name[256];
    int is_directory;
    int is_persistent;        // NEW: Flag for persistent storage
    char* host_path;          // NEW: Real filesystem path
    size_t size;
    void* data;
    struct VNode* parent;
    struct VNode* children;
    struct VNode* next;
} VNode;

// Virtual file system
typedef struct {
    VNode* root;
    VNode* current_dir;
} VirtualFS;

// VFS functions
int vfs_init(void);
void vfs_cleanup(void);
VNode* vfs_find_node(const char* path);
int vfs_mkdir(const char* path);
int vfs_rmdir(const char* path);
int vfs_create_file(const char* path);
int vfs_delete_file(const char* path);

// Add persistent filesystem functions
int vfs_mount_persistent(const char* vm_path, const char* host_path);
int vfs_sync_persistent_node(VNode* node);
int vfs_load_persistent_directory(const char* vm_path, const char* host_path);
int vfs_create_persistent_file(const char* vm_path, const char* host_path);
int vfs_sync_all_persistent(void);

// Virtual system call replacements
char* vm_getcwd(void);
int vm_chdir(const char* path);
int vm_mkdir(const char* path);
int vm_rmdir(const char* path);
int vm_system(const char* command);
FILE* vm_fopen(const char* filename, const char* mode);
int vm_remove(const char* filename);

// Virtual command implementations
int vm_ls(void);
int vm_clear(void);
int vm_pwd(void);
int vm_ps(void);  // Add this declaration

// Getter for VFS instance
VirtualFS* vfs_get_instance(void);

#endif // VFS_H