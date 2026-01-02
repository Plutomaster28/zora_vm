#ifndef KERNEL_SYSCALL_TABLE_H
#define KERNEL_SYSCALL_TABLE_H

#include <stdint.h>

// System call numbers (following Linux convention)
#define SYS_exit            1
#define SYS_fork            2
#define SYS_read            3
#define SYS_write           4
#define SYS_open            5
#define SYS_close           6
#define SYS_wait4           7
#define SYS_creat           8
#define SYS_link            9
#define SYS_unlink          10
#define SYS_execve          11
#define SYS_chdir           12
#define SYS_time            13
#define SYS_mknod           14
#define SYS_chmod           15
#define SYS_lchown          16
#define SYS_stat            18
#define SYS_lseek           19
#define SYS_getpid          20
#define SYS_mount           21
#define SYS_umount          22
#define SYS_setuid          23
#define SYS_getuid          24
#define SYS_stime           25
#define SYS_ptrace          26
#define SYS_alarm           27
#define SYS_fstat           28
#define SYS_pause           29
#define SYS_utime           30
#define SYS_access          33
#define SYS_sync            36
#define SYS_kill            37
#define SYS_rename          38
#define SYS_mkdir           39
#define SYS_rmdir           40
#define SYS_dup             41
#define SYS_pipe            42
#define SYS_times           43
#define SYS_brk             45
#define SYS_setgid          46
#define SYS_getgid          47
#define SYS_signal          48
#define SYS_geteuid         49
#define SYS_getegid         50
#define SYS_acct            51
#define SYS_umount2         52
#define SYS_ioctl           54
#define SYS_fcntl           55
#define SYS_setpgid         57
#define SYS_umask           60
#define SYS_chroot          61
#define SYS_ustat           62
#define SYS_dup2            63
#define SYS_getppid         64
#define SYS_getpgrp         65
#define SYS_setsid          66
#define SYS_sigaction       67
#define SYS_setreuid        70
#define SYS_setregid        71
#define SYS_sigsuspend      72
#define SYS_sigpending      73
#define SYS_sethostname     74
#define SYS_setrlimit       75
#define SYS_getrlimit       76
#define SYS_getrusage       77
#define SYS_gettimeofday    78
#define SYS_settimeofday    79
#define SYS_getgroups       80
#define SYS_setgroups       81
#define SYS_select          82
#define SYS_symlink         83
#define SYS_readlink        85
#define SYS_uselib          86
#define SYS_swapon          87
#define SYS_reboot          88
#define SYS_readdir         89
#define SYS_mmap            90
#define SYS_munmap          91
#define SYS_truncate        92
#define SYS_ftruncate       93
#define SYS_fchmod          94
#define SYS_fchown          95
#define SYS_getpriority     96
#define SYS_setpriority     97
#define SYS_statfs          99
#define SYS_fstatfs         100

// Networking syscalls
#define SYS_socket          200
#define SYS_bind            201
#define SYS_connect         202
#define SYS_listen          203
#define SYS_accept          204
#define SYS_getsockname     205
#define SYS_getpeername     206
#define SYS_socketpair      207
#define SYS_send            208
#define SYS_recv            209
#define SYS_sendto          210
#define SYS_recvfrom        211
#define SYS_shutdown        212
#define SYS_setsockopt      213
#define SYS_getsockopt      214
#define SYS_sendmsg         215
#define SYS_recvmsg         216

// Total number of syscalls
#define MAX_SYSCALLS        256

// Syscall handler function type
typedef int (*syscall_handler_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, 
                                 uint32_t arg4, uint32_t arg5);

// Syscall table entry
typedef struct {
    syscall_handler_t handler;
    const char* name;
    int arg_count;
} SyscallTableEntry;

// Initialize syscall table
void syscall_table_init(void);

// Dispatch syscall
int syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, 
                     uint32_t arg3, uint32_t arg4, uint32_t arg5);

// Syscall implementations
int sys_exit(uint32_t status, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_fork(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5);
int sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5);
int sys_open(uint32_t pathname, uint32_t flags, uint32_t mode, uint32_t arg4, uint32_t arg5);
int sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_getuid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_brk(uint32_t addr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t fd);
int sys_munmap(uint32_t addr, uint32_t length, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_socket(uint32_t domain, uint32_t type, uint32_t protocol, uint32_t arg4, uint32_t arg5);
int sys_bind(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5);
int sys_connect(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5);
int sys_listen(uint32_t sockfd, uint32_t backlog, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int sys_accept(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5);

#endif // KERNEL_SYSCALL_TABLE_H
