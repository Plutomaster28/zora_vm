#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "package/package_manager.h"

// Enhanced package management commands for ZoraVM

void zpm_command(int argc, char **argv) {
    if (argc < 2) {
        printf("ZoraVM Package Manager (ZPM) - Advanced Package Management\n");
        printf("═══════════════════════════════════════════════════════════════════════════\n");
        printf("Usage: zpm <command> [options] [packages...]\n\n");
        printf("Package Management Commands:\n");
        printf("  install <package>...      Install packages\n");
        printf("  remove <package>...       Remove packages\n");
        printf("  purge <package>...        Remove packages and configuration\n");
        printf("  upgrade [package]         Upgrade packages (or all if no package specified)\n");
        printf("  search <query>            Search for packages\n");
        printf("  show <package>            Show detailed package information\n");
        printf("  list                      List installed packages\n");
        printf("  list-available            List all available packages\n");
        printf("  list-upgradable           List upgradable packages\n");
        printf("\nRepository Management:\n");
        printf("  repo add <name> <url>     Add package repository\n");
        printf("  repo remove <name>        Remove repository\n");
        printf("  repo list                 List repositories\n");
        printf("  repo update               Update repository information\n");
        printf("\nCache and Maintenance:\n");
        printf("  clean                     Clean package cache\n");
        printf("  autoremove               Remove orphaned packages\n");
        printf("  check                     Check for broken dependencies\n");
        printf("  stats                     Show package statistics\n");
        printf("\nAdvanced Features:\n");
        printf("  hold <package>            Hold package at current version\n");
        printf("  unhold <package>          Remove version hold\n");
        printf("  pin <package> <version>   Pin package to specific version\n");
        printf("  snapshot create <name>    Create system snapshot\n");
        printf("  snapshot restore <name>   Restore system snapshot\n");
        printf("  export <file>             Export package list\n");
        printf("  import <file>             Import package list\n");
        printf("\nPackage Development:\n");
        printf("  build <source_dir>        Build package from source\n");
        printf("  create <dir> <name> <ver> Create package from directory\n");
        printf("  verify <package>          Verify package integrity\n");
        printf("  files <package>           List files in package\n");
        printf("  owns <file>               Find package that owns file\n");
        printf("═══════════════════════════════════════════════════════════════════════════\n");
        return;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "install") == 0) {
        if (argc < 3) {
            printf("Usage: zpm install <package>...\n");
            return;
        }
        
        printf("Installing packages...\n");
        for (int i = 2; i < argc; i++) {
            pm_install_package(argv[i]);
        }
    }
    else if (strcmp(command, "remove") == 0) {
        if (argc < 3) {
            printf("Usage: zpm remove <package>...\n");
            return;
        }
        
        printf("Removing packages...\n");
        for (int i = 2; i < argc; i++) {
            pm_remove_package(argv[i]);
        }
    }
    else if (strcmp(command, "purge") == 0) {
        if (argc < 3) {
            printf("Usage: zpm purge <package>...\n");
            return;
        }
        
        printf("Purging packages (removing configuration files)...\n");
        for (int i = 2; i < argc; i++) {
            pm_purge_package(argv[i]);
        }
    }
    else if (strcmp(command, "upgrade") == 0) {
        if (argc == 2) {
            printf("Upgrading all packages...\n");
            pm_upgrade_all_packages();
        } else {
            printf("Upgrading specific packages...\n");
            for (int i = 2; i < argc; i++) {
                pm_upgrade_package(argv[i]);
            }
        }
    }
    else if (strcmp(command, "search") == 0) {
        if (argc < 3) {
            printf("Usage: zpm search <query>\n");
            return;
        }
        
        pm_search_packages(argv[2]);
    }
    else if (strcmp(command, "show") == 0) {
        if (argc < 3) {
            printf("Usage: zpm show <package>\n");
            return;
        }
        
        pm_show_package_info(argv[2]);
    }
    else if (strcmp(command, "list") == 0) {
        pm_list_installed_packages();
    }
    else if (strcmp(command, "list-available") == 0) {
        pm_list_available_packages();
    }
    else if (strcmp(command, "list-upgradable") == 0) {
        pm_list_upgradable_packages();
    }
    else if (strcmp(command, "repo") == 0) {
        if (argc < 3) {
            printf("Usage: zpm repo <add|remove|list|update>\n");
            return;
        }
        
        char* repo_cmd = argv[2];
        
        if (strcmp(repo_cmd, "add") == 0) {
            if (argc < 5) {
                printf("Usage: zpm repo add <name> <url> [distribution] [component]\n");
                return;
            }
            
            char* name = argv[3];
            char* url = argv[4];
            char* distribution = (argc > 5) ? argv[5] : "stable";
            char* component = (argc > 6) ? argv[6] : "main";
            
            pm_add_repository(name, url, distribution, component);
        }
        else if (strcmp(repo_cmd, "remove") == 0) {
            if (argc < 4) {
                printf("Usage: zpm repo remove <name>\n");
                return;
            }
            
            pm_remove_repository(argv[3]);
        }
        else if (strcmp(repo_cmd, "list") == 0) {
            pm_list_repositories();
        }
        else if (strcmp(repo_cmd, "update") == 0) {
            printf("Updating repository information...\n");
            pm_update_repositories();
        }
        else {
            printf("Unknown repo command: %s\n", repo_cmd);
        }
    }
    else if (strcmp(command, "clean") == 0) {
        printf("Cleaning package cache...\n");
        pm_clean_package_cache();
    }
    else if (strcmp(command, "autoremove") == 0) {
        printf("Removing orphaned packages...\n");
        pm_list_orphaned_packages();
    }
    else if (strcmp(command, "check") == 0) {
        printf("Checking for broken dependencies...\n");
        pm_validate_dependencies();
    }
    else if (strcmp(command, "stats") == 0) {
        pm_show_package_statistics();
    }
    else if (strcmp(command, "hold") == 0) {
        if (argc < 3) {
            printf("Usage: zpm hold <package>\n");
            return;
        }
        
        pm_hold_package(argv[2]);
    }
    else if (strcmp(command, "unhold") == 0) {
        if (argc < 3) {
            printf("Usage: zpm unhold <package>\n");
            return;
        }
        
        pm_unhold_package(argv[2]);
    }
    else if (strcmp(command, "pin") == 0) {
        if (argc < 4) {
            printf("Usage: zpm pin <package> <version>\n");
            return;
        }
        
        pm_pin_package_version(argv[2], argv[3]);
    }
    else if (strcmp(command, "snapshot") == 0) {
        if (argc < 3) {
            printf("Usage: zpm snapshot <create|restore> <name>\n");
            return;
        }
        
        char* snap_cmd = argv[2];
        
        if (strcmp(snap_cmd, "create") == 0) {
            if (argc < 4) {
                printf("Usage: zpm snapshot create <name>\n");
                return;
            }
            
            pm_create_snapshot(argv[3]);
        }
        else if (strcmp(snap_cmd, "restore") == 0) {
            if (argc < 4) {
                printf("Usage: zpm snapshot restore <name>\n");
                return;
            }
            
            pm_restore_snapshot(argv[3]);
        }
        else {
            printf("Unknown snapshot command: %s\n", snap_cmd);
        }
    }
    else if (strcmp(command, "export") == 0) {
        if (argc < 3) {
            printf("Usage: zpm export <filename>\n");
            return;
        }
        
        pm_export_package_list(argv[2]);
    }
    else if (strcmp(command, "import") == 0) {
        if (argc < 3) {
            printf("Usage: zpm import <filename>\n");
            return;
        }
        
        pm_import_package_list(argv[2]);
    }
    else if (strcmp(command, "build") == 0) {
        if (argc < 3) {
            printf("Usage: zpm build <source_directory>\n");
            return;
        }
        
        printf("Building package from source: %s\n", argv[2]);
        pm_build_package_from_source(argv[2], "build.sh");
    }
    else if (strcmp(command, "create") == 0) {
        if (argc < 5) {
            printf("Usage: zpm create <source_dir> <package_name> <version>\n");
            return;
        }
        
        pm_create_package(argv[2], argv[3], argv[4]);
    }
    else if (strcmp(command, "verify") == 0) {
        if (argc < 3) {
            printf("Usage: zpm verify <package>\n");
            return;
        }
        
        pm_verify_package_integrity(argv[2]);
    }
    else if (strcmp(command, "files") == 0) {
        if (argc < 3) {
            printf("Usage: zpm files <package>\n");
            return;
        }
        
        pm_list_package_files(argv[2]);
    }
    else if (strcmp(command, "owns") == 0) {
        if (argc < 3) {
            printf("Usage: zpm owns <filename>\n");
            return;
        }
        
        pm_which_package_owns_file(argv[2]);
    }
    else {
        printf("Unknown command: %s\n", command);
        printf("Run 'zpm' for help\n");
    }
}

