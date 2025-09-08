#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "platform/platform.h"

// Debug control - set to 0 to disable verbose debug output
#define VFS_DEBUG_VERBOSE 0

// Windows-specific directory handling
#include <windows.h>
#include <direct.h>

// Virtual file system that stays in memory
static VirtualFS* vm_fs = NULL;
static char current_directory[256] = "/";
static char host_root_directory[512] = {0};

// NEW: Write-through helper function declarations
static char* vfs_get_host_path(VNode* node);
static int vfs_ensure_host_directory(const char* host_path);
static int vfs_sync_to_host(VNode* node);

// Create directory node
VNode* vfs_create_directory_node(const char* name) {
    VNode* node = malloc(sizeof(VNode));
    if (!node) return NULL;
    
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->is_directory = 1;
    node->is_persistent = 0;  // Will be removed eventually
    node->size = 0;
    node->data = NULL;
    node->host_path = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;
    
    return node;
}

// Create file node
VNode* vfs_create_file_node(const char* name) {
    VNode* node = malloc(sizeof(VNode));
    if (!node) return NULL;
    
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->is_directory = 0;
    node->is_persistent = 0;  // Will be removed eventually
    node->size = 0;
    node->data = NULL;
    node->host_path = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;
    
    return node;
}

// Add child to parent
void vfs_add_child(VNode* parent, VNode* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    child->next = parent->children;
    parent->children = child;
}

// Load file content from host filesystem (on-demand)
int vfs_load_file_content(VNode* node) {
    if (!node || node->is_directory || !node->host_path) {
        return -1;
    }
    
    // Already loaded
    if (node->data) {
        return 0;
    }
    
    FILE* f = fopen(node->host_path, "rb");
    if (!f) {
        printf("VFS: Could not open file: %s\n", node->host_path);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0 || size > 16*1024*1024) { // Limit to 16MB
        printf("VFS: Invalid file size for: %s (size: %ld)\n", node->host_path, size);
        fclose(f);
        return -1;
    }
    
    fseek(f, 0, SEEK_SET);
    
    // Allocate memory for content plus null terminator
    node->data = malloc(size + 1);
    if (!node->data) {
        printf("VFS: Memory allocation failed for: %s\n", node->host_path);
        fclose(f);
        return -1;
    }
    
    size_t read_size = 0;
    if (size > 0) {
        read_size = fread(node->data, 1, size, f);
    }
    fclose(f);
    
    if (read_size != (size_t)size) {
        printf("VFS: File read error for: %s (expected: %ld, read: %zu)\n", 
               node->host_path, size, read_size);
        free(node->data);
        node->data = NULL;
        return -1;
    }
    
    // Null-terminate for text files
    ((char*)node->data)[size] = '\0';
    node->size = size;
    
    printf("VFS: Loaded file content: %s (%zu bytes)\n", node->name, node->size);
    return 0;
}

// NEW: Create directory recursively
int create_directory_recursive(const char* path) {
    char temp_path[MAX_PATH];
    strncpy(temp_path, path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';
    
    char* ptr = temp_path;
    while (*ptr) {
        if (*ptr == '/' || *ptr == '\\') {
            *ptr = '\0';
            
            // Check if directory exists
            DWORD attrib = GetFileAttributesA(temp_path);
            if (attrib == INVALID_FILE_ATTRIBUTES) {
                if (CreateDirectoryA(temp_path, NULL) == 0) {
                    DWORD error = GetLastError();
                    if (error != ERROR_ALREADY_EXISTS) {
                        printf("Failed to create directory: %s (error: %lu)\n", temp_path, error);
                        return -1;
                    }
                }
            }
            *ptr = '/';
        }
        ptr++;
    }
    
    // Create the final directory
    DWORD attrib = GetFileAttributesA(temp_path);
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA(temp_path, NULL) == 0) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                printf("Failed to create final directory: %s (error: %lu)\n", temp_path, error);
                return -1;
            }
        }
    }
    
    return 0;
}

