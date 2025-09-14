#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <stdint.h>
#include <time.h>

#define MAX_PACKAGES 1000
#define MAX_REPOSITORIES 16
#define MAX_DEPENDENCIES 32
#define MAX_PACKAGE_NAME 64
#define MAX_VERSION_STRING 32
#define MAX_DESCRIPTION 256
#define MAX_URL 512

// Package status
typedef enum {
    PKG_NOT_INSTALLED,
    PKG_INSTALLED,
    PKG_UPGRADABLE,
    PKG_BROKEN,
    PKG_HELD,
    PKG_REMOVING,
    PKG_INSTALLING
} PackageStatus;

// Package category
typedef enum {
    CAT_SYSTEM,
    CAT_DEVELOPMENT,
    CAT_NETWORK,
    CAT_MULTIMEDIA,
    CAT_OFFICE,
    CAT_GAMES,
    CAT_UTILITIES,
    CAT_LIBRARIES,
    CAT_LANGUAGES,
    CAT_SECURITY,
    CAT_EDITORS,
    CAT_SHELLS
} PackageCategory;

// Dependency type
typedef enum {
    DEP_REQUIRES,
    DEP_SUGGESTS,
    DEP_CONFLICTS,
    DEP_PROVIDES,
    DEP_REPLACES
} DependencyType;

// Architecture
typedef enum {
    ARCH_ANY,
    ARCH_X86_64,
    ARCH_I386,
    ARCH_ARM64,
    ARCH_ARM
} PackageArchitecture;

// Package dependency
typedef struct {
    char package_name[MAX_PACKAGE_NAME];
    char version_spec[MAX_VERSION_STRING];  // ">=1.0", "=2.1", etc.
    DependencyType type;
} PackageDependency;

// Package information
typedef struct {
    char name[MAX_PACKAGE_NAME];
    char version[MAX_VERSION_STRING];
    char description[MAX_DESCRIPTION];
    char maintainer[128];
    char homepage[MAX_URL];
    char download_url[MAX_URL];
    PackageCategory category;
    PackageArchitecture architecture;
    PackageStatus status;
    uint64_t size_compressed;      // Size of package file
    uint64_t size_installed;       // Size when installed
    time_t install_date;
    time_t last_update;
    char install_path[256];        // Where package is installed
    
    // Dependencies
    PackageDependency dependencies[MAX_DEPENDENCIES];
    int dependency_count;
    
    // Files installed by this package
    char installed_files[256][256];
    int installed_file_count;
    
    // Checksums for integrity
    char md5_hash[33];
    char sha256_hash[65];
    
    // Package metadata
    int priority;                  // Installation priority
    int is_essential;              // Essential package flag
    int is_manual;                 // Manually installed flag
    char license[64];
    char source_package[MAX_PACKAGE_NAME];
} PackageInfo;

// Repository information
typedef struct {
    char name[64];
    char url[MAX_URL];
    char distribution[32];         // "stable", "testing", "unstable"
    char component[32];            // "main", "contrib", "non-free"
    int enabled;
    int trusted;                   // GPG verified
    time_t last_update;
    int package_count;
    char gpg_key[128];
} Repository;

// Package manager transaction
typedef struct {
    char operation[32];            // "install", "remove", "upgrade"
    char package_names[64][MAX_PACKAGE_NAME];
    int package_count;
    uint64_t total_download_size;
    uint64_t total_install_size;
    int requires_restart;
    time_t transaction_time;
} PackageTransaction;

// Package manager state
typedef struct {
    PackageInfo packages[MAX_PACKAGES];
    int package_count;
    
    Repository repositories[MAX_REPOSITORIES];
    int repository_count;
    
    PackageTransaction transactions[100];
    int transaction_count;
    
    // Configuration
    char cache_directory[256];
    char install_root[256];
    int auto_clean_cache;
    int check_signatures;
    int download_only;
    int assume_yes;
    int verbose;
    
    // Statistics
    uint64_t total_cached_size;
    uint64_t total_installed_size;
    int total_packages_installed;
    int total_packages_upgradable;
    time_t last_update_check;
} PackageManager;

// Core package manager functions
int package_manager_init(void);
void package_manager_cleanup(void);

// Repository management
int pm_add_repository(const char* name, const char* url, const char* distribution, const char* component);
int pm_remove_repository(const char* name);
int pm_enable_repository(const char* name);
int pm_disable_repository(const char* name);
int pm_update_repositories(void);
void pm_list_repositories(void);

// Package database operations
int pm_refresh_package_database(void);
int pm_search_packages(const char* query);
int pm_show_package_info(const char* package_name);
int pm_list_installed_packages(void);
int pm_list_available_packages(void);
int pm_list_upgradable_packages(void);

// Package installation/removal
int pm_install_package(const char* package_name);
int pm_install_package_file(const char* package_file);
int pm_remove_package(const char* package_name);
int pm_purge_package(const char* package_name);
int pm_upgrade_package(const char* package_name);
int pm_upgrade_all_packages(void);

// Dependency resolution
int pm_resolve_dependencies(const char* package_name, char result_packages[][MAX_PACKAGE_NAME], int* count);
int pm_check_dependency_conflicts(const char* package_name);
int pm_calculate_install_order(char packages[][MAX_PACKAGE_NAME], int count, char ordered[][MAX_PACKAGE_NAME]);

// Package validation
int pm_verify_package_integrity(const char* package_name);
int pm_check_package_signatures(const char* package_file);
int pm_validate_dependencies(void);

// Package creation
int pm_create_package(const char* source_dir, const char* package_name, const char* version);
int pm_build_package_from_source(const char* source_url, const char* build_script);

// Cache management
int pm_clean_package_cache(void);
int pm_download_package(const char* package_name, const char* destination);
void pm_show_cache_statistics(void);

// Configuration
int pm_set_configuration(const char* key, const char* value);
char* pm_get_configuration(const char* key);
int pm_save_configuration(void);
int pm_load_configuration(void);

// Transaction management
int pm_begin_transaction(void);
int pm_commit_transaction(void);
int pm_rollback_transaction(void);
void pm_show_transaction_history(void);

// Advanced features
int pm_hold_package(const char* package_name);
int pm_unhold_package(const char* package_name);
int pm_pin_package_version(const char* package_name, const char* version);
int pm_create_snapshot(const char* snapshot_name);
int pm_restore_snapshot(const char* snapshot_name);

// Package file operations
int pm_extract_package(const char* package_file, const char* destination);
int pm_list_package_files(const char* package_name);
int pm_find_package_by_file(const char* filename);
int pm_which_package_owns_file(const char* filepath);

// Security features
int pm_check_for_security_updates(void);
int pm_audit_installed_packages(void);
int pm_verify_system_integrity(void);

// Import/Export
int pm_export_package_list(const char* filename);
int pm_import_package_list(const char* filename);
int pm_backup_package_database(const char* backup_file);
int pm_restore_package_database(const char* backup_file);

// Statistics and reporting
void pm_show_package_statistics(void);
void pm_show_disk_usage(void);
void pm_generate_dependency_graph(const char* package_name);
void pm_list_orphaned_packages(void);
void pm_list_manually_installed_packages(void);

#endif // PACKAGE_MANAGER_H