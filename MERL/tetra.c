#include "tetra.h"
#include "utils.h"
#include "platform/platform.h"
#include "vfs/vfs.h"  // Add VFS support
// Platform-specific includes and compatibility layer
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Global repository location variable - now defaults to current VFS directory
static char session_repo_location[MAX_PATH] = {0};

// Modern repository location - uses VFS current directory
const char* get_repo_location() {
    if (session_repo_location[0] != '\0') {
        return session_repo_location;
    }
    
    // Check environment variable first
    const char* env_location = getenv("TETRA_REPO_LOCATION");
    if (env_location) {
        return env_location;
    }
    
    // Use current VFS directory instead of hardcoded path
    char* current_dir = vm_getcwd();
    if (current_dir && strlen(current_dir) > 0) {
        return current_dir;
    }
    
    // Fallback to root if current directory fails
    return "/";
}

void tetra_init(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tetra init <repo-folder-name> [target-directory]\n");
        printf("If target-directory is not specified, creates in current directory.\n");
        return;
    }
    
    // Allow user to specify target directory or use current location
    const char* base_location;
    if (argc >= 4) {
        base_location = argv[3];
        printf("Using specified directory: %s\n", base_location);
    } else {
        base_location = get_repo_location();
        printf("Using current directory: %s\n", base_location);
    }
    
    char repo_path[MAX_PATH];
    if (strcmp(base_location, "/") == 0) {
        snprintf(repo_path, sizeof(repo_path), "/%s", argv[2]);
    } else {
        snprintf(repo_path, sizeof(repo_path), "%s/%s", base_location, argv[2]);
    }
    
    // Use VFS to create directory
    if (vfs_create_directory(repo_path) == 0) {
        printf("Repository '%s' initialized at %s\n", argv[2], repo_path);
        
        // Create basic structure using VFS
        char bin_path[MAX_PATH];
        snprintf(bin_path, sizeof(bin_path), "%s/bin", repo_path);
        vfs_create_directory(bin_path);
        
        char scripts_path[MAX_PATH];
        snprintf(scripts_path, sizeof(scripts_path), "%s/scripts", repo_path);
        vfs_create_directory(scripts_path);
        
        char docs_path[MAX_PATH];
        snprintf(docs_path, sizeof(docs_path), "%s/docs", repo_path);
        vfs_create_directory(docs_path);
        
        // Create a basic README using VFS
        char readme_path[MAX_PATH];
        snprintf(readme_path, sizeof(readme_path), "%s/README.md", repo_path);
        FILE* readme = vm_fopen(readme_path, "w");
        if (readme) {
            fprintf(readme, "# %s\n\n", argv[2]);
            fprintf(readme, "Tetra repository created in ZoraVM.\n\n");
            fprintf(readme, "## Structure\n");
            fprintf(readme, "- `bin/` - Executable files\n");
            fprintf(readme, "- `scripts/` - Script files\n");
            fprintf(readme, "- `docs/` - Documentation\n\n");
            fprintf(readme, "Created with Tetra Package Manager v2.0\n");
            fclose(readme);
            printf("Created README.md with project structure\n");
        }
        
        printf("Repository structure created:\n");
        printf("  %s/\n", repo_path);
        printf("  ├── bin/\n");
        printf("  ├── scripts/\n");
        printf("  ├── docs/\n");
        printf("  └── README.md\n");
    } else {
        printf("Failed to create repository '%s' at %s\n", argv[2], repo_path);
        printf("Directory may already exist or path is invalid\n");
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

// List all folders in current location
void tetra_list(int argc, char **argv) {
    const char* location = get_repo_location();
    printf("Repositories in %s:\n", location);
    
    // Use VFS ls command to show directories
    char original_dir[256];
    char* current = vm_getcwd();
    if (current) {
        strncpy(original_dir, current, sizeof(original_dir) - 1);
        original_dir[sizeof(original_dir) - 1] = '\0';
    } else {
        strcpy(original_dir, "/");
    }
    
    // Change to target directory and list
    if (vm_chdir(location) == 0) {
        printf("Tetra repositories and directories:\n");
        vm_ls();  // Use VFS ls to show contents
        
        // Return to original directory
        vm_chdir(original_dir);
    } else {
        printf("Cannot access directory: %s\n", location);
    }
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
    
    const char* base_location = get_repo_location();
    char repo_path[MAX_PATH];
    
    if (strcmp(base_location, "/") == 0) {
        snprintf(repo_path, sizeof(repo_path), "/%s", argv[2]);
    } else {
        snprintf(repo_path, sizeof(repo_path), "%s/%s", base_location, argv[2]);
    }
    
    printf("Removing repository: %s\n", repo_path);
    printf("Are you sure? This will delete all files in the repository. (y/N): ");
    
    char confirmation;
    scanf(" %c", &confirmation);
    
    if (confirmation == 'y' || confirmation == 'Y') {
        if (vm_rmdir(repo_path) == 0) {
            printf("Repository '%s' removed successfully.\n", argv[2]);
        } else {
            printf("Failed to remove repository '%s'.\n", argv[2]);
            printf("Repository may not exist or may contain files.\n");
        }
    } else {
        printf("Repository removal cancelled.\n");
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
        printf("Sets the working directory for Tetra operations.\n");
        printf("Use VFS paths like /projects or /home/user/packages\n");
        return;
    }
    
    // Validate that the directory exists in VFS
    char original_dir[256];
    char* current = vm_getcwd();
    if (current) {
        strncpy(original_dir, current, sizeof(original_dir) - 1);
        original_dir[sizeof(original_dir) - 1] = '\0';
    } else {
        strcpy(original_dir, "/");
    }
    
    if (vm_chdir(argv[2]) == 0) {
        strncpy(session_repo_location, argv[2], MAX_PATH - 1);
        session_repo_location[MAX_PATH - 1] = '\0';
        printf("Tetra working directory set to: %s\n", session_repo_location);
        
        // Return to original directory
        vm_chdir(original_dir);
    } else {
        printf("Error: Directory '%s' does not exist in VFS\n", argv[2]);
        printf("Create it first with: mkdir %s\n", argv[2]);
    }
}

void tetra_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Tetra Package Manager v2.0 for ZoraVM\n");
        printf("Modern package manager with VFS integration\n\n");
        printf("Available commands:\n");
        printf("  init <name> [dir]     - Create a new repository (optionally in specific directory)\n");
        printf("  clone <src> <name>    - Clone a repository (legacy - uses host filesystem)\n");
        printf("  list                  - List all repositories in current directory\n");
        printf("  remove <name>         - Remove a repository (with confirmation)\n");
        printf("  move <name> <path>    - Move a repository (legacy)\n");
        printf("  set-location <path>   - Set repository location for this session\n");
        printf("  pwd                   - Show current repository location\n");
        printf("\nExamples:\n");
        printf("  tetra init myproject          # Create in current directory\n");
        printf("  tetra init myproject /projects # Create in /projects directory\n");
        printf("  tetra list                    # List repositories in current dir\n");
        printf("  tetra set-location /packages  # Set working location\n");
        return;
    }
    
    if (strcmp(argv[1], "init") == 0) {
        tetra_init(argc, argv);
    } else if (strcmp(argv[1], "clone") == 0) {
        printf("Note: clone command uses legacy host filesystem operations\n");
        tetra_clone(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        tetra_list(argc, argv);
    } else if (strcmp(argv[1], "remove") == 0) {
        tetra_remove(argc, argv);
    } else if (strcmp(argv[1], "move") == 0) {
        printf("Note: move command uses legacy host filesystem operations\n");
        tetra_move(argc, argv);
    } else if (strcmp(argv[1], "set-location") == 0) {
        tetra_set_location(argc, argv);
    } else if (strcmp(argv[1], "pwd") == 0) {
        printf("Current Tetra location: %s\n", get_repo_location());
    } else {
        printf("Unknown tetra command: %s\n", argv[1]);
        printf("Type 'tetra' for help.\n");
    }
}