// NEW: Get the full host filesystem path for a VFS node
static char* vfs_get_host_path(VNode* node) {
    if (!node || strlen(host_root_directory) == 0) return NULL;
    
    // Build path by walking up the tree
    static char path_buffer[1024];
    char temp_path[1024];
    path_buffer[0] = '\0';
    temp_path[0] = '\0';
    
    VNode* current = node;
    while (current && current->parent) {
        if (strlen(temp_path) > 0) {
            snprintf(path_buffer, sizeof(path_buffer), "%s%c%s", current->name, 
                     '\\',
                     temp_path);
        } else {
            strncpy(path_buffer, current->name, sizeof(path_buffer) - 1);
        }
        strncpy(temp_path, path_buffer, sizeof(temp_path) - 1);
        current = current->parent;
    }
    
    // Construct full host path
    static char full_path[1024];
    if (strlen(path_buffer) > 0) {
        snprintf(full_path, sizeof(full_path), "%s%c%s", host_root_directory,
                 '\\',
                 path_buffer);
    } else {
        strncpy(full_path, host_root_directory, sizeof(full_path) - 1);
    }
    
    return full_path;
}

// NEW: Ensure host directory exists (create if needed)
static int vfs_ensure_host_directory(const char* host_path) {
    if (!host_path) return -1;
    
    DWORD attrib = GetFileAttributesA(host_path);
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        return CreateDirectoryA(host_path, NULL) ? 0 : -1;
    }
    return (attrib & FILE_ATTRIBUTE_DIRECTORY) ? 0 : -1;
}

// NEW: Sync VFS node to host filesystem
static int vfs_sync_to_host(VNode* node) {
    if (!node || strlen(host_root_directory) == 0) return -1;
    
    char* host_path = vfs_get_host_path(node);
    if (!host_path) return -1;
    
    if (node->is_directory) {
        // Create directory on host
        return vfs_ensure_host_directory(host_path);
    } else {
        // Create/update file on host
        FILE* host_file = fopen(host_path, "wb");
        if (!host_file) return -1;
        
        if (node->data && node->size > 0) {
            fwrite(node->data, 1, node->size, host_file);
        }
        fclose(host_file);
        return 0;
    }
}

int vfs_chdir(const char* path) {
    if (!path) return -1;
    
    char new_path[256];
    
    // Handle absolute paths
    if (path[0] == '/') {
        strncpy(new_path, path, sizeof(new_path) - 1);
    } else {
        // Handle relative paths
        if (strcmp(current_directory, "/") == 0) {
            snprintf(new_path, sizeof(new_path), "/%s", path);
        } else {
            snprintf(new_path, sizeof(new_path), "%s/%s", current_directory, path);
        }
    }
    new_path[sizeof(new_path) - 1] = '\0';
    
    // Check if the directory exists
    VNode* target_dir = vfs_find_node(new_path);
    if (target_dir && target_dir->is_directory) {
        strncpy(current_directory, new_path, sizeof(current_directory) - 1);
        current_directory[sizeof(current_directory) - 1] = '\0';
        return 0;
    }
    
    return -1; // Directory not found
}

char* vfs_getcwd(void) {
    return current_directory;
}

int vfs_init(void) {
    if (vm_fs) {
        return 0; // Already initialized
    }
    
    vm_fs = malloc(sizeof(VirtualFS));
    if (!vm_fs) {
        return -1;
    }
    
    // Create root directory
    vm_fs->root = malloc(sizeof(VNode));
    if (!vm_fs->root) {
        free(vm_fs);
        vm_fs = NULL;
        return -1;
    }
    
    strcpy(vm_fs->root->name, "/");
    vm_fs->root->is_directory = 1;
    vm_fs->root->is_persistent = 0;
    vm_fs->root->size = 0;
    vm_fs->root->data = NULL;
    vm_fs->root->host_path = NULL;
    vm_fs->root->parent = NULL;
    vm_fs->root->children = NULL;
    vm_fs->root->next = NULL;
    
    vm_fs->current_dir = vm_fs->root;
    
    // Create basic directory structure
    vfs_mkdir("/bin");
    vfs_mkdir("/home");
    vfs_mkdir("/tmp");
    vfs_mkdir("/etc");
    vfs_mkdir("/usr");
    vfs_mkdir("/var");
    
    // Create some basic files
    vfs_create_file("/etc/hosts");
    vfs_create_file("/etc/passwd");
    vfs_create_file("/home/readme.txt");
    
#if ZORA_VERBOSE_BOOT
    printf("Virtual filesystem initialized\n");
#endif
    return 0;
}

