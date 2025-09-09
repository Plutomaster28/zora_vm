#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

// Windows-specific includes for secure password input
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

#define MAX_USERS 100
#define USERNAME_LEN 50
#define PASSWORD_LEN 50
#define HASH_LEN 64

typedef struct {
    char username[USERNAME_LEN];
    char password_hash[HASH_LEN];  // Store hashed password instead of plaintext
} User;

static User user_list[MAX_USERS];
int user_count = 0;

// Global variables for user management (accessible from other modules)
char current_user[50] = "guest"; // Default user
int is_logged_in = 0;            // Login status

// Simple hash function for passwords (basic security)
// In production, use proper cryptographic hash like bcrypt or Argon2
void simple_hash(const char* input, char* output) {
    unsigned long hash = 5381;
    int c;
    const char* str = input;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    // Convert to hex string
    snprintf(output, HASH_LEN, "%lx_%s_zora", hash, "salt");
}

// Secure password input function (Windows-specific)
int secure_password_input(char* password, int max_len, const char* prompt) {
    printf("%s", prompt);
    fflush(stdout);
    
    int i = 0;
    int ch;
    
#ifdef _WIN32
    // Windows secure input - hide characters
    while (i < max_len - 1) {
        ch = _getch();
        
        if (ch == '\r' || ch == '\n') {  // Enter key
            break;
        } else if (ch == '\b' || ch == 127) {  // Backspace
            if (i > 0) {
                i--;
                printf("\b \b");  // Erase the asterisk
            }
        } else if (ch >= 32 && ch <= 126) {  // Printable characters
            password[i++] = ch;
            printf("*");  // Show asterisk instead of character
        }
    }
#else
    // Fallback for non-Windows (basic implementation)
    if (fgets(password, max_len, stdin)) {
        i = strlen(password);
        if (i > 0 && password[i-1] == '\n') {
            password[i-1] = '\0';
            i--;
        }
    }
#endif
    
    password[i] = '\0';
    printf("\n");
    return i;
}

// Command implementations

void whoami_command(int argc, char **argv) {
    printf("Current user: %s\n", current_user);
}

void useradd_command(int argc, char **argv) {
    // Check if current user has permission to create users (root or setup mode)
    int is_root = (strcmp(current_user, "root") == 0);
    int is_setup_mode = (user_count == 0); // No users exist yet
    
    if (!is_root && !is_setup_mode) {
        printf("useradd: Permission denied - only root can create users\n");
        printf("Use 'su root' to switch to root, or if no root exists, run 'setup-root'\n");
        return;
    }
    
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    char password_hash[HASH_LEN];
    
    // Get username
    if (argc >= 2) {
        strncpy(username, argv[1], USERNAME_LEN - 1);
        username[USERNAME_LEN - 1] = '\0';
    } else {
        printf("Username: ");
        if (!fgets(username, USERNAME_LEN, stdin)) {
            printf("Error reading username.\n");
            return;
        }
        // Remove newline
        username[strcspn(username, "\n")] = '\0';
    }
    
    // Check for duplicate
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, username) == 0) {
            printf("User '%s' already exists.\n", username);
            return;
        }
    }
    
    if (user_count >= MAX_USERS) {
        printf("User limit reached.\n");
        return;
    }
    
    // Get password securely
    if (argc >= 3) {
        // Command line password (less secure, for backwards compatibility)
        printf("Warning: Command-line passwords are visible in process lists.\n");
        printf("Consider using 'useradd %s' without password for secure input.\n", username);
        strncpy(password, argv[2], PASSWORD_LEN - 1);
        password[PASSWORD_LEN - 1] = '\0';
    } else {
        // Secure password input
        if (secure_password_input(password, PASSWORD_LEN, "Password: ") < 1) {
            printf("Password cannot be empty.\n");
            return;
        }
        
        // Confirm password
        char confirm_password[PASSWORD_LEN];
        if (secure_password_input(confirm_password, PASSWORD_LEN, "Confirm password: ") < 1) {
            printf("Password confirmation failed.\n");
            return;
        }
        
        if (strcmp(password, confirm_password) != 0) {
            printf("Passwords do not match.\n");
            return;
        }
    }
    
    // Hash the password
    simple_hash(password, password_hash);
    
    // Add user
    strncpy(user_list[user_count].username, username, USERNAME_LEN - 1);
    user_list[user_count].username[USERNAME_LEN - 1] = '\0';
    strncpy(user_list[user_count].password_hash, password_hash, HASH_LEN - 1);
    user_list[user_count].password_hash[HASH_LEN - 1] = '\0';
    user_count++;
    save_users();
    
    printf("User '%s' added successfully with secure password.\n", username);
}

