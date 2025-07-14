#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>     // Add this line
#include "vfs.h"

// Virtual file system that stays in memory
static VirtualFS* vm_fs = NULL;

static char current_directory[256] = "/";  // Add this global variable

// Fix the VFS node type - use your existing VNode structure
typedef enum {
    VFS_FILE,
    VFS_DIRECTORY
} VFSNodeType;

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
    vm_fs->root->is_persistent = 0;     // Add this line
    vm_fs->root->size = 0;
    vm_fs->root->data = NULL;
    vm_fs->root->host_path = NULL;      // Add this line
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
            return 0; // Already exists - return success instead of failure
        }
        existing = existing->next;
    }
    
    // Create new directory node
    VNode* new_dir = malloc(sizeof(VNode));
    if (!new_dir) return -1;
    
    strcpy(new_dir->name, dir_name);
    new_dir->is_directory = 1;
    new_dir->is_persistent = 1;     // Mark as persistent
    new_dir->size = 0;
    new_dir->data = NULL;
    new_dir->host_path = NULL;
    new_dir->parent = parent;
    new_dir->children = NULL;
    new_dir->next = parent->children;
    
    parent->children = new_dir;
    
    return 0;
}

int vfs_create_directory(const char* path) {
    return vfs_mkdir(path);  // Use the existing vfs_mkdir implementation
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
    VNode* new_file = malloc(sizeof(VNode));
    if (!new_file) return -1;
    
    strcpy(new_file->name, file_name);
    new_file->is_directory = 0;
    new_file->size = 0;
    new_file->data = NULL;
    new_file->parent = parent;
    new_file->children = NULL;
    new_file->next = parent->children;
    
    parent->children = new_file;
    
    return 0;
}

int vfs_rmdir(const char* path) {
    VNode* node = vfs_find_node(path);
    if (!node || !node->is_directory || node->children) {
        return -1; // Not found, not a directory, or not empty
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
        return -1; // Not found or is a directory
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
    free(node);
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

int vm_chdir(const char* path) {
    if (!vm_fs || !path) return -1;
    
    VNode* target = vfs_find_node(path);
    if (!target || !target->is_directory) {
        return -1;
    }
    
    vm_fs->current_dir = target;
    return 0;
}

int vm_mkdir(const char* path) {
    return vfs_mkdir(path);
}

int vm_rmdir(const char* path) {
    return vfs_rmdir(path);
}

int vm_remove(const char* filename) {
    return vfs_delete_file(filename);
}

VirtualFS* vfs_get_instance(void) {
    return vm_fs;
}

// Virtual command implementations
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

// Mount persistent directory
int vfs_mount_persistent(const char* vm_path, const char* host_path) {
    printf("DEBUG: Mounting persistent directory: %s -> %s\n", vm_path, host_path);
    
    // Create the host directory (recursively if needed)
    if (create_directory_recursive(host_path) != 0) {
        printf("Warning: Could not create host directory structure: %s\n", host_path);
        // Continue anyway - maybe the directory exists but we can't create it
    }
    
    // Load the directory contents
    return vfs_load_persistent_directory(vm_path, host_path);
}

// Load directory structure from host filesystem
int vfs_load_persistent_directory(const char* vm_path, const char* host_path) {
    printf("DEBUG: Loading persistent directory: %s -> %s\n", vm_path, host_path);
    
    // Get the VFS directory node
    VNode* vfs_dir = vfs_find_node(vm_path);
    if (!vfs_dir) {
        printf("DEBUG: VFS directory not found: %s\n", vm_path);
        return -1;
    }
    
    if (!vfs_dir->is_directory) {
        printf("DEBUG: VFS node is not a directory: %s\n", vm_path);
        return -1;
    }
    
    printf("DEBUG: Found VFS directory: %s\n", vm_path);
    
    // Open the host directory
    DIR* dir = opendir(host_path);
    if (!dir) {
        printf("DEBUG: Cannot open host directory: %s (errno: %d)\n", host_path, errno);
        return -1;
    }
    
    printf("DEBUG: Successfully opened host directory: %s\n", host_path);
    
    struct dirent* entry;
    int file_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        printf("DEBUG: Found entry: %s\n", entry->d_name);
        
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            printf("DEBUG: Skipping special directory: %s\n", entry->d_name);
            continue;
        }
        
        // Build full paths
        char full_host_path[512];
        char full_vm_path[512];
        snprintf(full_host_path, sizeof(full_host_path), "%s/%s", host_path, entry->d_name);
        snprintf(full_vm_path, sizeof(full_vm_path), "%s/%s", vm_path, entry->d_name);
        
        printf("DEBUG: Processing: %s -> %s\n", full_host_path, full_vm_path);
        
        // Check if it's a directory or file
        struct stat st;
        if (stat(full_host_path, &st) == 0) {
            printf("DEBUG: stat() successful for %s\n", full_host_path);
            
            if (S_ISDIR(st.st_mode)) {
                printf("DEBUG: Creating directory: %s\n", full_vm_path);
                // Create directory in VFS
                if (vfs_mkdir(full_vm_path) == 0) {
                    printf("DEBUG: Successfully created directory: %s\n", full_vm_path);
                    // Recursively load subdirectory
                    vfs_load_persistent_directory(full_vm_path, full_host_path);
                } else {
                    printf("DEBUG: Failed to create directory: %s\n", full_vm_path);
                }
            } else {
                printf("DEBUG: Creating file: %s (size: %zu)\n", full_vm_path, st.st_size);
                
                // Create file in VFS
                VNode* file_node = malloc(sizeof(VNode));
                if (file_node) {
                    strncpy(file_node->name, entry->d_name, sizeof(file_node->name) - 1);
                    file_node->name[sizeof(file_node->name) - 1] = '\0';
                    file_node->is_directory = 0;
                    file_node->is_persistent = 1;
                    file_node->size = st.st_size;
                    file_node->data = NULL; // We'll load data on demand
                    file_node->parent = vfs_dir;
                    file_node->children = NULL;
                    file_node->next = vfs_dir->children;
                    
                    // Store host path for later access
                    file_node->host_path = malloc(strlen(full_host_path) + 1);
                    if (file_node->host_path) {
                        strcpy(file_node->host_path, full_host_path);
                    }
                    
                    vfs_dir->children = file_node;
                    file_count++;
                    printf("DEBUG: Successfully created file node: %s (%zu bytes)\n", full_vm_path, file_node->size);
                } else {
                    printf("DEBUG: Failed to allocate memory for file node: %s\n", full_vm_path);
                }
            }
        } else {
            printf("DEBUG: stat() failed for %s (errno: %d)\n", full_host_path, errno);
        }
    }
    
    closedir(dir);
    printf("DEBUG: Loaded %d files from %s\n", file_count, host_path);
    return 0;
}