void vfs_cleanup_node(VNode* node) {
    if (!node) return;
    
    // Clean up children recursively
    VNode* child = node->children;
    while (child) {
        VNode* next = child->next;
        vfs_cleanup_node(child);
        child = next;
    }
    
    // Free data if it's a file
    if (node->data) {
        free(node->data);
    }
    
    if (node->host_path) {
        free(node->host_path);
    }
    
    free(node);
}

void vfs_cleanup(void) {
    if (vm_fs) {
        vfs_cleanup_node(vm_fs->root);
        free(vm_fs);
        vm_fs = NULL;
        printf("Virtual filesystem cleaned up\n");
    }
}

VNode* vfs_find_node(const char* path) {
    if (!vm_fs || !path) return NULL;
    
    if (strcmp(path, "/") == 0) {
        return vm_fs->root;
    }
    
    // Simple path resolution - start from root
    VNode* current = vm_fs->root;
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    // Skip leading slash
    char* token = strtok(path_copy[0] == '/' ? path_copy + 1 : path_copy, "/");
    
    while (token && current) {
        VNode* child = current->children;
        current = NULL; // Reset to indicate not found
        
        while (child) {
            if (strcmp(child->name, token) == 0) {
                current = child;
                break;
            }
            child = child->next;
        }
        
        token = strtok(NULL, "/");
    }
    
    return current;
}

int vfs_mkdir(const char* path) {
    if (!vm_fs || !path) return -1;
    
    // Find parent directory
    char parent_path[256];
    char dir_name[256];
    
    // Extract directory name and parent path
    const char* last_slash = strrchr(path, '/');
    if (last_slash) {
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        strcpy(dir_name, last_slash + 1);
    } else {
        strcpy(parent_path, "/");
        strcpy(dir_name, path);
    }
    
    if (strlen(parent_path) == 0) {
        strcpy(parent_path, "/");
    }
    
    VNode* parent = vfs_find_node(parent_path);
    if (!parent || !parent->is_directory) {
        return -1;
    }
    
    // Check if directory already exists
    VNode* existing = parent->children;
    while (existing) {
        if (strcmp(existing->name, dir_name) == 0) {
            return 0; // Already exists
        }
        existing = existing->next;
    }
    
    // Create new directory node
    VNode* new_dir = vfs_create_directory_node(dir_name);
    if (!new_dir) return -1;

    vfs_add_child(parent, new_dir);
    
    // NEW: Write-through to host filesystem
    if (strlen(host_root_directory) > 0) {
        vfs_sync_to_host(new_dir);
    }
    
    return 0;
}

int vfs_create_file(const char* path) {
    if (!vm_fs || !path) return -1;
    
    // Find parent directory
    char parent_path[256];
    char file_name[256];
    
    const char* last_slash = strrchr(path, '/');
    if (last_slash) {
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        strcpy(file_name, last_slash + 1);
    } else {
        strcpy(parent_path, "/");
        strcpy(file_name, path);
    }
    
    if (strlen(parent_path) == 0) {
        strcpy(parent_path, "/");
    }
    
    VNode* parent = vfs_find_node(parent_path);
    if (!parent || !parent->is_directory) {
        return -1;
    }
    
    // Create new file node
    VNode* new_file = vfs_create_file_node(file_name);
    if (!new_file) return -1;
    
    vfs_add_child(parent, new_file);
    
    // NEW: Write-through to host filesystem
    if (strlen(host_root_directory) > 0) {
        vfs_sync_to_host(new_file);
    }
    
    return 0;
}

