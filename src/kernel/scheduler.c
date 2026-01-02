#include "kernel/scheduler.h"
#include "kernel/privilege.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global scheduler state
static Scheduler g_scheduler;
static int g_scheduler_initialized = 0;

// Helper: Allocate queue node
static ProcessQueueNode* alloc_queue_node(Process* process) {
    ProcessQueueNode* node = (ProcessQueueNode*)malloc(sizeof(ProcessQueueNode));
    if (!node) return NULL;
    
    node->process = process;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

// Helper: Free queue node
static void free_queue_node(ProcessQueueNode* node) {
    free(node);
}

// Initialize process queue
static void queue_init(ProcessQueue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
}

// Enqueue process at tail
static void queue_enqueue(ProcessQueue* queue, Process* process) {
    ProcessQueueNode* node = alloc_queue_node(process);
    if (!node) return;
    
    if (queue->tail) {
        queue->tail->next = node;
        node->prev = queue->tail;
        queue->tail = node;
    } else {
        queue->head = queue->tail = node;
    }
    
    queue->count++;
}

// Dequeue process from head
static Process* queue_dequeue(ProcessQueue* queue) {
    if (!queue->head) return NULL;
    
    ProcessQueueNode* node = queue->head;
    Process* process = node->process;
    
    queue->head = node->next;
    if (queue->head) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }
    
    queue->count--;
    free_queue_node(node);
    
    return process;
}

// Remove specific process from queue
static void queue_remove(ProcessQueue* queue, Process* process) {
    ProcessQueueNode* node = queue->head;
    
    while (node) {
        if (node->process == process) {
            // Unlink node
            if (node->prev) {
                node->prev->next = node->next;
            } else {
                queue->head = node->next;
            }
            
            if (node->next) {
                node->next->prev = node->prev;
            } else {
                queue->tail = node->prev;
            }
            
            queue->count--;
            free_queue_node(node);
            return;
        }
        node = node->next;
    }
}

// Initialize scheduler
int scheduler_init(void) {
    if (g_scheduler_initialized) {
        return 0;  // Already initialized
    }
    
    memset(&g_scheduler, 0, sizeof(Scheduler));
    
    // Set default configuration
    g_scheduler.algorithm = SCHED_ROUND_ROBIN;
    g_scheduler.quantum_ms = SCHEDULER_QUANTUM_MS;
    g_scheduler.current_process = NULL;
    g_scheduler.idle_process = NULL;
    g_scheduler.preemption_enabled = 1;
    g_scheduler.scheduling_enabled = 0;  // Not started yet
    
    // Initialize queues
    queue_init(&g_scheduler.ready_queue);
    queue_init(&g_scheduler.blocked_queue);
    
    // Initialize priority queues for multi-level scheduling
    for (int i = 0; i < SCHEDULER_MAX_QUEUES; i++) {
        g_scheduler.priority_queues[i] = (ProcessQueue*)malloc(sizeof(ProcessQueue));
        if (g_scheduler.priority_queues[i]) {
            queue_init(g_scheduler.priority_queues[i]);
        }
    }
    
    g_scheduler_initialized = 1;
    
    printf("[SCHEDULER] Initialized (%s, quantum=%dms)\n",
           g_scheduler.algorithm == SCHED_ROUND_ROBIN ? "Round-Robin" : "Priority",
           g_scheduler.quantum_ms);
    
    return 0;
}

// Cleanup scheduler
void scheduler_cleanup(void) {
    if (!g_scheduler_initialized) return;
    
    // Free priority queues
    for (int i = 0; i < SCHEDULER_MAX_QUEUES; i++) {
        if (g_scheduler.priority_queues[i]) {
            free(g_scheduler.priority_queues[i]);
            g_scheduler.priority_queues[i] = NULL;
        }
    }
    
    g_scheduler_initialized = 0;
    printf("[SCHEDULER] Cleaned up\n");
}

