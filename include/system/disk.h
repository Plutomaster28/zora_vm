#ifndef ZORA_DISK_H
#define ZORA_DISK_H

#include <stdint.h>
#include <time.h>

// Disk information structure
typedef struct {
    char mount_point[256];
    char filesystem_type[32];
    uint64_t total_size;
    uint64_t used_size;
    uint64_t available_size;
    uint64_t total_inodes;
    uint64_t used_inodes;
    uint64_t available_inodes;
    int usage_percent;
    int readonly;
} DiskInfo;

// Directory entry for listing
typedef struct {
    char name[256];
    uint64_t size;
    int is_directory;
    time_t modified_time;
    char permissions[12];
} DiskEntry;

// Alias for backward compatibility
typedef DiskEntry DirectoryEntry;

// Disk quota information
typedef struct {
    char user[64];
    uint64_t block_used;
    uint64_t block_soft_limit;
    uint64_t block_hard_limit;
    uint64_t inode_used;
    uint64_t inode_soft_limit;
    uint64_t inode_hard_limit;
} DiskQuota;

// Disk operations
int disk_init(void);
void disk_cleanup(void);

// Disk information
int disk_get_info(const char* path, DiskInfo* info);
int disk_get_all_mounts(DiskInfo** mounts, int* count);
uint64_t disk_get_free_space(const char* path);
uint64_t disk_get_total_space(const char* path);
int disk_get_usage_percent(const char* path);

// Directory operations
int disk_list_directory(const char* path, DiskEntry** entries, int* count);
int disk_create_directory(const char* path, int mode);
int disk_remove_directory(const char* path, int recursive);
int disk_change_directory(const char* path);
char* disk_get_current_directory(char* buffer, size_t size);

// File size and information
uint64_t disk_get_file_size(const char* path);
time_t disk_get_file_mtime(const char* path);
int disk_get_file_info(const char* path, DiskEntry* entry);

// Disk usage calculation
uint64_t disk_calculate_directory_size(const char* path, int recursive);
int disk_find_large_files(const char* path, uint64_t min_size, 
                          DiskEntry** entries, int* count);

// Quota management (simplified)
int disk_get_quota(const char* user, uint64_t* quota_bytes, uint64_t* used_bytes);
int disk_set_quota(const char* user, uint64_t quota_bytes);
int disk_check_quota(const char* user);

// Filesystem checks
int disk_check_filesystem(const char* path);
int disk_repair_filesystem(const char* path);

// Disk statistics
typedef struct {
    uint64_t reads_completed;
    uint64_t writes_completed;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t io_errors;
} DiskStats;

int disk_get_stats(const char* device, DiskStats* stats);

// Utility functions
void disk_format_size(uint64_t bytes, char* buffer, size_t size);
int disk_path_exists(const char* path);
int disk_is_directory(const char* path);
int disk_is_file(const char* path);

#endif // ZORA_DISK_H