int vfs_create_persistent_file(const char* vm_path, const char* host_path) {
    // Implementation for creating persistent files
    printf("Creating persistent file: %s -> %s\n", vm_path, host_path);
    return 0;
}

// Sync persistent node to host filesystem
int vfs_sync_persistent_node(VNode* node) {
    if (!node || !node->is_persistent || !node->host_path) {
        return -1;
    }
    
    if (node->is_directory) {
        // Ensure directory exists
        mkdir(node->host_path);
    } else {
        // Write file content
        FILE* f = fopen(node->host_path, "wb");
        if (f && node->data) {
            fwrite(node->data, 1, node->size, f);
            fclose(f);
        }
    }
    
    return 0;
}

// Sync all persistent storage
int vfs_sync_all_persistent(void) {
    // Implementation for syncing all persistent storage
    printf("Syncing all persistent storage\n");
    return 0;
}

// Add this function to properly initialize persistent directories:

void vfs_initialize_persistent_directories(void) {
    printf("Initializing persistent directories...\n");
    
    // Create the persistent directory structure
    vfs_mkdir("/persistent");
    
    // Mount and load the host directories directly
    // Use "../ZoraPerl" since we're running from build directory
    printf("Loading persistent storage from host filesystem...\n");
    vfs_mount_persistent("/persistent/documents", "../ZoraPerl/documents");
    vfs_mount_persistent("/persistent/scripts", "../ZoraPerl/scripts");
    vfs_mount_persistent("/persistent/projects", "../ZoraPerl/projects");
    vfs_mount_persistent("/persistent/data", "../ZoraPerl/data");
    
    printf("Persistent directories initialized and loaded\n");
}

// Add this helper function for recursive directory creation:

int create_directory_recursive(const char* path) {
    char temp_path[512];
    char* p = NULL;
    size_t len;
    
    snprintf(temp_path, sizeof(temp_path), "%s", path);
    len = strlen(temp_path);
    
    // Remove trailing slash if present
    if (temp_path[len - 1] == '/' || temp_path[len - 1] == '\\') {
        temp_path[len - 1] = '\0';
    }
    
    // Create directories recursively
    for (p = temp_path + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            
            struct stat st;
            if (stat(temp_path, &st) != 0) {
                if (mkdir(temp_path) != 0) {
                    printf("Warning: Could not create directory: %s\n", temp_path);
                    return -1;
                }
                printf("Created directory: %s\n", temp_path);
            }
            
            *p = '/';
        }
    }
    
    // Create the final directory
    struct stat st;
    if (stat(temp_path, &st) != 0) {
        if (mkdir(temp_path) != 0) {
            printf("Warning: Could not create directory: %s\n", temp_path);
            return -1;
        }
        printf("Created directory: %s\n", temp_path);
    }
    
    return 0;
}