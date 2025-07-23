#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/platform.h"
#include "tetra.h"
#include "config.h"

// Platform-specific includes and compatibility layer
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <shlwapi.h>
    #include <shellapi.h>
    #pragma comment(lib, "Shlwapi.lib")
#else
    // Linux includes
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <ftw.h>
    #include <errno.h>
    
    // Define missing constants for Linux
    #ifndef INVALID_FILE_ATTRIBUTES
    #define INVALID_FILE_ATTRIBUTES ((unsigned long)-1)
    #endif
    
    #ifndef FILE_ATTRIBUTE_DIRECTORY
    #define FILE_ATTRIBUTE_DIRECTORY S_IFDIR
    #endif
    
    // Windows-style structures for Linux
    typedef struct {
        char cFileName[256];
        unsigned long dwFileAttributes;
    } WIN32_FIND_DATA;
    
    typedef struct {
        DIR* dir;
        char pattern[256];
        char current_path[512];
    } LinuxFindHandle;
    
    static LinuxFindHandle* linux_find_handle = NULL;
    
    // Linux implementation of Windows directory functions
    static inline DWORD GetFileAttributes(const char* path) {
        struct stat st;
        if (stat(path, &st) == 0) {
            return st.st_mode;
        }
        return INVALID_FILE_ATTRIBUTES;
    }
    
    // Forward declarations
    static int FindNextFile(HANDLE hFind, WIN32_FIND_DATA* find_data);
    static void FindClose(HANDLE hFind);
    
    static inline HANDLE FindFirstFile(const char* path, WIN32_FIND_DATA* find_data) {
        if (linux_find_handle) {
            if (linux_find_handle->dir) closedir(linux_find_handle->dir);
            free(linux_find_handle);
        }
        
        linux_find_handle = malloc(sizeof(LinuxFindHandle));
        if (!linux_find_handle) return INVALID_HANDLE_VALUE;
        
        // Extract directory and pattern from path
        char* last_slash = strrchr(path, '/');
        if (last_slash) {
            strncpy(linux_find_handle->current_path, path, last_slash - path);
            linux_find_handle->current_path[last_slash - path] = '\0';
            strcpy(linux_find_handle->pattern, last_slash + 1);
        } else {
            strcpy(linux_find_handle->current_path, ".");
            strcpy(linux_find_handle->pattern, path);
        }
        
        linux_find_handle->dir = opendir(linux_find_handle->current_path);
        if (!linux_find_handle->dir) {
            free(linux_find_handle);
            linux_find_handle = NULL;
            return INVALID_HANDLE_VALUE;
        }
        
        // Try to find first match
        if (FindNextFile((HANDLE)linux_find_handle, find_data)) {
            return (HANDLE)linux_find_handle;
        } else {
            FindClose((HANDLE)linux_find_handle);
            return INVALID_HANDLE_VALUE;
        }
    }
    
    static int FindNextFile(HANDLE hFind, WIN32_FIND_DATA* find_data) {
        LinuxFindHandle* handle = (LinuxFindHandle*)hFind;
        if (!handle || !handle->dir) return 0;
        
        struct dirent* entry;
        while ((entry = readdir(handle->dir)) != NULL) {
            // Simple pattern matching (just * wildcard)
            if (strcmp(handle->pattern, "*") == 0 || 
                strstr(entry->d_name, handle->pattern) != NULL) {
                
                strcpy(find_data->cFileName, entry->d_name);
                
                // Get file attributes
                char full_path[768];
                snprintf(full_path, sizeof(full_path), "%s/%s", 
                         handle->current_path, entry->d_name);
                
                struct stat st;
                if (stat(full_path, &st) == 0) {
                    find_data->dwFileAttributes = st.st_mode;
                } else {
                    find_data->dwFileAttributes = 0;
                }
                
                return 1; // Found a match
            }
        }
        
        return 0; // No more matches
    }
    
    static void FindClose(HANDLE hFind) {
        LinuxFindHandle* handle = (LinuxFindHandle*)hFind;
        if (handle) {
            if (handle->dir) closedir(handle->dir);
            free(handle);
            if (linux_find_handle == handle) {
                linux_find_handle = NULL;
            }
        }
    }
    
    // Linux implementations of missing functions
    static int remove_directory_recursive_linux(const char* path) {
        char command[512];
        snprintf(command, sizeof(command), "rm -rf \"%s\"", path);
        return system(command);
    }
    
    static int move_directory_recursive_linux(const char* src, const char* dst) {
        char command[1024];
        snprintf(command, sizeof(command), "mv \"%s\" \"%s\"", src, dst);
        return system(command);
    }
#endif

static char session_repo_location[MAX_PATH] = TETRA_REPO_LOCATION;

// Helper to get current repo location
const char* get_repo_location() {
    return session_repo_location;
}