void login_command(int argc, char **argv) {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    char password_hash[HASH_LEN];
    
    // Get username
    if (argc >= 2) {
        strncpy(username, argv[1], USERNAME_LEN - 1);
        username[USERNAME_LEN - 1] = '\0';
    } else {
        printf("Username: ");
        if (!fgets(username, USERNAME_LEN, stdin)) {
            printf("Error reading username.\n");
            return;
        }
        // Remove newline
        username[strcspn(username, "\n")] = '\0';
    }
    
    // Get password securely
    if (argc >= 3) {
        // Command line password (less secure, for backwards compatibility)
        printf("Warning: Command-line passwords are visible in process lists.\n");
        printf("Consider using 'login %s' without password for secure input.\n", username);
        strncpy(password, argv[2], PASSWORD_LEN - 1);
        password[PASSWORD_LEN - 1] = '\0';
    } else {
        // Secure password input
        if (secure_password_input(password, PASSWORD_LEN, "Password: ") < 1) {
            printf("Password cannot be empty.\n");
            return;
        }
    }
    
    // Hash the entered password
    simple_hash(password, password_hash);
    
    // Check credentials
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, username) == 0 &&
            strcmp(user_list[i].password_hash, password_hash) == 0) {
            strncpy(current_user, username, sizeof(current_user) - 1);
            current_user[sizeof(current_user) - 1] = '\0';
            is_logged_in = 1;
            printf("Successfully logged in as '%s'.\n", current_user);
            return;
        }
    }
    printf("Invalid username or password.\n");
}

void logout_command(int argc, char **argv) {
    if (!is_logged_in) {
        printf("No user is currently logged in.\n");
        return;
    }
    printf("User '%s' logged out.\n", current_user);
    strncpy(current_user, "guest", sizeof(current_user) - 1);
    current_user[sizeof(current_user) - 1] = '\0';
    is_logged_in = 0;
    
    // Update VFS globals back to guest
    extern char vfs_current_user[64];
    extern char vfs_current_group[64];
    extern int vfs_is_root;
    strncpy(vfs_current_user, "guest", sizeof(vfs_current_user) - 1);
    vfs_current_user[sizeof(vfs_current_user) - 1] = '\0';
    strncpy(vfs_current_group, "users", sizeof(vfs_current_group) - 1);
    vfs_current_group[sizeof(vfs_current_group) - 1] = '\0';
    vfs_is_root = 0;
}

void passwd_command(int argc, char **argv) {
    if (!is_logged_in) {
        printf("You must be logged in to change the password.\n");
        return;
    }
    
    char old_password[PASSWORD_LEN];
    char new_password[PASSWORD_LEN];
    char old_password_hash[HASH_LEN];
    char new_password_hash[HASH_LEN];
    
    // Verify current password first
    if (secure_password_input(old_password, PASSWORD_LEN, "Current password: ") < 1) {
        printf("Password cannot be empty.\n");
        return;
    }
    
    simple_hash(old_password, old_password_hash);
    
    // Find and verify current user
    int user_index = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, current_user) == 0) {
            if (strcmp(user_list[i].password_hash, old_password_hash) == 0) {
                user_index = i;
                break;
            } else {
                printf("Current password is incorrect.\n");
                return;
            }
        }
    }
    
    if (user_index == -1) {
        printf("Current user not found.\n");
        return;
    }
    
    // Get new password
    if (secure_password_input(new_password, PASSWORD_LEN, "New password: ") < 1) {
        printf("Password cannot be empty.\n");
        return;
    }
    
    // Confirm new password
    char confirm_password[PASSWORD_LEN];
    if (secure_password_input(confirm_password, PASSWORD_LEN, "Confirm new password: ") < 1) {
        printf("Password confirmation failed.\n");
        return;
    }
    
    if (strcmp(new_password, confirm_password) != 0) {
        printf("New passwords do not match.\n");
        return;
    }
    
    // Hash and update password
    simple_hash(new_password, new_password_hash);
    strncpy(user_list[user_index].password_hash, new_password_hash, HASH_LEN - 1);
    user_list[user_index].password_hash[HASH_LEN - 1] = '\0';
    save_users();
    
    printf("Password successfully updated for user '%s'.\n", current_user);
}

