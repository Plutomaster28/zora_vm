#ifndef USER_H
#define USER_H

// Command prototypes for user management
void whoami_command(int argc, char **argv);
void useradd_command(int argc, char **argv);
void login_command(int argc, char **argv);
void logout_command(int argc, char **argv);
void passwd_command(int argc, char **argv);

// Helper function prototypes for persistence
void load_users();
void save_users();

#endif // USER_H