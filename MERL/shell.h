#ifndef SHELL_H
#define SHELL_H

// Add these includes at the top
#ifdef ZORA_VM_MODE
#include "vfs.h"
#include "syscall.h"
#endif

// Command structure
typedef struct {
    char *name;
    void (*handler)(int argc, char **argv);
    char *description;
} Command;

// Function prototypes
void start_shell(void);
void handle_command(char *command);
void man_command(int argc, char **argv);
void help_command(int argc, char **argv);
void sysinfo_command(int argc, char **argv);
void pwd_command(int argc, char **argv);
void ls_command(int argc, char **argv);
void cd_command(int argc, char **argv);
void mkdir_command(int argc, char **argv);
void rmdir_command(int argc, char **argv);
void touch_command(int argc, char **argv);
void rm_command(int argc, char **argv);
void cp_command(int argc, char **argv);
void mv_command(int argc, char **argv);
void rename_command(int argc, char **argv);
void calendar_command(int argc, char **argv);
void clock_command(int argc, char **argv);
void clear_command(int argc, char **argv);
void echo_command(int argc, char **argv);
void cat_command(int argc, char **argv);
void pull_command(int argc, char **argv);   // Changed from int to void
void flipper_command(int argc, char **argv);
void search_command(int argc, char **argv); // Changed from int to void
void edit_command(int argc, char **argv);
void run_command(int argc, char **argv);

// VM-specific commands
void vm_status_command(int argc, char **argv);
void vm_reboot_command(int argc, char **argv);
void vm_shutdown_command(int argc, char **argv);

// Kernel command wrappers
void fork_wrapper(int argc, char **argv);
void kill_wrapper(int argc, char **argv);
void ps_wrapper(int argc, char **argv);
void read_wrapper(int argc, char **argv);
void write_wrapper(int argc, char **argv);
void route_wrapper(int argc, char **argv);

// Missing file system commands
void more_command(int argc, char **argv);
void less_command(int argc, char **argv);
void head_command(int argc, char **argv);
void tail_command(int argc, char **argv);
void grep_command(int argc, char **argv);

// Missing file permissions commands
void chmod_command(int argc, char **argv);
void chown_command(int argc, char **argv);

// Missing process management commands
void top_command(int argc, char **argv);
void htop_command(int argc, char **argv);
void jobs_command(int argc, char **argv);
void bg_command(int argc, char **argv);
void fg_command(int argc, char **argv);

// Missing system information commands
void date_command(int argc, char **argv);
void df_command(int argc, char **argv);
void du_command(int argc, char **argv);
void uname_command(int argc, char **argv);
void history_command(int argc, char **argv);

// Missing networking commands
void scp_command(int argc, char **argv);

// Missing archiving commands
void tar_command(int argc, char **argv);
void gzip_command(int argc, char **argv);
void gunzip_command(int argc, char **argv);
void zip_command(int argc, char **argv);
void unzip_command(int argc, char **argv);

// Helper functions
void add_to_history(const char* command);

// External references
extern Command command_table[];
extern const int command_table_size;

#endif // SHELL_H