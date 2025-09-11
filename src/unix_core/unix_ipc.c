#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "unix_ipc.h"
#include "terminal/terminal_detector.h"

// Static arrays to simulate IPC objects
static MessageQueue message_queues[32];
static Semaphore semaphores[32];
static SharedMemory shared_memory[32];

static int msg_count = 0;
static int sem_count = 0;
static int shm_count = 0;

static int ipc_initialized = 0;

int unix_ipc_init(void) {
    if (ipc_initialized) return 0;
    
    printf("[IPC] Initializing Research UNIX IPC system...\n");
    
    // Initialize arrays
    memset(message_queues, 0, sizeof(message_queues));
    memset(semaphores, 0, sizeof(semaphores));
    memset(shared_memory, 0, sizeof(shared_memory));
    
    msg_count = sem_count = shm_count = 0;
    
    // Create some default system IPC objects
    unix_msgget("system.log", 0666);
    unix_semget("console.lock", 1, 0666);
    unix_shmget("system.stats", 4096, 0666);
    
    ipc_initialized = 1;
    printf("[IPC] IPC system initialized with system objects\n");
    return 0;
}

int unix_msgget(const char* name, int flags) {
    if (msg_count >= 32) {
        printf("msgget: too many message queues\n");
        return -1;
    }
    
    // Check if already exists
    for (int i = 0; i < msg_count; i++) {
        if (strcmp(message_queues[i].name, name) == 0) {
            return message_queues[i].id;
        }
    }
    
    // Create new message queue
    MessageQueue* mq = &message_queues[msg_count];
    mq->id = msg_count + 1;
    strncpy(mq->name, name, sizeof(mq->name) - 1);
    mq->max_messages = 10;
    mq->current_messages = 0;
    mq->max_message_size = 256;
    mq->created = time(NULL);
    mq->owner_uid = 0;  // root for now
    mq->permissions = flags;
    
    msg_count++;
    printf("Message queue created: %s (id=%d)\n", name, mq->id);
    return mq->id;
}

int unix_msgsnd(int id, IPCMessage* msg) {
    for (int i = 0; i < msg_count; i++) {
        if (message_queues[i].id == id) {
            if (message_queues[i].current_messages >= message_queues[i].max_messages) {
                printf("msgsnd: queue full\n");
                return -1;
            }
            message_queues[i].current_messages++;
            msg->timestamp = time(NULL);
            printf("Message sent to queue %d: %.50s\n", id, msg->data);
            return 0;
        }
    }
    printf("msgsnd: invalid queue id %d\n", id);
    return -1;
}

int unix_msgrcv(int id, IPCMessage* msg, long type) {
    for (int i = 0; i < msg_count; i++) {
        if (message_queues[i].id == id) {
            if (message_queues[i].current_messages <= 0) {
                printf("msgrcv: queue empty\n");
                return -1;
            }
            message_queues[i].current_messages--;
            msg->type = type;
            strcpy(msg->data, "Sample message from queue");
            msg->size = strlen(msg->data);
            msg->timestamp = time(NULL);
            printf("Message received from queue %d\n", id);
            return 0;
        }
    }
    printf("msgrcv: invalid queue id %d\n", id);
    return -1;
}

int unix_semget(const char* name, int value, int flags) {
    if (sem_count >= 32) {
        printf("semget: too many semaphores\n");
        return -1;
    }
    
    // Check if already exists
    for (int i = 0; i < sem_count; i++) {
        if (strcmp(semaphores[i].name, name) == 0) {
            return semaphores[i].id;
        }
    }
    
    // Create new semaphore
    Semaphore* sem = &semaphores[sem_count];
    sem->id = sem_count + 1;
    strncpy(sem->name, name, sizeof(sem->name) - 1);
    sem->value = value;
    sem->max_value = value;
    sem->created = time(NULL);
    sem->owner_uid = 0;
    sem->permissions = flags;
    
    sem_count++;
    printf("Semaphore created: %s (id=%d, value=%d)\n", name, sem->id, value);
    return sem->id;
}