void tetra_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: tetra <download|list|remove|move|set-location> [args]\n");
        return;
    }
    if (strcmp(argv[1], "download") == 0) {
        tetra_download(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        tetra_list(argc, argv);
    } else if (strcmp(argv[1], "remove") == 0) {
        tetra_remove(argc, argv);
    } else if (strcmp(argv[1], "move") == 0) {
        tetra_move(argc, argv);
    } else if (strcmp(argv[1], "set-location") == 0) {
        tetra_set_location(argc, argv);
    } else {
        printf("Unknown tetra subcommand: %s\n", argv[1]);
    }
}

void tetra_download(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra download <git-repo-url>\n");
        return;
    }
    char command[512];
    snprintf(command, sizeof(command), "git clone %s %s", argv[2], get_repo_location());
    printf("Cloning repository to %s...\n", get_repo_location());
    int result = system(command);
    if (result == 0) {
        printf("Repository cloned successfully.\n");
    } else {
        printf("Failed to clone repository. Make sure git is installed and the URL is correct.\n");
    }
}

// List all folders in TETRA_REPO_LOCATION
void tetra_list(int argc, char **argv) {
    printf("Repositories in %s:\n", get_repo_location());
    char search_path[MAX_PATH];
    
#ifdef PLATFORM_WINDOWS
    snprintf(search_path, sizeof(search_path), "%s\\*", get_repo_location());
#else
    snprintf(search_path, sizeof(search_path), "%s/*", get_repo_location());
#endif

    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No repositories found or directory does not exist.\n");
        return;
    }

    do {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(find_data.cFileName, ".") != 0 &&
            strcmp(find_data.cFileName, "..") != 0) {
            printf(" - %s\n", find_data.cFileName);
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

#ifdef PLATFORM_WINDOWS
int remove_directory_recursive(const char *path) {
    WIN32_FIND_DATA find_data;
    char search_path[MAX_PATH];
    char file_path[MAX_PATH];

    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    HANDLE hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;
        snprintf(file_path, sizeof(file_path), "%s\\%s", path, find_data.cFileName);
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            remove_directory_recursive(file_path);
        } else {
            DeleteFile(file_path);
        }
    } while (FindNextFile(hFind, &find_data) != 0);
    FindClose(hFind);
    return RemoveDirectory(path);
}
#else
int remove_directory_recursive(const char *path) {
    return remove_directory_recursive_linux(path) == 0;
}
#endif

void tetra_remove(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra remove <repo-folder-name>\n");
        return;
    }
    char repo_path[MAX_PATH];
    
#ifdef PLATFORM_WINDOWS
    snprintf(repo_path, sizeof(repo_path), "%s\\%s", get_repo_location(), argv[2]);
#else
    snprintf(repo_path, sizeof(repo_path), "%s/%s", get_repo_location(), argv[2]);
#endif

    DWORD attrib = GetFileAttributes(repo_path);
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        printf("Repository '%s' not found.\n", argv[2]);
        return;
    }
    if (remove_directory_recursive(repo_path)) {
        printf("Repository '%s' removed successfully.\n", argv[2]);
    } else {
        printf("Failed to remove repository '%s'.\n", argv[2]);
    }
}

#ifdef PLATFORM_WINDOWS
int move_directory_recursive(const char *src, const char *dst) {
    SHFILEOPSTRUCT fileOp = {0};
    char from[MAX_PATH];
    char to[MAX_PATH];

    snprintf(from, sizeof(from), "%s", src);
    snprintf(to, sizeof(to), "%s", dst);

    // SHFileOperation requires double-null-terminated strings
    from[strlen(from) + 1] = '\0';
    to[strlen(to) + 1] = '\0';

    fileOp.wFunc = FO_MOVE;
    fileOp.pFrom = from;
    fileOp.pTo = to;
    fileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

    return SHFileOperation(&fileOp) == 0;
}
#else
int move_directory_recursive(const char *src, const char *dst) {
    return move_directory_recursive_linux(src, dst) == 0;
}
#endif

void tetra_move(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: tetra move <repo-folder-name> <new-path>\n");
        return;
    }
    char src_path[MAX_PATH];
    
#ifdef PLATFORM_WINDOWS
    snprintf(src_path, sizeof(src_path), "%s\\%s", get_repo_location(), argv[2]);
#else
    snprintf(src_path, sizeof(src_path), "%s/%s", get_repo_location(), argv[2]);
#endif

    DWORD attrib = GetFileAttributes(src_path);
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        printf("Repository '%s' not found.\n", argv[2]);
        return;
    }

    char dst_path[MAX_PATH];
#ifdef PLATFORM_WINDOWS
    snprintf(dst_path, sizeof(dst_path), "%s\\%s", argv[3], argv[2]);
#else
    snprintf(dst_path, sizeof(dst_path), "%s/%s", argv[3], argv[2]);
#endif

    if (move_directory_recursive(src_path, dst_path)) {
        printf("Repository '%s' moved to '%s'.\n", argv[2], argv[3]);
    } else {
        printf("Failed to move repository '%s'.\n", argv[2]);
    }
}

void tetra_set_location(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra set-location <new-directory>\n");
        return;
    }
    strncpy(session_repo_location, argv[2], MAX_PATH - 1);
    session_repo_location[MAX_PATH - 1] = '\0';
    printf("Temporary repository location set to: %s\n", session_repo_location);
}