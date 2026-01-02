#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <stdint.h>
#include "system/process.h"

// Scheduling algorithms
typedef enum {
    SCHED_ROUND_ROBIN,      // Simple round-robin
    SCHED_PRIORITY,         // Priority-based
    SCHED_MULTILEVEL,       // Multi-level feedback queue
    SCHED_REAL_TIME         // Real-time scheduling
} SchedulerAlgorithm;

// Scheduler configuration
#define SCHEDULER_QUANTUM_MS        10      // Default time slice: 10ms
#define SCHEDULER_MAX_QUEUES        5       // For multi-level scheduling
#define SCHEDULER_BOOST_INTERVAL    100     // Priority boost every 100ms

// Process queue node
typedef struct ProcessQueueNode {
    Process* process;
    struct ProcessQueueNode* next;
    struct ProcessQueueNode* prev;
} ProcessQueueNode;

// Process queue
typedef struct {
    ProcessQueueNode* head;
    ProcessQueueNode* tail;
    int count;
} ProcessQueue;

// Scheduler state
typedef struct {
    SchedulerAlgorithm algorithm;
    Process* current_process;
    Process* idle_process;
    
    // Queues
    ProcessQueue ready_queue;
    ProcessQueue blocked_queue;
    ProcessQueue* priority_queues[SCHEDULER_MAX_QUEUES];
    
    // Statistics
    uint64_t total_context_switches;
    uint64_t total_preemptions;
    uint64_t total_yields;
    
    // Timing
    uint32_t quantum_ms;
    uint64_t last_schedule_time;
    uint64_t current_quantum_remaining;
    
    // Flags
    int preemption_enabled;
    int scheduling_enabled;
} Scheduler;

// Scheduler functions
int scheduler_init(void);
void scheduler_cleanup(void);
void scheduler_start(void);
void scheduler_stop(void);

// Process queue management
void scheduler_enqueue_ready(Process* process);
void scheduler_enqueue_blocked(Process* process);
void scheduler_remove_from_queue(Process* process);
Process* scheduler_dequeue_ready(void);

// Scheduling operations
void scheduler_schedule(void);          // Main scheduling function
void scheduler_yield(void);             // Voluntary yield
void scheduler_preempt(void);           // Forced preemption
void scheduler_tick(void);              // Called on timer interrupt

// Process state changes
void scheduler_block_process(Process* process);
void scheduler_unblock_process(Process* process);
void scheduler_sleep_process(Process* process, uint32_t ms);
void scheduler_wake_process(Process* process);

// Context switching
void scheduler_context_switch(Process* old_proc, Process* new_proc);
void scheduler_save_context(Process* process);
void scheduler_load_context(Process* process);

// Scheduling policy
void scheduler_set_algorithm(SchedulerAlgorithm algo);
void scheduler_set_quantum(uint32_t ms);
void scheduler_boost_priorities(void);

// Process selection
Process* scheduler_pick_next_process(void);
int scheduler_should_preempt(Process* current, Process* candidate);

// Utility functions
Scheduler* scheduler_get_state(void);
Process* scheduler_get_current_process(void);
uint64_t scheduler_get_context_switches(void);
void scheduler_dump_queues(void);

#endif // KERNEL_SCHEDULER_H