// Refresh directory by checking for new files in host filesystem
void vfs_refresh_directory(VNode* vm_node) {
    if (!vm_node || !vm_node->is_directory || !vm_node->host_path) {
        return;
    }
    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Refreshing directory: %s from %s\n", vm_node->name, vm_node->host_path);
    
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", vm_node->host_path);
    
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (VFS_DEBUG_VERBOSE) printf("DEBUG: Failed to refresh directory: %s\n", vm_node->host_path);
        return;
    }
    
    do {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        // Check if this file/directory already exists in VM
        VNode* existing = vm_node->children;
        bool found = false;
        while (existing) {
            if (strcmp(existing->name, find_data.cFileName) == 0) {
                found = true;
                break;
            }
            existing = existing->next;
        }
        
        if (!found) {
            // New file/directory found, add it
            if (VFS_DEBUG_VERBOSE) printf("DEBUG: Found new entry: %s\n", find_data.cFileName);
            char full_host_path[MAX_PATH];
            snprintf(full_host_path, sizeof(full_host_path), "%s\\%s", vm_node->host_path, find_data.cFileName);
            
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // New directory
                if (VFS_DEBUG_VERBOSE) printf("DEBUG: Adding new directory: %s\n", find_data.cFileName);
                VNode* dir_node = vfs_create_directory_node(find_data.cFileName);
                if (dir_node) {
                    vfs_add_child(vm_node, dir_node);
                    vfs_load_host_directory(dir_node, full_host_path);
                }
            } else {
                // New file
                if (VFS_DEBUG_VERBOSE) printf("DEBUG: Adding new file: %s\n", find_data.cFileName);
                VNode* file_node = vfs_create_file_node(find_data.cFileName);
                if (file_node) {
                    file_node->host_path = malloc(strlen(full_host_path) + 1);
                    if (file_node->host_path) {
                        strcpy(file_node->host_path, full_host_path);
                    }
                    file_node->size = find_data.nFileSizeLow;
                    vfs_add_child(vm_node, file_node);
                    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Added new file: %s (size: %zu)\n", file_node->name, file_node->size);
                }
            }
        }
    } while (FindNextFileA(hFind, &find_data) != 0);
    
    FindClose(hFind);
    
    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Finished refreshing directory: %s\n", vm_node->name);
}

// Load directory contents from host filesystem
void vfs_load_host_directory(VNode* vm_node, const char* host_path) {
    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Loading host directory from %s to %s\n", host_path, vm_node->name);
    
    // Windows implementation using FindFirstFile/FindNextFile
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", host_path);
    
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (VFS_DEBUG_VERBOSE) printf("DEBUG: Failed to open directory: %s\n", host_path);
        return;
    }
    
    do {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        if (VFS_DEBUG_VERBOSE) printf("DEBUG: Found entry: %s\n", find_data.cFileName);
        
        // Build full paths
        char full_host_path[MAX_PATH];
        snprintf(full_host_path, sizeof(full_host_path), "%s\\%s", host_path, find_data.cFileName);
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // It's a directory
            if (VFS_DEBUG_VERBOSE) printf("DEBUG: Creating directory node: %s\n", find_data.cFileName);
            VNode* dir_node = vfs_create_directory_node(find_data.cFileName);
            if (dir_node) {
                vfs_add_child(vm_node, dir_node);
                // Recursive load
                vfs_load_host_directory(dir_node, full_host_path);
            }
        } else {
            // It's a file
            if (VFS_DEBUG_VERBOSE) printf("DEBUG: Creating file node: %s\n", find_data.cFileName);
            VNode* file_node = vfs_create_file_node(find_data.cFileName);
            if (file_node) {
                file_node->host_path = malloc(strlen(full_host_path) + 1);
                if (file_node->host_path) {
                    strcpy(file_node->host_path, full_host_path);
                }
                file_node->size = find_data.nFileSizeLow;
                // Note: content will be loaded on-demand via vfs_load_file_content()
                vfs_add_child(vm_node, file_node);
                if (VFS_DEBUG_VERBOSE) printf("DEBUG: Added file: %s (size: %zu)\n", file_node->name, file_node->size);
            }
        }
    } while (FindNextFileA(hFind, &find_data) != 0);
    
    FindClose(hFind);
    
    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Finished loading directory: %s\n", host_path);
}

// Mount host directory
int vfs_mount_persistent(const char* vm_path, const char* host_path) {
    if (VFS_DEBUG_VERBOSE) printf("DEBUG: Mounting host directory: %s -> %s\n", vm_path, host_path);
    
    // Find or create VM directory
    VNode* vm_node = vfs_find_node(vm_path);
    if (!vm_node) {
        // Create the directory
        if (vfs_mkdir(vm_path) != 0) {
            printf("Failed to create VM directory: %s\n", vm_path);
            return -1;
        }
        vm_node = vfs_find_node(vm_path);
    }
    
    if (!vm_node || !vm_node->is_directory) {
        printf("VM path is not a directory: %s\n", vm_path);
        return -1;
    }
    
    // Create the host directory if needed
    if (create_directory_recursive(host_path) != 0) {
        printf("Warning: Could not create host directory: %s\n", host_path);
    }
    
    // Load the directory contents
    vfs_load_host_directory(vm_node, host_path);
    return 0;
}