// Start scheduler
void scheduler_start(void) {
    g_scheduler.scheduling_enabled = 1;
    g_scheduler.last_schedule_time = 0; // Would get real time here
    printf("[SCHEDULER] Started\n");
}

// Stop scheduler
void scheduler_stop(void) {
    g_scheduler.scheduling_enabled = 0;
    printf("[SCHEDULER] Stopped\n");
}

// Enqueue process to ready queue
void scheduler_enqueue_ready(Process* process) {
    if (!process) return;
    
    process->state = PROC_STATE_RUNNING;  // Use correct enum name
    queue_enqueue(&g_scheduler.ready_queue, process);
}

// Enqueue process to blocked queue
void scheduler_enqueue_blocked(Process* process) {
    if (!process) return;
    
    process->state = PROC_STATE_SLEEPING;  // Use correct enum name
    queue_enqueue(&g_scheduler.blocked_queue, process);
}

// Remove process from any queue
void scheduler_remove_from_queue(Process* process) {
    if (!process) return;
    
    queue_remove(&g_scheduler.ready_queue, process);
    queue_remove(&g_scheduler.blocked_queue, process);
}

// Dequeue next ready process
Process* scheduler_dequeue_ready(void) {
    return queue_dequeue(&g_scheduler.ready_queue);
}

// Pick next process to run
Process* scheduler_pick_next_process(void) {
    switch (g_scheduler.algorithm) {
        case SCHED_ROUND_ROBIN:
            // Simple round-robin: pick first from ready queue
            return scheduler_dequeue_ready();
            
        case SCHED_PRIORITY:
            // Priority-based: search priority queues from highest to lowest
            for (int i = SCHEDULER_MAX_QUEUES - 1; i >= 0; i--) {
                if (g_scheduler.priority_queues[i]->count > 0) {
                    return queue_dequeue(g_scheduler.priority_queues[i]);
                }
            }
            return NULL;
            
        default:
            return scheduler_dequeue_ready();
    }
}

// Main scheduling function
void scheduler_schedule(void) {
    if (!g_scheduler_initialized || !g_scheduler.scheduling_enabled) {
        return;
    }
    
    // Switch to kernel mode for scheduling
    privilege_enter_kernel_mode();
    
    Process* old_process = g_scheduler.current_process;
    Process* new_process = scheduler_pick_next_process();
    
    // If no process to run, use idle process
    if (!new_process) {
        new_process = g_scheduler.idle_process;
    }
    
    // If same process, nothing to do
    if (new_process == old_process) {
        return;
    }
    
    // Perform context switch
    if (old_process && old_process->state == PROC_STATE_RUNNING) {
        old_process->state = PROC_STATE_RUNNING;  // Keep as running, will be enqueued
        scheduler_enqueue_ready(old_process);
    }
    
    if (new_process) {
        new_process->state = PROC_STATE_RUNNING;
        g_scheduler.current_process = new_process;
        
        // Perform actual context switch
        scheduler_context_switch(old_process, new_process);
        
        // Update statistics
        g_scheduler.total_context_switches++;
        g_scheduler.current_quantum_remaining = g_scheduler.quantum_ms;
    }
}

// Voluntary yield
void scheduler_yield(void) {
    if (!g_scheduler_initialized) return;
    
    g_scheduler.total_yields++;
    scheduler_schedule();
}

// Forced preemption
void scheduler_preempt(void) {
    if (!g_scheduler_initialized || !g_scheduler.preemption_enabled) {
        return;
    }
    
    g_scheduler.total_preemptions++;
    scheduler_schedule();
}

// Called on timer interrupt
void scheduler_tick(void) {
    if (!g_scheduler_initialized || !g_scheduler.scheduling_enabled) {
        return;
    }
    
    // Decrement quantum
    if (g_scheduler.current_quantum_remaining > 0) {
        g_scheduler.current_quantum_remaining--;
    }
    
    // If quantum expired, preempt
    if (g_scheduler.current_quantum_remaining == 0 && g_scheduler.preemption_enabled) {
        scheduler_preempt();
    }
}

