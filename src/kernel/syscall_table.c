#include "kernel/syscall_table.h"
#include "kernel/privilege.h"
#include "kernel/network_stack.h"
#include "system/process.h"
#include "vfs/vfs.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// File descriptor table (0=stdin, 1=stdout, 2=stderr)
#define MAX_FDS 256
typedef struct {
    int in_use;
    char* path;
    void* data;
    size_t size;
    size_t position;
    int flags;  // O_RDONLY, O_WRONLY, O_RDWR
} FileDescriptor;

static FileDescriptor g_fd_table[MAX_FDS] = {0};
static int next_fd = 3;  // Start after stdin/stdout/stderr

// Global syscall table
static SyscallTableEntry g_syscall_table[MAX_SYSCALLS];

// Syscall implementations

int sys_exit(uint32_t status, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] exit(%d)\n", status);
    // Would terminate current process here
    return 0;
}

int sys_fork(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] fork() - creating child process\n");
    // Would create new process here
    return 0;  // Return child PID
}

int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] read(fd=%d, count=%d)\n", fd, count);
    
    if (!buf) {
        printf("[SYSCALL] read: invalid buffer\n");
        return -1;
    }
    
    // Handle stdin (fd=0)
    if (fd == 0) {
        // Would read from terminal input
        printf("[SYSCALL] read: stdin not yet implemented\n");
        return 0;
    }
    
    // Validate file descriptor
    if (fd < 3 || fd >= MAX_FDS || !g_fd_table[fd].in_use) {
        printf("[SYSCALL] read: invalid fd\n");
        return -1;
    }
    
    FileDescriptor* file = &g_fd_table[fd];
    
    // Check if opened for reading
    if ((file->flags & 0x03) == 1) {  // O_WRONLY
        printf("[SYSCALL] read: file not open for reading\n");
        return -1;
    }
    
    // Calculate how much to read
    size_t available = file->size - file->position;
    size_t to_read = (count < available) ? count : available;
    
    if (to_read == 0) {
        return 0;  // EOF
    }
    
    // Copy data to buffer
    memcpy((void*)buf, (char*)file->data + file->position, to_read);
    file->position += to_read;
    
    printf("[SYSCALL] read: read %zu bytes (pos now %zu/%zu)\n", 
           to_read, file->position, file->size);
    return (int)to_read;
}

int sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t arg4, uint32_t arg5) {
    if (!buf) {
        printf("[SYSCALL] write: invalid buffer\n");
        return -1;
    }
    
    // Handle stdout/stderr (fd=1,2)
    if (fd == 1 || fd == 2) {
        const char* data = (const char*)buf;
        for (size_t i = 0; i < count; i++) {
            putchar(data[i]);
        }
        fflush(stdout);
        return count;
    }
    
    printf("[SYSCALL] write(fd=%d, count=%d)\n", fd, count);
    
    // Validate file descriptor
    if (fd < 3 || fd >= MAX_FDS || !g_fd_table[fd].in_use) {
        printf("[SYSCALL] write: invalid fd\n");
        return -1;
    }
    
    FileDescriptor* file = &g_fd_table[fd];
    
    // Check if opened for writing
    if ((file->flags & 0x03) == 0) {  // O_RDONLY
        printf("[SYSCALL] write: file not open for writing\n");
        return -1;
    }
    
    // Expand buffer if needed
    size_t new_pos = file->position + count;
    if (new_pos > file->size) {
        // Reallocate buffer
        void* new_data = realloc(file->data, new_pos);
        if (!new_data) {
            printf("[SYSCALL] write: allocation failed\n");
            return -1;
        }
        file->data = new_data;
        file->size = new_pos;
    }
    
    // Write data
    memcpy((char*)file->data + file->position, (const void*)buf, count);
    file->position += count;
    
    // Write back to VFS
    if (vfs_write_file(file->path, file->data, file->size) != 0) {
        printf("[SYSCALL] write: failed to write to VFS\n");
        return -1;
    }
    
    printf("[SYSCALL] write: wrote %d bytes (pos now %zu/%zu)\n", 
           count, file->position, file->size);
    return count;
}