// Simplified package commands for easier access
void pkg_install_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pkg-install <package>...\n");
        printf("Quick package installation command\n");
        return;
    }
    
    printf("Quick install mode:\n");
    for (int i = 1; i < argc; i++) {
        printf("Installing %s...\n", argv[i]);
        pm_install_package(argv[i]);
    }
}

void pkg_remove_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pkg-remove <package>...\n");
        printf("Quick package removal command\n");
        return;
    }
    
    printf("Quick remove mode:\n");
    for (int i = 1; i < argc; i++) {
        printf("Removing %s...\n", argv[i]);
        pm_remove_package(argv[i]);
    }
}

void pkg_search_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pkg-search <query>\n");
        printf("Quick package search command\n");
        return;
    }
    
    pm_search_packages(argv[1]);
}

void pkg_list_command(int argc, char **argv) {
    (void)argc; (void)argv;
    pm_list_installed_packages();
}

void pkg_upgrade_command(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Upgrading all packages...\n");
    pm_upgrade_all_packages();
}

void pkg_info_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: pkg-info <package>\n");
        printf("Quick package information command\n");
        return;
    }
    
    pm_show_package_info(argv[1]);
}

// Repository management commands
void repo_add_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: repo-add <name> <url> [distribution] [component]\n");
        printf("Add a package repository\n");
        printf("Examples:\n");
        printf("  repo-add myrepo https://packages.example.com/debian stable main\n");
        printf("  repo-add testing https://test.repo.com unstable universe\n");
        return;
    }
    
    char* name = argv[1];
    char* url = argv[2];
    char* distribution = (argc > 3) ? argv[3] : "stable";
    char* component = (argc > 4) ? argv[4] : "main";
    
    pm_add_repository(name, url, distribution, component);
}

void repo_list_command(int argc, char **argv) {
    (void)argc; (void)argv;
    pm_list_repositories();
}

void repo_update_command(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Updating all repositories...\n");
    pm_update_repositories();
}

// Package management utilities
void pkg_clean_command(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Cleaning package cache and temporary files...\n");
    pm_clean_package_cache();
}

void pkg_stats_command(int argc, char **argv) {
    (void)argc; (void)argv;
    pm_show_package_statistics();
}

void pkg_check_command(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Checking system integrity and dependencies...\n");
    pm_validate_dependencies();
    pm_audit_installed_packages();
}