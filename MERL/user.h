#ifndef USER_H
#define USER_H

// Global user state variables (defined in user.c)
extern char current_user[50];
extern int is_logged_in;
extern int user_count;

// Command prototypes for user management
void whoami_command(int argc, char **argv);
void useradd_command(int argc, char **argv);
void login_command(int argc, char **argv);
void logout_command(int argc, char **argv);
void passwd_command(int argc, char **argv);
void su_command(int argc, char **argv);
void users_command(int argc, char **argv);
void setup_root_command(int argc, char **argv);

// Security functions
int secure_password_input(char* password, int max_len, const char* prompt);
void simple_hash(const char* input, char* output);

// Helper function prototypes for persistence
void load_users();
void save_users();

#endif // USER_H