int unix_semop(int id, int operation) {
    for (int i = 0; i < sem_count; i++) {
        if (semaphores[i].id == id) {
            semaphores[i].value += operation;
            if (semaphores[i].value < 0) semaphores[i].value = 0;
            if (semaphores[i].value > semaphores[i].max_value) {
                semaphores[i].value = semaphores[i].max_value;
            }
            printf("Semaphore %d: operation %d, new value=%d\n", 
                   id, operation, semaphores[i].value);
            return 0;
        }
    }
    printf("semop: invalid semaphore id %d\n", id);
    return -1;
}

int unix_shmget(const char* name, size_t size, int flags) {
    if (shm_count >= 32) {
        printf("shmget: too many shared memory segments\n");
        return -1;
    }
    
    // Check if already exists
    for (int i = 0; i < shm_count; i++) {
        if (strcmp(shared_memory[i].name, name) == 0) {
            return shared_memory[i].id;
        }
    }
    
    // Create new shared memory segment
    SharedMemory* shm = &shared_memory[shm_count];
    shm->id = shm_count + 1;
    strncpy(shm->name, name, sizeof(shm->name) - 1);
    shm->size = size;
    shm->data = malloc(size);
    if (!shm->data) {
        printf("shmget: failed to allocate memory\n");
        return -1;
    }
    memset(shm->data, 0, size);
    shm->created = time(NULL);
    shm->owner_uid = 0;
    shm->permissions = flags;
    shm->attached_processes = 0;
    
    shm_count++;
    printf("Shared memory created: %s (id=%d, size=%zu)\n", name, shm->id, size);
    return shm->id;
}

void* unix_shmat(int id) {
    for (int i = 0; i < shm_count; i++) {
        if (shared_memory[i].id == id) {
            shared_memory[i].attached_processes++;
            printf("Shared memory attached: id=%d, processes=%d\n", 
                   id, shared_memory[i].attached_processes);
            return shared_memory[i].data;
        }
    }
    printf("shmat: invalid shared memory id %d\n", id);
    return NULL;
}

void unix_list_message_queues(void) {
    const char* tl = get_box_char(BOX_TOP_LEFT);
    const char* tr = get_box_char(BOX_TOP_RIGHT);
    const char* bl = get_box_char(BOX_BOTTOM_LEFT);
    const char* br = get_box_char(BOX_BOTTOM_RIGHT);
    const char* h = get_box_char(BOX_HORIZONTAL);
    const char* v = get_box_char(BOX_VERTICAL);
    const char* cross = get_box_char(BOX_CROSS);
    
    printf("\n%s", tl);
    for (int i = 0; i < 70; i++) printf("%s", h);
    printf("%s\n", tr);
    
    printf("%s                          Message Queues                           %s\n", v, v);
    
    printf("%s", tl);
    for (int i = 0; i < 70; i++) printf("%s", h);
    printf("%s\n", tr);
    
    printf("%s ID  %s Name             %s Messages %s Max %s Owner %s Perms    %s\n", 
           v, v, v, v, v, v, v);
    
    for (int i = 0; i < msg_count; i++) {
        MessageQueue* mq = &message_queues[i];
        printf("%s %-3d %s %-15s %s %-8d %s %-3d %s %-5d %s %04o     %s\n",
               v, mq->id, v, mq->name, v, mq->current_messages, 
               v, mq->max_messages, v, mq->owner_uid, v, mq->permissions, v);
    }
    
    printf("%s", bl);
    for (int i = 0; i < 70; i++) printf("%s", h);
    printf("%s\n", br);
}

void unix_list_semaphores(void) {
    const char* tl = get_box_char(BOX_TOP_LEFT);
    const char* tr = get_box_char(BOX_TOP_RIGHT);
    const char* bl = get_box_char(BOX_BOTTOM_LEFT);
    const char* br = get_box_char(BOX_BOTTOM_RIGHT);
    const char* h = get_box_char(BOX_HORIZONTAL);
    const char* v = get_box_char(BOX_VERTICAL);
    
    printf("\n%s", tl);
    for (int i = 0; i < 60; i++) printf("%s", h);
    printf("%s\n", tr);
    
    printf("%s                       Semaphores                        %s\n", v, v);
    
    printf("%s ID %s Name           %s Value %s Max %s Owner %s Perms  %s\n", 
           v, v, v, v, v, v, v);
    
    for (int i = 0; i < sem_count; i++) {
        Semaphore* sem = &semaphores[i];
        printf("%s %-2d %s %-13s %s %-5d %s %-3d %s %-5d %s %04o  %s\n",
               v, sem->id, v, sem->name, v, sem->value, 
               v, sem->max_value, v, sem->owner_uid, v, sem->permissions, v);
    }
    
    printf("%s", bl);
    for (int i = 0; i < 60; i++) printf("%s", h);
    printf("%s\n", br);
}