// Virtual system call implementations
char* vm_getcwd(void) {
    static char path[256];
    if (!vm_fs || !vm_fs->current_dir) {
        strcpy(path, "/");
        return path;
    }
    
    // Build path by traversing up to root
    VNode* current = vm_fs->current_dir;
    char temp_path[256] = "";
    
    while (current && current->parent) {
        char new_path[256];
        snprintf(new_path, sizeof(new_path), "/%s%s", current->name, temp_path);
        strcpy(temp_path, new_path);
        current = current->parent;
    }
    
    if (strlen(temp_path) == 0) {
        strcpy(path, "/");
    } else {
        strcpy(path, temp_path);
    }
    
    return path;
}

VirtualFS* vfs_get_instance(void) {
    return vm_fs;
}

int vm_ls(void) {
    if (!vm_fs || !vm_fs->current_dir) {
        return -1;
    }
    
    VNode* current = vm_fs->current_dir;
    VNode* child = current->children;
    
    printf("Virtual directory contents:\n");
    while (child) {
        printf("%-20s %s\n", child->name, child->is_directory ? "<DIR>" : "<FILE>");
        child = child->next;
    }
    
    return 0;
}

int vm_clear(void) {
    // Clear screen (this is safe)
    printf("\033[2J\033[H");
    fflush(stdout);
    return 0;
}

int vm_pwd(void) {
    char* cwd = vm_getcwd();
    printf("%s\n", cwd);
    return 0;
}

FILE* vm_fopen(const char* filename, const char* mode) {
    // For now, return NULL to indicate file operations are not supported
    // In a full implementation, this would create virtual file handles
    printf("vm_fopen: Virtual file operations not fully implemented\n");
    return NULL;
}

int vm_ps(void) {
    printf("VM Process List:\n");
    printf("  PID  CMD\n");
    printf("    1  vm_init\n");
    printf("    2  merl_shell\n");
    printf("    3  vm_kernel\n");
    return 0;
}

// Add other missing vm_ functions as needed...
int vm_chdir(const char* path) { return vfs_chdir(path); }
int vm_mkdir(const char* path) { return vfs_mkdir(path); }
int vm_rmdir(const char* path) { return vfs_rmdir(path); }
int vm_remove(const char* filename) { return vfs_delete_file(filename); }

// Add missing functions for rmdir and delete_file
int vfs_rmdir(const char* path) {
    VNode* node = vfs_find_node(path);
    if (!node || !node->is_directory || node->children) {
        return -1;
    }

    // NEW: Delete from host filesystem first
    if (strlen(host_root_directory) > 0) {
        char* host_path = vfs_get_host_path(node);
        if (host_path) {
            RemoveDirectoryA(host_path);
        }
    }

    // Remove from parent's children list
    VNode* parent = node->parent;
    if (parent) {
        VNode* prev = NULL;
        VNode* current = parent->children;
        
        while (current) {
            if (current == node) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    parent->children = current->next;
                }
                break;
            }
            prev = current;
            current = current->next;
        }
    }

    free(node);
    return 0;
}int vfs_delete_file(const char* path) {
    VNode* node = vfs_find_node(path);
    if (!node || node->is_directory) {
        return -1;
    }

    // NEW: Delete from host filesystem first
    if (strlen(host_root_directory) > 0) {
        char* host_path = vfs_get_host_path(node);
        if (host_path) {
            DeleteFileA(host_path);
        }
    }

    // Remove from parent's children list
    VNode* parent = node->parent;
    if (parent) {
        VNode* prev = NULL;
        VNode* current = parent->children;
        
        while (current) {
            if (current == node) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    parent->children = current->next;
                }
                break;
            }
            prev = current;
            current = current->next;
        }
    }

    if (node->data) {
        free(node->data);
    }
    if (node->host_path) {
        free(node->host_path);
    }
    free(node);
    return 0;
}int vfs_create_directory(const char* path) {
    return vfs_mkdir(path);
}

