#include "system/disk.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

// Helper function to count inodes recursively
static int count_inodes_recursive(VNode* node) {
    if (!node) return 0;
    
    int count = 1;  // Count this node
    
    if (node->is_directory && node->children) {
        VNode* child = node->children;
        while (child) {
            count += count_inodes_recursive(child);
            child = child->next;
        }
    }
    
    return count;
}

// Helper function to calculate directory size recursively
static uint64_t calculate_size_recursive(VNode* node) {
    if (!node) return 0;
    
    uint64_t total = node->size;
    
    if (node->is_directory && node->children) {
        VNode* child = node->children;
        while (child) {
            total += calculate_size_recursive(child);
            child = child->next;
        }
    }
    
    return total;
}

// Initialize disk management
int disk_init(void) {
    // Nothing special needed for now
    return 0;
}

void disk_cleanup(void) {
    // Nothing to clean up
}

// Get disk information for a path
int disk_get_info(const char* path, DiskInfo* info) {
    if (!path || !info) return -1;
    
    memset(info, 0, sizeof(DiskInfo));
    strncpy(info->mount_point, path, sizeof(info->mount_point) - 1);
    strcpy(info->filesystem_type, "ZoraVFS");
    
    // For VFS, get virtual filesystem stats
    VNode* node = vfs_find_node(path);
    if (node) {
        VirtualFS* vfs = vfs_get_instance();
        
        // Calculate total size by traversing VFS
        info->total_size = 1024ULL * 1024 * 1024 * 10; // 10GB virtual
        info->used_size = (vfs && vfs->root) ? calculate_size_recursive(vfs->root) : 0;
        info->available_size = info->total_size - info->used_size;
        info->usage_percent = (int)((info->used_size * 100) / info->total_size);
        info->total_inodes = 100000;
        info->used_inodes = (vfs && vfs->root) ? count_inodes_recursive(vfs->root) : 0;
        info->available_inodes = info->total_inodes - info->used_inodes;
        info->readonly = 0;
        return 0;
    }
    
    return -1;
}

// Get all mount points
int disk_get_all_mounts(DiskInfo** mounts, int* count) {
    if (!mounts || !count) return -1;
    
    // For now, just return root mount
    *mounts = (DiskInfo*)malloc(sizeof(DiskInfo));
    if (!*mounts) return -1;
    
    disk_get_info("/", *mounts);
    *count = 1;
    
    return 0;
}

// Get free space
uint64_t disk_get_free_space(const char* path) {
    DiskInfo info;
    if (disk_get_info(path, &info) == 0) {
        return info.available_size;
    }
    return 0;
}

// Get total space
uint64_t disk_get_total_space(const char* path) {
    DiskInfo info;
    if (disk_get_info(path, &info) == 0) {
        return info.total_size;
    }
    return 0;
}

// Get usage percentage
int disk_get_usage_percent(const char* path) {
    DiskInfo info;
    if (disk_get_info(path, &info) == 0) {
        return info.usage_percent;
    }
    return -1;
}

// List directory contents
int disk_list_directory(const char* path, DiskEntry** entries, int* count) {
    if (!path || !entries || !count) return -1;
    
    VNode* dir = vfs_find_node(path);
    if (!dir || !dir->is_directory) {
        return -1;
    }
    
    // Count children
    int num_entries = 0;
    VNode* child = dir->children;
    while (child) {
        num_entries++;
        child = child->next;
    }
    
    // Allocate array
    *entries = (DiskEntry*)calloc(num_entries, sizeof(DiskEntry));
    if (!*entries) return -1;
    
    // Fill entries
    child = dir->children;
    int index = 0;
    while (child && index < num_entries) {
        DiskEntry* entry = &(*entries)[index];
        
        strncpy(entry->name, child->name, sizeof(entry->name) - 1);
        entry->size = child->size;
        entry->is_directory = child->is_directory;
        entry->modified_time = child->modified_time;
        
        // Format permissions
        snprintf(entry->permissions, sizeof(entry->permissions), 
                "%c%c%c%c%c%c%c%c%c%c",
                child->is_directory ? 'd' : '-',
                (child->mode & 0400) ? 'r' : '-',
                (child->mode & 0200) ? 'w' : '-',
                (child->mode & 0100) ? 'x' : '-',
                (child->mode & 0040) ? 'r' : '-',
                (child->mode & 0020) ? 'w' : '-',
                (child->mode & 0010) ? 'x' : '-',
                (child->mode & 0004) ? 'r' : '-',
                (child->mode & 0002) ? 'w' : '-',
                (child->mode & 0001) ? 'x' : '-');
        
        child = child->next;
        index++;
    }
    
    *count = num_entries;
    return 0;
}

