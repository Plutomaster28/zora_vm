#ifndef VFS_H
#define VFS_H

#include <stdio.h>
#include <stddef.h>
#include <time.h>

// Unix-style permission constants
#define VFS_S_IRUSR 0400    // Owner read permission
#define VFS_S_IWUSR 0200    // Owner write permission
#define VFS_S_IXUSR 0100    // Owner execute permission
#define VFS_S_IRGRP 0040    // Group read permission
#define VFS_S_IWGRP 0020    // Group write permission
#define VFS_S_IXGRP 0010    // Group execute permission
#define VFS_S_IROTH 0004    // Others read permission
#define VFS_S_IWOTH 0002    // Others write permission
#define VFS_S_IXOTH 0001    // Others execute permission

// Common permission combinations
#define VFS_S_IRWXU (VFS_S_IRUSR | VFS_S_IWUSR | VFS_S_IXUSR)  // 700
#define VFS_S_IRWXG (VFS_S_IRGRP | VFS_S_IWGRP | VFS_S_IXGRP)  // 070
#define VFS_S_IRWXO (VFS_S_IROTH | VFS_S_IWOTH | VFS_S_IXOTH)  // 007

// Default permissions
#define VFS_DEFAULT_FILE_PERMS  0644  // rw-r--r--
#define VFS_DEFAULT_DIR_PERMS   0755  // rwxr-xr-x
#define VFS_ROOT_ONLY_PERMS     0600  // rw-------

// Forward declarations
typedef struct VNode VNode;
typedef struct VirtualFS VirtualFS;

// VFS Node structure with Unix-style permissions
struct VNode {
    char name[256];
    int is_directory;
    size_t size;
    void* data;
    char* host_path;
    VNode* parent;
    VNode* children;
    VNode* next;
    
    // Unix-style permissions and ownership
    unsigned int mode;           // File permissions (rwx for owner/group/others)
    char owner[50];             // Username of owner
    char group[50];             // Group name
    time_t created_time;        // Creation time
    time_t modified_time;       // Last modification time
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
int vfs_write_file(const char* path, const void* data, size_t size);
int vfs_read_file(const char* path, void** data, size_t* size);

// Node operations
VNode* vfs_find_node(const char* path);
VNode* vfs_create_directory_node(const char* name);  // NEW: Returns VNode*
VNode* vfs_create_file_node(const char* name);       // NEW: Returns VNode*
void vfs_add_child(VNode* parent, VNode* child);     // NEW: Add child function
int vfs_load_file_content(VNode* node);              // NEW: Load file content on-demand

// Host directory operations
void vfs_load_host_directory(VNode* vm_node, const char* host_path);
void vfs_refresh_directory(VNode* vm_node);             // NEW: Refresh directory from host
int vfs_mount_persistent(const char* vm_path, const char* host_path);

// Utility functions
int create_directory_recursive(const char* path);    // NEW: Declare this function

// Permission and ownership functions
int vfs_chmod(const char* path, unsigned int mode);
int vfs_chown(const char* path, const char* owner, const char* group);
int vfs_check_permission(const char* path, const char* user, int required_perms);
int vfs_set_default_permissions(VNode* node, const char* owner, const char* group);
void vfs_format_permissions(unsigned int mode, char* output);
int vfs_parse_permissions(const char* perm_str);

// User context for permission checking
extern char vfs_current_user[50];
extern char vfs_current_group[50];
extern int vfs_is_root;

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
void vfs_sync_all(void);
int vm_system(const char* command);  // Add this for shell.c

// Root host mapping (treat a host directory tree as "/")
int vfs_mount_root_directories(const char* host_root, const char* const* dirs, size_t count);
int vfs_set_host_root(const char* host_root); // Optionally remember a host root (future use)
int vfs_mount_root_autodiscover(const char* host_root); // Scan host_root and mount all first-level dirs

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

// VFS to host path conversion
char* vfs_get_host_path_from_vfs_path(const char* vfs_path);

// Live file synchronization
int vfs_sync_from_host(void);           // Sync changes from host to VFS (silent)
int vfs_sync_from_host_verbose(void);   // Sync changes from host to VFS (with output)
int vfs_start_live_sync(void);          // Start background file monitoring
void vfs_stop_live_sync(void);          // Stop background file monitoring
int vfs_is_live_sync_enabled(void);     // Check if live sync is active

#endif // VFS_H