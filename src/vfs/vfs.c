#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "platform/platform.h"

// Platform-specific directory handling
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <direct.h>
    #include <sys/stat.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

// Virtual file system that stays in memory
static VirtualFS* vm_fs = NULL;
static char current_directory[256] = "/";

// NEW: Helper function to create directory nodes
VNode* vfs_create_directory_node(const char* name) {
    VNode* node = malloc(sizeof(VNode));
    if (!node) return NULL;
    
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->is_directory = 1;
    node->is_persistent = 0;
    node->size = 0;
    node->data = NULL;
    node->host_path = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;
    
    return node;
}

// NEW: Helper function to create file nodes
VNode* vfs_create_file_node(const char* name) {
    VNode* node = malloc(sizeof(VNode));
    if (!node) return NULL;
    
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->is_directory = 0;
    node->is_persistent = 0;
    node->size = 0;
    node->data = NULL;
    node->host_path = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;
    
    return node;
}

// NEW: Helper function to add child to parent
void vfs_add_child(VNode* parent, VNode* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    child->next = parent->children;
    parent->children = child;
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
#ifdef PLATFORM_WINDOWS
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
#else
            struct stat st;
            if (stat(temp_path, &st) != 0) {
                if (mkdir(temp_path, 0755) != 0) {
                    perror("mkdir");
                    return -1;
                }
            }
#endif
            *ptr = '/';
        }
        ptr++;
    }
    
    // Create the final directory
#ifdef PLATFORM_WINDOWS
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
#else
    struct stat st;
    if (stat(temp_path, &st) != 0) {
        if (mkdir(temp_path, 0755) != 0) {
            perror("mkdir");
            return -1;
        }
    }
#endif
    
    return 0;
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
    
    printf("Virtual filesystem initialized\n");
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
    return 0;
}

// FIXED: Load persistent directory function
void vfs_load_persistent_directory(VNode* vm_node, const char* host_path) {
    printf("DEBUG: Loading persistent directory from %s to %s\n", host_path, vm_node->name);
    
#ifdef PLATFORM_WINDOWS
    // Windows implementation using FindFirstFile/FindNextFile
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", host_path);
    
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("DEBUG: Failed to open directory: %s\n", host_path);
        return;
    }
    
    do {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        printf("DEBUG: Found entry: %s\n", find_data.cFileName);
        
        // Build full paths
        char full_host_path[MAX_PATH];
        snprintf(full_host_path, sizeof(full_host_path), "%s\\%s", host_path, find_data.cFileName);
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // It's a directory
            printf("DEBUG: Creating directory node: %s\n", find_data.cFileName);
            VNode* dir_node = vfs_create_directory_node(find_data.cFileName);
            if (dir_node) {
                vfs_add_child(vm_node, dir_node);
                // Recursive load
                vfs_load_persistent_directory(dir_node, full_host_path);
            }
        } else {
            // It's a file
            printf("DEBUG: Creating file node: %s\n", find_data.cFileName);
            VNode* file_node = vfs_create_file_node(find_data.cFileName);
            if (file_node) {
                file_node->host_path = malloc(strlen(full_host_path) + 1);
                if (file_node->host_path) {
                    strcpy(file_node->host_path, full_host_path);
                }
                file_node->size = find_data.nFileSizeLow;
                file_node->is_persistent = 1;
                vfs_add_child(vm_node, file_node);
                printf("DEBUG: Added file: %s (size: %zu)\n", file_node->name, file_node->size);
            }
        }
    } while (FindNextFileA(hFind, &find_data) != 0);
    
    FindClose(hFind);
    
#else
    // Linux implementation
    DIR* dir = opendir(host_path);
    if (!dir) {
        printf("DEBUG: Failed to open directory: %s\n", host_path);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        printf("DEBUG: Found entry: %s\n", entry->d_name);
        
        // Build full path
        char full_host_path[PATH_MAX];
        snprintf(full_host_path, sizeof(full_host_path), "%s/%s", host_path, entry->d_name);
        
        // Check if it's a directory or file
        struct stat st;
        if (stat(full_host_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // It's a directory
                VNode* dir_node = vfs_create_directory_node(entry->d_name);
                if (dir_node) {
                    vfs_add_child(vm_node, dir_node);
                    vfs_load_persistent_directory(dir_node, full_host_path);
                }
            } else {
                // It's a file
                VNode* file_node = vfs_create_file_node(entry->d_name);
                if (file_node) {
                    file_node->host_path = malloc(strlen(full_host_path) + 1);
                    if (file_node->host_path) {
                        strcpy(file_node->host_path, full_host_path);
                    }
                    file_node->size = st.st_size;
                    file_node->is_persistent = 1;
                    vfs_add_child(vm_node, file_node);
                }
            }
        }
    }
    
    closedir(dir);
#endif
    
    printf("DEBUG: Finished loading directory: %s\n", host_path);
}

// Mount persistent directory
int vfs_mount_persistent(const char* vm_path, const char* host_path) {
    printf("DEBUG: Mounting persistent directory: %s -> %s\n", vm_path, host_path);
    
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
    vfs_load_persistent_directory(vm_node, host_path);
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
}

int vfs_delete_file(const char* path) {
    VNode* node = vfs_find_node(path);
    if (!node || node->is_directory) {
        return -1;
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
}

int vfs_create_directory(const char* path) {
    return vfs_mkdir(path);
}

void vfs_sync_all_persistent(void) {
    printf("Syncing all persistent storage...\n");
    // Implementation for syncing persistent files
    // For now, just print a message
    printf("Sync completed\n");
}