// Get file size
uint64_t disk_get_file_size(const char* path) {
    VNode* node = vfs_find_node(path);
    if (node && !node->is_directory) {
        return node->size;
    }
    return 0;
}

// Get file modification time
time_t disk_get_file_mtime(const char* path) {
    VNode* node = vfs_find_node(path);
    if (node) {
        return node->modified_time;
    }
    return 0;
}

// Get detailed file information
int disk_get_file_info(const char* path, DiskEntry* entry) {
    if (!path || !entry) return -1;
    
    VNode* node = vfs_find_node(path);
    if (!node) return -1;
    
    strncpy(entry->name, node->name, sizeof(entry->name) - 1);
    entry->size = node->size;
    entry->is_directory = node->is_directory;
    entry->modified_time = node->modified_time;
    
    snprintf(entry->permissions, sizeof(entry->permissions),
            "%c%c%c%c%c%c%c%c%c%c",
            node->is_directory ? 'd' : '-',
            (node->mode & 0400) ? 'r' : '-',
            (node->mode & 0200) ? 'w' : '-',
            (node->mode & 0100) ? 'x' : '-',
            (node->mode & 0040) ? 'r' : '-',
            (node->mode & 0020) ? 'w' : '-',
            (node->mode & 0010) ? 'x' : '-',
            (node->mode & 0004) ? 'r' : '-',
            (node->mode & 0002) ? 'w' : '-',
            (node->mode & 0001) ? 'x' : '-');
    
    return 0;
}

// Calculate directory size
uint64_t disk_calculate_directory_size(const char* path, int recursive) {
    VNode* node = vfs_find_node(path);
    if (!node) return 0;
    
    if (!node->is_directory) {
        return node->size;
    }
    
    if (node->is_directory && recursive) {
        return calculate_size_recursive(node);
    } else if (node->is_directory) {
        // Only direct children
        uint64_t total = 0;
        VNode* child = node->children;
        while (child) {
            if (!child->is_directory) {
                total += child->size;
            }
            child = child->next;
        }
        return total;
    }
    
    return 0;
}

// Format size
void disk_format_size(uint64_t bytes, char* buffer, size_t buffer_size) {
    if (bytes < 1024) {
        snprintf(buffer, buffer_size, "%lluB", (unsigned long long)bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fKB", bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fMB", bytes / (1024.0 * 1024));
    } else if (bytes < 1024ULL * 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2fGB", bytes / (1024.0 * 1024 * 1024));
    } else {
        snprintf(buffer, buffer_size, "%.2fTB", bytes / (1024.0 * 1024 * 1024 * 1024));
    }
}

// Check if path exists
int disk_path_exists(const char* path) {
    return vfs_find_node(path) != NULL;
}

// Check if is directory
int disk_is_directory(const char* path) {
    VNode* node = vfs_find_node(path);
    return node && node->is_directory;
}

// Check if is file
int disk_is_file(const char* path) {
    VNode* node = vfs_find_node(path);
    return node && !node->is_directory;
}

// Find large files
int disk_find_large_files(const char* path, uint64_t min_size, DiskEntry** entries, int* count) {
    if (!path || !entries || !count) return -1;
    
    VNode* dir = vfs_find_node(path);
    if (!dir || !dir->is_directory) return -1;
    
    // First pass: count matching files
    int num_found = 0;
    VNode* child = dir->children;
    while (child) {
        if (!child->is_directory && child->size >= min_size) {
            num_found++;
        }
        child = child->next;
    }
    
    if (num_found == 0) {
        *entries = NULL;
        *count = 0;
        return 0;
    }
    
    // Allocate array
    *entries = (DiskEntry*)calloc(num_found, sizeof(DiskEntry));
    if (!*entries) return -1;
    
    // Second pass: fill entries
    child = dir->children;
    int index = 0;
    while (child && index < num_found) {
        if (!child->is_directory && child->size >= min_size) {
            disk_get_file_info(child->name, &(*entries)[index]);
            index++;
        }
        child = child->next;
    }
    
    *count = num_found;
    return 0;
}

// Quota management
int disk_set_quota(const char* user, uint64_t quota_bytes) {
    // Store quota settings (simplified - in a real system this would be persistent)
    // For now just accept the value
    return 0;
}

int disk_get_quota(const char* user, uint64_t* quota_bytes, uint64_t* used_bytes) {
    if (!user || !quota_bytes || !used_bytes) return -1;
    
    // Simplified: return defaults
    *quota_bytes = 1024ULL * 1024 * 1024; // 1GB
    *used_bytes = 0; // Would need to track per-user usage
    
    return 0;
}

int disk_check_quota(const char* user) {
    uint64_t quota, used;
    if (disk_get_quota(user, &quota, &used) == 0) {
        return (used < quota) ? 0 : -1;
    }
    return -1;
}
