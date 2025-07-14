#ifndef KERNEL_H
#define KERNEL_H

// Kernel-level APIs
int fork_task(const char *desc);
void kill_task(int pid);
void list_tasks();
void kernel_read(const char *filename);
void kernel_write(const char *filename, const char *content);

// Command routing
void route_command(const char *command, int argc, char **argv);

#endif // KERNEL_H