// Block current process
void scheduler_block_process(Process* process) {
    if (!process) return;
    
    scheduler_remove_from_queue(process);
    scheduler_enqueue_blocked(process);
    
    if (process == g_scheduler.current_process) {
        scheduler_schedule();  // Switch to another process
    }
}

// Unblock process
void scheduler_unblock_process(Process* process) {
    if (!process) return;
    
    queue_remove(&g_scheduler.blocked_queue, process);
    scheduler_enqueue_ready(process);
}

// Sleep process for milliseconds
void scheduler_sleep_process(Process* process, uint32_t ms) {
    if (!process) return;
    
    // Would set wakeup time here
    scheduler_block_process(process);
}

// Wake process
void scheduler_wake_process(Process* process) {
    scheduler_unblock_process(process);
}

// Save process context
void scheduler_save_context(Process* process) {
    if (!process) return;
    
    // Save CPU registers to process structure
    // In real kernel, would save all general-purpose registers, stack pointer, etc.
    // For now, just mark that context was saved
    printf("[SCHEDULER] Saved context for PID %d\n", process->pid);
}

// Load process context
void scheduler_load_context(Process* process) {
    if (!process) return;
    
    // Restore CPU registers from process structure
    // In real kernel, would restore all general-purpose registers, stack pointer, etc.
    // For now, just mark that context was loaded
    printf("[SCHEDULER] Loaded context for PID %d\n", process->pid);
}

// Perform context switch
void scheduler_context_switch(Process* old_proc, Process* new_proc) {
    if (!new_proc) return;
    
    // Save old process context
    if (old_proc) {
        scheduler_save_context(old_proc);
    }
    
    // Load new process context
    scheduler_load_context(new_proc);
    
    // Switch page directory if MMU is enabled
    // Would call mmu_switch_page_directory(new_proc->page_directory) here
    
    // Switch to user mode if new process is user process
    if (new_proc->pid > 0) {  // PID 0 is kernel
        privilege_enter_user_mode();
    }
    
    printf("[SCHEDULER] Context switch: %s -> %s\n",
           old_proc ? old_proc->name : "none",
           new_proc->name);
}

// Set scheduling algorithm
void scheduler_set_algorithm(SchedulerAlgorithm algo) {
    g_scheduler.algorithm = algo;
    printf("[SCHEDULER] Algorithm changed to %d\n", algo);
}

// Set time quantum
void scheduler_set_quantum(uint32_t ms) {
    g_scheduler.quantum_ms = ms;
    printf("[SCHEDULER] Quantum set to %dms\n", ms);
}

// Boost priorities (for aging)
void scheduler_boost_priorities(void) {
    // Move processes from lower priority queues to higher ones
    // This prevents starvation
    printf("[SCHEDULER] Priority boost\n");
}

// Check if should preempt
int scheduler_should_preempt(Process* current, Process* candidate) {
    if (!current || !candidate) return 0;
    
    // Simple policy: always preempt if candidate has higher priority
    return (candidate->priority > current->priority);
}

// Get scheduler state
Scheduler* scheduler_get_state(void) {
    return &g_scheduler;
}

// Get current process
Process* scheduler_get_current_process(void) {
    return g_scheduler.current_process;
}

// Get context switch count
uint64_t scheduler_get_context_switches(void) {
    return g_scheduler.total_context_switches;
}

// Dump scheduler queues
void scheduler_dump_queues(void) {
    printf("[SCHEDULER] Ready queue: %d processes\n", g_scheduler.ready_queue.count);
    printf("[SCHEDULER] Blocked queue: %d processes\n", g_scheduler.blocked_queue.count);
    printf("[SCHEDULER] Context switches: %llu\n", g_scheduler.total_context_switches);
    printf("[SCHEDULER] Preemptions: %llu\n", g_scheduler.total_preemptions);
    printf("[SCHEDULER] Yields: %llu\n", g_scheduler.total_yields);
}