int sys_open(uint32_t pathname, uint32_t flags, uint32_t mode, uint32_t arg4, uint32_t arg5) {
    const char* path_str = (const char*)pathname;
    if (!path_str) return -1;
    
    printf("[SYSCALL] open(path=%s, flags=0x%X, mode=0x%X)\n", path_str, flags, mode);
    
    // Find free file descriptor
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (!g_fd_table[i].in_use) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        printf("[SYSCALL] open: too many open files\n");
        return -1;
    }
    
    // Check if file exists in VFS
    VNode* node = vfs_find_node(path_str);
    if (!node) {
        // If O_CREAT flag, create the file
        if (flags & 0x40) {  // O_CREAT = 0x40
            if (vfs_create_file(path_str) != 0) {
                printf("[SYSCALL] open: failed to create file\n");
                return -1;
            }
            node = vfs_find_node(path_str);
            if (!node) return -1;
        } else {
            printf("[SYSCALL] open: file not found\n");
            return -1;
        }
    }
    
    // Check if it's a directory
    if (node->is_directory) {
        printf("[SYSCALL] open: is a directory\n");
        return -1;
    }
    
    // Read file contents
    void* data = NULL;
    size_t size = 0;
    if (vfs_read_file(path_str, &data, &size) != 0) {
        printf("[SYSCALL] open: failed to read file\n");
        return -1;
    }
    
    // Setup file descriptor
    g_fd_table[fd].in_use = 1;
    g_fd_table[fd].path = strdup(path_str);
    g_fd_table[fd].data = data;
    g_fd_table[fd].size = size;
    g_fd_table[fd].position = 0;
    g_fd_table[fd].flags = flags;
    
    printf("[SYSCALL] open: opened fd=%d (size=%zu bytes)\n", fd, size);
    return fd;
}

int sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] close(fd=%d)\n", fd);
    
    if (fd < 3 || fd >= MAX_FDS) {
        printf("[SYSCALL] close: invalid fd\n");
        return -1;
    }
    
    if (!g_fd_table[fd].in_use) {
        printf("[SYSCALL] close: fd not open\n");
        return -1;
    }
    
    // Cleanup file descriptor
    if (g_fd_table[fd].path) {
        free(g_fd_table[fd].path);
        g_fd_table[fd].path = NULL;
    }
    
    // Note: don't free data as it belongs to VFS
    g_fd_table[fd].data = NULL;
    g_fd_table[fd].size = 0;
    g_fd_table[fd].position = 0;
    g_fd_table[fd].flags = 0;
    g_fd_table[fd].in_use = 0;
    
    printf("[SYSCALL] close: closed fd=%d\n", fd);
    return 0;
}

int sys_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    // Return current process ID
    return 1;  // Would get from scheduler
}

int sys_getuid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    // Return current user ID
    return 0;  // Would get from process structure
}

int sys_brk(uint32_t addr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] brk(addr=0x%08X)\n", addr);
    // Would adjust process heap
    return addr;  // Return new break address
}

int sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t fd) {
    printf("[SYSCALL] mmap(addr=0x%08X, length=%d, prot=0x%X, flags=0x%X, fd=%d)\n",
           addr, length, prot, flags, fd);
    
    // Validate length
    if (length == 0 || length > memory_get_free()) {
        printf("[SYSCALL] mmap: invalid length or insufficient memory\n");
        return 0;  // Return NULL
    }
    
    // For MAP_ANONYMOUS (no file backing)
    if (flags & 0x20) {  // MAP_ANONYMOUS
        // Allocate memory from pool
        static uint32_t mmap_base = 0x40000000;  // Start at 1GB
        
        // Align length to page size (4KB)
        uint32_t aligned_length = (length + 0xFFF) & ~0xFFF;
        
        // Check if we have space
        if (mmap_base + aligned_length > 0x60000000) {  // 1.5GB limit
            printf("[SYSCALL] mmap: address space exhausted\n");
            return 0;
        }
        
        // Map the memory
        void* mapped = memory_map(mmap_base, aligned_length);
        if (!mapped) {
            printf("[SYSCALL] mmap: mapping failed\n");
            return 0;
        }
        
        // Initialize to zero if requested
        if (flags & 0x20) {
            memset(mapped, 0, aligned_length);
        }
        
        uint32_t result = mmap_base;
        mmap_base += aligned_length;
        
        printf("[SYSCALL] mmap: mapped %d bytes at 0x%08X\n", aligned_length, result);
        return result;
    }
    
    // File-backed mapping
    if (fd >= 3 && fd < MAX_FDS && g_fd_table[fd].in_use) {
        FileDescriptor* file = &g_fd_table[fd];
        
        // Allocate memory
        static uint32_t file_mmap_base = 0x30000000;  // Start at 768MB
        
        uint32_t map_size = (length < file->size) ? length : file->size;
        void* mapped = memory_map(file_mmap_base, map_size);
        
        if (mapped) {
            // Copy file data to mapped region
            memcpy(mapped, file->data, map_size);
            
            uint32_t result = file_mmap_base;
            file_mmap_base += map_size;
            
            printf("[SYSCALL] mmap: file-backed mapping of %d bytes at 0x%08X\n", 
                   map_size, result);
            return result;
        }
    }
    
    printf("[SYSCALL] mmap: unsupported mapping type\n");
    return 0;
}