// NEW: Write data to a file in VFS and sync to host
int vfs_write_file(const char* path, const void* data, size_t size) {
    VNode* node = vfs_find_node(path);
    if (!node || node->is_directory) {
        return -1;
    }

    // Update VFS data
    if (node->data) {
        free(node->data);
    }
    
    if (size > 0 && data) {
        node->data = malloc(size);
        if (!node->data) return -1;
        memcpy(node->data, data, size);
        node->size = size;
    } else {
        node->data = NULL;
        node->size = 0;
    }

    // NEW: Write-through to host filesystem
    if (strlen(host_root_directory) > 0) {
        vfs_sync_to_host(node);
    }

    return 0;
}

// Read data from a file in VFS (with on-demand loading)
int vfs_read_file(const char* path, void** data, size_t* size) {
    VNode* node = vfs_find_node(path);
    if (!node || node->is_directory) {
        return -1;
    }
    
    // Load content if not already loaded
    if (!node->data && vfs_load_file_content(node) != 0) {
        return -1;
    }
    
    if (data) *data = node->data;
    if (size) *size = node->size;
    return 0;
}

void vfs_sync_all_persistent(void) {
    printf("Syncing all persistent storage...\n");
    // Implementation for syncing persistent files
    // For now, just print a message
    printf("Sync completed\n");
}

int vfs_set_host_root(const char* host_root) {
    if (!host_root) return -1;
    strncpy(host_root_directory, host_root, sizeof(host_root_directory)-1);
    host_root_directory[sizeof(host_root_directory)-1] = '\0';
    return 0;
}

// Mount a set of standard root directories from a host tree into the VM root
int vfs_mount_root_directories(const char* host_root, const char* const* dirs, size_t count) {
    if (!host_root || !dirs || count == 0) return -1;
    vfs_set_host_root(host_root);
    for (size_t i = 0; i < count; ++i) {
        char vm_path[256];
        char host_path[512];
        const char* name = dirs[i];
        if (!name || !*name) continue;
        // Ensure the directory exists in VM
        snprintf(vm_path, sizeof(vm_path), "/%s", name);
        vfs_create_directory(vm_path);
        // Build host path
        snprintf(host_path, sizeof(host_path), "%s\\%s", host_root, name);
        create_directory_recursive(host_path);
        vfs_mount_persistent(vm_path, host_path);
    }
    return 0;
}

int vfs_mount_root_autodiscover(const char* host_root) {
    if (!host_root) return -1;
    vfs_set_host_root(host_root);
    printf("Autodiscovering host root directories in %s...\n", host_root);
    WIN32_FIND_DATAA find_data;
    char search[MAX_PATH];
    snprintf(search, sizeof(search), "%s\\*", host_root);
    HANDLE hFind = FindFirstFileA(search, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No entries found in host root.\n");
        return -1;
    }
    do {
        if (strcmp(find_data.cFileName,".") == 0 || strcmp(find_data.cFileName,"..") == 0) continue;
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            char vm_path[256];
            char host_path[MAX_PATH];
            snprintf(vm_path, sizeof(vm_path), "/%s", find_data.cFileName);
            snprintf(host_path, sizeof(host_path), "%s\\%s", host_root, find_data.cFileName);
            vfs_create_directory(vm_path);
            vfs_mount_persistent(vm_path, host_path);
        } else {
            // Handle files in root directory
            char vm_path[256];
            char host_path[MAX_PATH];
            snprintf(vm_path, sizeof(vm_path), "/%s", find_data.cFileName);
            snprintf(host_path, sizeof(host_path), "%s\\%s", host_root, find_data.cFileName);
            
            // Create file node in VFS
            VNode* file_node = vfs_create_file_node(find_data.cFileName);
            if (file_node) {
                file_node->host_path = strdup(host_path);
                file_node->is_persistent = 1;
                
                // Add to root directory
                VNode* root = vfs_find_node("/");
                if (root) {
                    file_node->parent = root;
                    file_node->next = root->children;
                    root->children = file_node;
                }
            }
        }
    } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);
    printf("Autodiscovery complete.\n");
    return 0;
}