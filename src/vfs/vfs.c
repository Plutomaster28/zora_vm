#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"
#include <sys/stat.h>
#include <dirent.h>

// Virtual file system that stays in memory
static VirtualFS* vm_fs = NULL;

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
    vm_fs->root->size = 0;
    vm_fs->root->data = NULL;
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
            return -1; // Already exists
        }
        existing = existing->next;
    }
    
    // Create new directory node
    VNode* new_dir = malloc(sizeof(VNode));
    if (!new_dir) return -1;
    
    strcpy(new_dir->name, dir_name);
    new_dir->is_directory = 1;
    new_dir->size = 0;
    new_dir->data = NULL;
    new_dir->parent = parent;
    new_dir->children = NULL;
    new_dir->next = parent->children;
    
    parent->children = new_dir;
    
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
    // Create the mount point in VM filesystem
    VNode* mount_point = vfs_find_node(vm_path);
    if (!mount_point) {
        // Create the mount point
        if (vfs_mkdir(vm_path) != 0) {
            return -1;
        }
        mount_point = vfs_find_node(vm_path);
    }
    
    // Mark as persistent and set host path
    mount_point->is_persistent = 1;
    mount_point->host_path = malloc(strlen(host_path) + 1);
    strcpy(mount_point->host_path, host_path);
    
    // Load existing directory structure from host
    vfs_load_persistent_directory(vm_path, host_path);
    
    printf("Mounted persistent directory: %s -> %s\n", vm_path, host_path);
    return 0;
}

// Load directory structure from host filesystem
int vfs_load_persistent_directory(const char* vm_path, const char* host_path) {
    DIR* dir = opendir(host_path);
    if (!dir) {
        // Create directory if it doesn't exist
        mkdir(host_path);
        return 0;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_host_path[512];
        char full_vm_path[512];
        snprintf(full_host_path, sizeof(full_host_path), "%s/%s", host_path, entry->d_name);
        snprintf(full_vm_path, sizeof(full_vm_path), "%s/%s", vm_path, entry->d_name);
        
        struct stat st;
        if (stat(full_host_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Create directory in VM and recurse
                vfs_mkdir(full_vm_path);
                VNode* node = vfs_find_node(full_vm_path);
                if (node) {
                    node->is_persistent = 1;
                    node->host_path = malloc(strlen(full_host_path) + 1);
                    strcpy(node->host_path, full_host_path);
                }
                vfs_load_persistent_directory(full_vm_path, full_host_path);
            } else {
                // Create file in VM
                vfs_create_persistent_file(full_vm_path, full_host_path);
            }
        }
    }
    
    closedir(dir);
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