int sys_munmap(uint32_t addr, uint32_t length, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] munmap(addr=0x%08X, length=%d)\n", addr, length);
    
    // Validate address
    if (addr < 0x30000000 || addr >= 0x60000000) {
        printf("[SYSCALL] munmap: invalid address\n");
        return -1;
    }
    
    // Unmap the memory region
    void* ptr = (void*)((uintptr_t)addr);
    int result = memory_unmap(ptr, length);
    
    if (result == 0) {
        printf("[SYSCALL] munmap: unmapped %d bytes at 0x%08X\n", length, addr);
    } else {
        printf("[SYSCALL] munmap: failed to unmap\n");
    }
    
    return result;
}

int sys_socket(uint32_t domain, uint32_t type, uint32_t protocol, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] socket(domain=%d, type=%d, protocol=%d)\n", domain, type, protocol);
    return netstack_socket(domain, type, protocol);
}

int sys_bind(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] bind(sockfd=%d, addr=0x%08X, addrlen=%d)\n", sockfd, addr, addrlen);
    if (!addr) return -1;
    return netstack_bind(sockfd, (const SocketAddress*)addr);
}

int sys_connect(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] connect(sockfd=%d, addr=0x%08X, addrlen=%d)\n", sockfd, addr, addrlen);
    if (!addr) return -1;
    return netstack_connect(sockfd, (const SocketAddress*)addr);
}

int sys_listen(uint32_t sockfd, uint32_t backlog, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] listen(sockfd=%d, backlog=%d)\n", sockfd, backlog);
    return netstack_listen(sockfd, backlog);
}

int sys_accept(uint32_t sockfd, uint32_t addr, uint32_t addrlen, uint32_t arg4, uint32_t arg5) {
    printf("[SYSCALL] accept(sockfd=%d)\n", sockfd);
    return netstack_accept(sockfd, (SocketAddress*)addr);
}

int sys_send(uint32_t sockfd, uint32_t buf, uint32_t len, uint32_t flags, uint32_t arg5) {
    printf("[SYSCALL] send(sockfd=%d, len=%d, flags=%d)\n", sockfd, len, flags);
    if (!buf) return -1;
    return netstack_send(sockfd, (const void*)buf, len, flags);
}

int sys_recv(uint32_t sockfd, uint32_t buf, uint32_t len, uint32_t flags, uint32_t arg5) {
    printf("[SYSCALL] recv(sockfd=%d, len=%d, flags=%d)\n", sockfd, len, flags);
    if (!buf) return -1;
    return netstack_recv(sockfd, (void*)buf, len, flags);
}

int sys_sendto(uint32_t sockfd, uint32_t buf, uint32_t len, uint32_t flags, uint32_t dest_addr) {
    printf("[SYSCALL] sendto(sockfd=%d, len=%d, flags=%d)\n", sockfd, len, flags);
    if (!buf) return -1;
    return netstack_sendto(sockfd, (const void*)buf, len, (const SocketAddress*)dest_addr, flags);
}

int sys_recvfrom(uint32_t sockfd, uint32_t buf, uint32_t len, uint32_t flags, uint32_t src_addr) {
    printf("[SYSCALL] recvfrom(sockfd=%d, len=%d, flags=%d)\n", sockfd, len, flags);
    if (!buf) return -1;
    return netstack_recvfrom(sockfd, (void*)buf, len, (SocketAddress*)src_addr, flags);
}