void unix_list_shared_memory(void) {
    const char* tl = get_box_char(BOX_TOP_LEFT);
    const char* tr = get_box_char(BOX_TOP_RIGHT);
    const char* bl = get_box_char(BOX_BOTTOM_LEFT);
    const char* br = get_box_char(BOX_BOTTOM_RIGHT);
    const char* h = get_box_char(BOX_HORIZONTAL);
    const char* v = get_box_char(BOX_VERTICAL);
    
    printf("\n%s", tl);
    for (int i = 0; i < 70; i++) printf("%s", h);
    printf("%s\n", tr);
    
    printf("%s                         Shared Memory                          %s\n", v, v);
    
    printf("%s ID %s Name           %s Size     %s Attached %s Owner %s Perms  %s\n", 
           v, v, v, v, v, v, v);
    
    for (int i = 0; i < shm_count; i++) {
        SharedMemory* shm = &shared_memory[i];
        printf("%s %-2d %s %-13s %s %-8zu %s %-8d %s %-5d %s %04o  %s\n",
               v, shm->id, v, shm->name, v, shm->size, 
               v, shm->attached_processes, v, shm->owner_uid, v, shm->permissions, v);
    }
    
    printf("%s", bl);
    for (int i = 0; i < 70; i++) printf("%s", h);
    printf("%s\n", br);
}

void unix_ipcs(void) {
    printf("ZoraVM Research UNIX IPC Status\n");
    printf("===============================\n");
    
    unix_list_message_queues();
    unix_list_semaphores();
    unix_list_shared_memory();
    
    printf("\nIPC Summary:\n");
    printf("Message Queues: %d/32\n", msg_count);
    printf("Semaphores: %d/32\n", sem_count);
    printf("Shared Memory: %d/32\n", shm_count);
}

void unix_ipcrm(IPCType type, int id) {
    switch (type) {
        case IPC_MESSAGE_QUEUE:
            for (int i = 0; i < msg_count; i++) {
                if (message_queues[i].id == id) {
                    printf("Removing message queue %d (%s)\n", 
                           id, message_queues[i].name);
                    // Shift array
                    for (int j = i; j < msg_count - 1; j++) {
                        message_queues[j] = message_queues[j + 1];
                    }
                    msg_count--;
                    return;
                }
            }
            break;
        case IPC_SEMAPHORE:
            for (int i = 0; i < sem_count; i++) {
                if (semaphores[i].id == id) {
                    printf("Removing semaphore %d (%s)\n", 
                           id, semaphores[i].name);
                    for (int j = i; j < sem_count - 1; j++) {
                        semaphores[j] = semaphores[j + 1];
                    }
                    sem_count--;
                    return;
                }
            }
            break;
        case IPC_SHARED_MEMORY:
            for (int i = 0; i < shm_count; i++) {
                if (shared_memory[i].id == id) {
                    printf("Removing shared memory %d (%s)\n", 
                           id, shared_memory[i].name);
                    free(shared_memory[i].data);
                    for (int j = i; j < shm_count - 1; j++) {
                        shared_memory[j] = shared_memory[j + 1];
                    }
                    shm_count--;
                    return;
                }
            }
            break;
        default:
            printf("ipcrm: unknown IPC type\n");
            break;
    }
    printf("ipcrm: IPC object %d not found\n", id);
}

void unix_ipc_cleanup(void) {
    // Free shared memory
    for (int i = 0; i < shm_count; i++) {
        if (shared_memory[i].data) {
            free(shared_memory[i].data);
        }
    }
    
    msg_count = sem_count = shm_count = 0;
    ipc_initialized = 0;
}
