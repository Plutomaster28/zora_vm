#ifndef UNIX_IPC_H
#define UNIX_IPC_H

// Research UNIX IPC (Inter-Process Communication) system
// Message queues, semaphores, shared memory simulation

#include <stddef.h>
#include <time.h>

// IPC types
typedef enum {
    IPC_MESSAGE_QUEUE,
    IPC_SEMAPHORE,
    IPC_SHARED_MEMORY,
    IPC_PIPE,
    IPC_SOCKET
} IPCType;

// Message queue structure
typedef struct {
    int id;
    char name[64];
    int max_messages;
    int current_messages;
    size_t max_message_size;
    time_t created;
    int owner_uid;
    int permissions;
} MessageQueue;

// Semaphore structure
typedef struct {
    int id;
    char name[64];
    int value;
    int max_value;
    time_t created;
    int owner_uid;
    int permissions;
} Semaphore;

// Shared memory structure
typedef struct {
    int id;
    char name[64];
    size_t size;
    void* data;
    time_t created;
    int owner_uid;
    int permissions;
    int attached_processes;
} SharedMemory;

// Message structure
typedef struct {
    long type;
    char data[256];
    size_t size;
    time_t timestamp;
} IPCMessage;

// Function prototypes
int unix_ipc_init(void);
void unix_ipc_cleanup(void);

// Message queues
int unix_msgget(const char* name, int flags);
int unix_msgsnd(int id, IPCMessage* msg);
int unix_msgrcv(int id, IPCMessage* msg, long type);
int unix_msgctl(int id, int cmd);
void unix_list_message_queues(void);

// Semaphores
int unix_semget(const char* name, int value, int flags);
int unix_semop(int id, int operation);
int unix_semctl(int id, int cmd, int value);
void unix_list_semaphores(void);

// Shared memory
int unix_shmget(const char* name, size_t size, int flags);
void* unix_shmat(int id);
int unix_shmdt(void* addr);
int unix_shmctl(int id, int cmd);
void unix_list_shared_memory(void);

// Pipes and sockets
int unix_pipe(int pipefd[2]);
int unix_socket(int domain, int type, int protocol);
int unix_bind(int sockfd, const char* path);
int unix_listen(int sockfd, int backlog);
int unix_accept(int sockfd);
int unix_connect(int sockfd, const char* path);

// IPC utilities
void unix_ipcs(void);  // Show all IPC objects
void unix_ipcrm(IPCType type, int id);  // Remove IPC object

#endif // UNIX_IPC_H
