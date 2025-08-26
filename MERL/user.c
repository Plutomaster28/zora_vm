#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

#define MAX_USERS 100
#define USERNAME_LEN 50
#define PASSWORD_LEN 50

typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
} User;

static User user_list[MAX_USERS];
static int user_count = 0;

// Global variables for user management (accessible from other modules)
char current_user[50] = "guest"; // Default user
int is_logged_in = 0;            // Login status

// Command implementations

void whoami_command(int argc, char **argv) {
    printf("Current user: %s\n", current_user);
}

void useradd_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: useradd <username> <password>\n");
        return;
    }
    // Check for duplicate
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, argv[1]) == 0) {
            printf("User '%s' already exists.\n", argv[1]);
            return;
        }
    }
    if (user_count >= MAX_USERS) {
        printf("User limit reached.\n");
        return;
    }
    strncpy(user_list[user_count].username, argv[1], USERNAME_LEN - 1);
    user_list[user_count].username[USERNAME_LEN - 1] = '\0';
    strncpy(user_list[user_count].password, argv[2], PASSWORD_LEN - 1);
    user_list[user_count].password[PASSWORD_LEN - 1] = '\0';
    user_count++;
    save_users();
    printf("User '%s' added successfully.\n", argv[1]);
}

void login_command(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: login <username> <password>\n");
        return;
    }
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, argv[1]) == 0 &&
            strcmp(user_list[i].password, argv[2]) == 0) {
            strncpy(current_user, argv[1], sizeof(current_user) - 1);
            current_user[sizeof(current_user) - 1] = '\0';
            is_logged_in = 1;
            printf("Logged in as '%s'.\n", current_user);
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
}

void passwd_command(int argc, char **argv) {
    if (!is_logged_in) {
        printf("You must be logged in to change the password.\n");
        return;
    }
    if (argc < 2) {
        printf("Usage: passwd <newpassword>\n");
        return;
    }
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_list[i].username, current_user) == 0) {
            strncpy(user_list[i].password, argv[1], PASSWORD_LEN - 1);
            user_list[i].password[PASSWORD_LEN - 1] = '\0';
            save_users();
            printf("Password updated for user '%s'.\n", current_user);
            return;
        }
    }
    printf("Current user not found.\n");
}

void load_users() {
    FILE *file = fopen("users.txt", "r");
    if (!file) return;
    user_count = 0;
    while (fscanf(file, "%49[^:]:%49[^\n]\n", user_list[user_count].username, user_list[user_count].password) == 2) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(file);
}

void save_users() {
    FILE *file = fopen("users.txt", "w");
    if (!file) return;
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s:%s\n", user_list[i].username, user_list[i].password);
    }
    fclose(file);
}