void su_command(int argc, char **argv) {
    char target_user[USERNAME_LEN] = "root";
    
    // Parse arguments
    if (argc > 1) {
        strncpy(target_user, argv[1], USERNAME_LEN - 1);
        target_user[USERNAME_LEN - 1] = '\0';
    }
    
    // Special case for guest user - no password required for su guest
    if (strcmp(target_user, "guest") == 0) {
        strncpy(current_user, "guest", sizeof(current_user) - 1);
        current_user[sizeof(current_user) - 1] = '\0';
        is_logged_in = 1;
        
        // Update VFS globals for guest
        extern char vfs_current_user[64];
        extern char vfs_current_group[64];
        extern int vfs_is_root;
        strncpy(vfs_current_user, "guest", sizeof(vfs_current_user) - 1);
        vfs_current_user[sizeof(vfs_current_user) - 1] = '\0';
        strncpy(vfs_current_group, "users", sizeof(vfs_current_group) - 1);
        vfs_current_group[sizeof(vfs_current_group) - 1] = '\0';
        vfs_is_root = 0;
        
        printf("Switched to user '%s'\n", current_user);
        return;
    }
    
    // For root and other users, check if they exist and authenticate
    char password[PASSWORD_LEN];
    char password_hash[HASH_LEN];
    
    if (secure_password_input(password, PASSWORD_LEN, "Password: ") < 1) {
        printf("Authentication failed.\n");
        return;
    }
    
    simple_hash(password, password_hash);
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, target_user) == 0) {
            if (strcmp(user_list[i].password_hash, password_hash) == 0) {
                strncpy(current_user, target_user, sizeof(current_user) - 1);
                current_user[sizeof(current_user) - 1] = '\0';
                is_logged_in = 1;
                
                // Update VFS globals
                extern char vfs_current_user[64];
                extern char vfs_current_group[64];
                extern int vfs_is_root;
                strncpy(vfs_current_user, target_user, sizeof(vfs_current_user) - 1);
                vfs_current_user[sizeof(vfs_current_user) - 1] = '\0';
                
                if (strcmp(target_user, "root") == 0) {
                    strncpy(vfs_current_group, "root", sizeof(vfs_current_group) - 1);
                    vfs_current_group[sizeof(vfs_current_group) - 1] = '\0';
                    vfs_is_root = 1;
                    printf("Switched to root user\n");
                } else {
                    strncpy(vfs_current_group, "users", sizeof(vfs_current_group) - 1);
                    vfs_current_group[sizeof(vfs_current_group) - 1] = '\0';
                    vfs_is_root = 0;
                    printf("Switched to user '%s'\n", current_user);
                }
                return;
            }
        }
    }
    printf("Authentication failed.\n");
}

void users_command(int argc, char **argv) {
    printf("Users on the system:\n");
    if (user_count == 0) {
        printf("  No users found. Use 'setup-root' to create the root user.\n");
        return;
    }
    
    for (int i = 0; i < user_count; i++) {
        printf("  %s", user_list[i].username);
        if (strcmp(user_list[i].username, current_user) == 0) {
            printf(" (current)");
        }
        if (strcmp(user_list[i].username, "root") == 0) {
            printf(" (administrator)");
        }
        printf("\n");
    }
    printf("Total users: %d\n", user_count);
}

void setup_root_command(int argc, char **argv) {
    // Check if root already exists
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, "root") == 0) {
            printf("Root user already exists. Use 'passwd' as root to change the password.\n");
            return;
        }
    }
    
    printf("=== Root User Setup ===\n");
    printf("Setting up the root (administrator) user for the first time.\n");
    printf("The root user has full system privileges.\n\n");
    
    if (user_count >= MAX_USERS) {
        printf("User limit reached.\n");
        return;
    }
    
    char password[PASSWORD_LEN];
    char confirm_password[PASSWORD_LEN];
    char password_hash[HASH_LEN];
    
    // Get root password with confirmation
    if (secure_password_input(password, PASSWORD_LEN, "Set root password: ") < 1) {
        printf("Password cannot be empty.\n");
        return;
    }
    
    if (secure_password_input(confirm_password, PASSWORD_LEN, "Confirm root password: ") < 1) {
        printf("Password confirmation failed.\n");
        return;
    }
    
    if (strcmp(password, confirm_password) != 0) {
        printf("Passwords do not match.\n");
        return;
    }
    
    // Create root user
    simple_hash(password, password_hash);
    strncpy(user_list[user_count].username, "root", USERNAME_LEN - 1);
    user_list[user_count].username[USERNAME_LEN - 1] = '\0';
    strncpy(user_list[user_count].password_hash, password_hash, HASH_LEN - 1);
    user_list[user_count].password_hash[HASH_LEN - 1] = '\0';
    user_count++;
    save_users();
    
    printf("Root user created successfully!\n");
    printf("You can now use 'su root' to switch to root user.\n");
}

void load_users() {
    FILE *file = fopen("users.txt", "r");
    if (!file) return;
    user_count = 0;
    while (fscanf(file, "%49[^:]:%63[^\n]\n", user_list[user_count].username, user_list[user_count].password_hash) == 2) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(file);
}

void save_users() {
    FILE *file = fopen("users.txt", "w");
    if (!file) return;
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s:%s\n", user_list[i].username, user_list[i].password_hash);
    }
    fclose(file);
}