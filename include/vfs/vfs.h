#ifndef VFS_H
#define VFS_H

#include <stdio.h>
#include <stddef.h>

// Forward declarations
typedef struct VNode VNode;
typedef struct VirtualFS VirtualFS;

// VFS Node structure
struct VNode {
    char name[256];
    int is_directory;
    int is_persistent;
    size_t size;
    void* data;
    char* host_path;
    VNode* parent;
    VNode* children;
    VNode* next;
};

// Virtual filesystem structure
struct VirtualFS {
    VNode* root;
    VNode* current_dir;
};

// VFS Core functions
int vfs_init(void);
void vfs_cleanup(void);
VirtualFS* vfs_get_instance(void);

// Directory operations
int vfs_mkdir(const char* path);
int vfs_rmdir(const char* path);
int vfs_chdir(const char* path);
char* vfs_getcwd(void);

// File operations
int vfs_create_file(const char* path);
int vfs_delete_file(const char* path);

// Node operations
VNode* vfs_find_node(const char* path);
VNode* vfs_create_directory_node(const char* name);  // NEW: Returns VNode*
VNode* vfs_create_file_node(const char* name);       // NEW: Returns VNode*
void vfs_add_child(VNode* parent, VNode* child);     // NEW: Add child function

// Persistent directory operations - FIXED SIGNATURE
void vfs_load_persistent_directory(VNode* vm_node, const char* host_path);
int vfs_mount_persistent(const char* vm_path, const char* host_path);

// Utility functions
int create_directory_recursive(const char* path);    // NEW: Declare this function

// Virtual syscalls
char* vm_getcwd(void);
int vm_chdir(const char* path);
int vm_mkdir(const char* path);
int vm_rmdir(const char* path);
int vm_remove(const char* filename);
FILE* vm_fopen(const char* filename, const char* mode);

// Virtual commands
int vm_ls(void);
int vm_clear(void);
int vm_pwd(void);
int vm_ps(void);

// Add these missing function declarations:
int vfs_create_directory(const char* path);
void vfs_sync_all_persistent(void);
int vm_system(const char* command);  // Add this for shell.c

// Add other missing vm_ function declarations:
int vm_clear(void);
int vm_pwd(void);
int vm_ps(void);
char* vm_getcwd(void);
int vm_chdir(const char* path);
int vm_mkdir(const char* path);
int vm_rmdir(const char* path);
int vm_remove(const char* filename);
int vm_ls(void);
FILE* vm_fopen(const char* filename, const char* mode);

#endif // VFS_H