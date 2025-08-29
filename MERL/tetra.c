#include "tetra.h"
#include "utils.h"
#include "platform/platform.h"
// Platform-specific includes and compatibility layer
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Global repository location variable
static char session_repo_location[MAX_PATH] = {0};

// Default repository location - can be overridden by environment or command
const char* get_repo_location() {
    if (session_repo_location[0] != '\0') {
        return session_repo_location;
    }
    
    // Check environment variable first
    const char* env_location = getenv("TETRA_REPO_LOCATION");
    if (env_location) {
        return env_location;
    }
    
    // Default location
    return "C:\\ZoraPerl\\projects";
}

void tetra_init(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra init <repo-folder-name>\n");
        return;
    }
    char repo_path[MAX_PATH];
    snprintf(repo_path, sizeof(repo_path), "%s\\%s", get_repo_location(), argv[2]);
    
    if (CreateDirectoryA(repo_path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        printf("Repository '%s' initialized at %s\n", argv[2], repo_path);
        
        // Create basic structure
        char bin_path[MAX_PATH];
        snprintf(bin_path, sizeof(bin_path), "%s\\bin", repo_path);
        CreateDirectoryA(bin_path, NULL);
        
        char scripts_path[MAX_PATH];
        snprintf(scripts_path, sizeof(scripts_path), "%s\\scripts", repo_path);
        CreateDirectoryA(scripts_path, NULL);
        
        // Create a basic README
        char readme_path[MAX_PATH];
        snprintf(readme_path, sizeof(readme_path), "%s\\README.md", repo_path);
        FILE* readme = fopen(readme_path, "w");
        if (readme) {
            fprintf(readme, "# %s\n\nTetra repository created on ZoraVM.\n", argv[2]);
            fclose(readme);
        }
    } else {
        printf("Failed to create repository '%s'. Error: %d\n", argv[2], GetLastError());
    }
}

void tetra_clone(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: tetra clone <source-path> <repo-folder-name>\n");
        return;
    }
    char dest_path[MAX_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s\\%s", get_repo_location(), argv[3]);
    
    SHFILEOPSTRUCT fileOp = {0};
    char from[MAX_PATH];
    char to[MAX_PATH];
    
    snprintf(from, sizeof(from), "%s", argv[2]);
    snprintf(to, sizeof(to), "%s", dest_path);
    
    // SHFileOperation requires double-null-terminated strings
    from[strlen(from) + 1] = '\0';
    to[strlen(to) + 1] = '\0';
    
    fileOp.wFunc = FO_COPY;
    fileOp.pFrom = from;
    fileOp.pTo = to;
    fileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
    
    if (SHFileOperation(&fileOp) == 0) {
        printf("Repository cloned from '%s' to '%s'\n", argv[2], argv[3]);
    } else {
        printf("Failed to clone repository from '%s'\n", argv[2]);
    }
}

// List all folders in TETRA_REPO_LOCATION
void tetra_list(int argc, char **argv) {
    printf("Repositories in %s:\n", get_repo_location());
    char search_path[MAX_PATH];
    
    snprintf(search_path, sizeof(search_path), "%s\\*", get_repo_location());

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

void tetra_remove(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra remove <repo-folder-name>\n");
        return;
    }
    char repo_path[MAX_PATH];
    
    snprintf(repo_path, sizeof(repo_path), "%s\\%s", get_repo_location(), argv[2]);

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

void tetra_move(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: tetra move <repo-folder-name> <new-path>\n");
        return;
    }
    char src_path[MAX_PATH];
    
    snprintf(src_path, sizeof(src_path), "%s\\%s", get_repo_location(), argv[2]);

    DWORD attrib = GetFileAttributes(src_path);
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        printf("Repository '%s' not found.\n", argv[2]);
        return;
    }

    char dst_path[MAX_PATH];
    snprintf(dst_path, sizeof(dst_path), "%s\\%s", argv[3], argv[2]);

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

void tetra_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Tetra Package Manager for ZoraVM\n");
        printf("Available commands:\n");
        printf("  init <name>           - Create a new repository\n");
        printf("  clone <src> <name>    - Clone a repository\n");
        printf("  list                  - List all repositories\n");
        printf("  remove <name>         - Remove a repository\n");
        printf("  move <name> <path>    - Move a repository\n");
        printf("  set-location <path>   - Set repository location\n");
        return;
    }
    
    if (strcmp(argv[1], "init") == 0) {
        tetra_init(argc, argv);
    } else if (strcmp(argv[1], "clone") == 0) {
        tetra_clone(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        tetra_list(argc, argv);
    } else if (strcmp(argv[1], "remove") == 0) {
        tetra_remove(argc, argv);
    } else if (strcmp(argv[1], "move") == 0) {
        tetra_move(argc, argv);
    } else if (strcmp(argv[1], "set-location") == 0) {
        tetra_set_location(argc, argv);
    } else {
        printf("Unknown tetra command: %s\n", argv[1]);
        printf("Type 'tetra' for help.\n");
    }
}