// Initialize syscall table
void syscall_table_init(void) {
    memset(g_syscall_table, 0, sizeof(g_syscall_table));
    
    // Register syscall handlers
    g_syscall_table[SYS_exit].handler = sys_exit;
    g_syscall_table[SYS_exit].name = "exit";
    g_syscall_table[SYS_exit].arg_count = 1;
    
    g_syscall_table[SYS_fork].handler = sys_fork;
    g_syscall_table[SYS_fork].name = "fork";
    g_syscall_table[SYS_fork].arg_count = 0;
    
    g_syscall_table[SYS_read].handler = sys_read;
    g_syscall_table[SYS_read].name = "read";
    g_syscall_table[SYS_read].arg_count = 3;
    
    g_syscall_table[SYS_write].handler = sys_write;
    g_syscall_table[SYS_write].name = "write";
    g_syscall_table[SYS_write].arg_count = 3;
    
    g_syscall_table[SYS_open].handler = sys_open;
    g_syscall_table[SYS_open].name = "open";
    g_syscall_table[SYS_open].arg_count = 3;
    
    g_syscall_table[SYS_close].handler = sys_close;
    g_syscall_table[SYS_close].name = "close";
    g_syscall_table[SYS_close].arg_count = 1;
    
    g_syscall_table[SYS_getpid].handler = sys_getpid;
    g_syscall_table[SYS_getpid].name = "getpid";
    g_syscall_table[SYS_getpid].arg_count = 0;
    
    g_syscall_table[SYS_getuid].handler = sys_getuid;
    g_syscall_table[SYS_getuid].name = "getuid";
    g_syscall_table[SYS_getuid].arg_count = 0;
    
    g_syscall_table[SYS_brk].handler = sys_brk;
    g_syscall_table[SYS_brk].name = "brk";
    g_syscall_table[SYS_brk].arg_count = 1;
    
    g_syscall_table[SYS_mmap].handler = sys_mmap;
    g_syscall_table[SYS_mmap].name = "mmap";
    g_syscall_table[SYS_mmap].arg_count = 5;
    
    g_syscall_table[SYS_munmap].handler = sys_munmap;
    g_syscall_table[SYS_munmap].name = "munmap";
    g_syscall_table[SYS_munmap].arg_count = 2;
    
    g_syscall_table[SYS_socket].handler = sys_socket;
    g_syscall_table[SYS_socket].name = "socket";
    g_syscall_table[SYS_socket].arg_count = 3;
    
    g_syscall_table[SYS_bind].handler = sys_bind;
    g_syscall_table[SYS_bind].name = "bind";
    g_syscall_table[SYS_bind].arg_count = 3;
    
    g_syscall_table[SYS_connect].handler = sys_connect;
    g_syscall_table[SYS_connect].name = "connect";
    g_syscall_table[SYS_connect].arg_count = 3;
    
    g_syscall_table[SYS_listen].handler = sys_listen;
    g_syscall_table[SYS_listen].name = "listen";
    g_syscall_table[SYS_listen].arg_count = 2;
    
    g_syscall_table[SYS_accept].handler = sys_accept;
    g_syscall_table[SYS_accept].name = "accept";
    g_syscall_table[SYS_accept].arg_count = 3;
    
    g_syscall_table[SYS_send].handler = sys_send;
    g_syscall_table[SYS_send].name = "send";
    g_syscall_table[SYS_send].arg_count = 4;
    
    g_syscall_table[SYS_recv].handler = sys_recv;
    g_syscall_table[SYS_recv].name = "recv";
    g_syscall_table[SYS_recv].arg_count = 4;
    
    g_syscall_table[SYS_sendto].handler = sys_sendto;
    g_syscall_table[SYS_sendto].name = "sendto";
    g_syscall_table[SYS_sendto].arg_count = 5;
    
    g_syscall_table[SYS_recvfrom].handler = sys_recvfrom;
    g_syscall_table[SYS_recvfrom].name = "recvfrom";
    g_syscall_table[SYS_recvfrom].arg_count = 5;
    
    printf("[SYSCALL_TABLE] Initialized with %d syscalls\n", 21);
}

// Dispatch syscall
int syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, 
                     uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    // Check privilege
    if (g_privilege_context.current_level != PRIVILEGE_USER) {
        // Syscalls should be called from user mode
        // (Kernel mode can call functions directly)
    }
    
    // Validate syscall number
    if (syscall_num >= MAX_SYSCALLS) {
        printf("[SYSCALL] Invalid syscall number: %d\n", syscall_num);
        return -1;
    }
    
    // Check if handler exists
    if (!g_syscall_table[syscall_num].handler) {
        printf("[SYSCALL] Unimplemented syscall: %d\n", syscall_num);
        return -1;
    }
    
    // Log syscall (for debugging)
    if (g_syscall_table[syscall_num].name) {
        printf("[SYSCALL] Dispatching: %s\n", g_syscall_table[syscall_num].name);
    }
    
    // Switch to kernel mode
    privilege_enter_kernel_mode();
    
    // Call handler
    int result = g_syscall_table[syscall_num].handler(arg1, arg2, arg3, arg4, arg5);
    
    // Return to user mode
    privilege_enter_user_mode();
    
    return result;
}
