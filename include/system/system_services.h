#ifndef SYSTEM_SERVICES_H
#define SYSTEM_SERVICES_H

#include <stdint.h>
#include <time.h>

#define MAX_SERVICES 64
#define MAX_CRON_JOBS 128
#define MAX_SERVICE_NAME 64
#define MAX_COMMAND_LENGTH 512
#define MAX_LOG_ENTRIES 1000

// Service states
typedef enum {
    SERVICE_STOPPED,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_STOPPING,
    SERVICE_FAILED,
    SERVICE_DISABLED
} ServiceState;

// Service types
typedef enum {
    SERVICE_ONESHOT,        // Run once and exit
    SERVICE_SIMPLE,         // Long-running process
    SERVICE_FORKING,        // Daemon that forks
    SERVICE_NOTIFY,         // Service that notifies when ready
    SERVICE_IDLE,           // Service that runs when system is idle
    SERVICE_TIMER           // Timer-activated service
} ServiceType;

// Cron job frequency
typedef enum {
    CRON_MINUTE,           // Every minute
    CRON_HOURLY,           // Every hour
    CRON_DAILY,            // Every day
    CRON_WEEKLY,           // Every week
    CRON_MONTHLY,          // Every month
    CRON_YEARLY,           // Every year
    CRON_CUSTOM            // Custom cron expression
} CronFrequency;

// Service dependency type
typedef enum {
    DEP_REQUIRES,          // Service must be running
    DEP_WANTS,             // Service should be running (not critical)
    DEP_CONFLICTS,         // Service must not be running
    DEP_AFTER,             // Start after this service
    DEP_BEFORE             // Start before this service
} DependencyType;

// Service dependency
typedef struct {
    char service_name[MAX_SERVICE_NAME];
    DependencyType type;
} ServiceDependency;

// Service configuration
typedef struct {
    char name[MAX_SERVICE_NAME];
    char description[256];
    char exec_start[MAX_COMMAND_LENGTH];
    char exec_stop[MAX_COMMAND_LENGTH];
    char exec_reload[MAX_COMMAND_LENGTH];
    char working_directory[256];
    char user[32];
    char group[32];
    
    ServiceType type;
    ServiceState state;
    
    // Restart behavior
    int restart_on_failure;
    int restart_on_success;
    int restart_delay_seconds;
    int max_restart_attempts;
    int current_restart_count;
    
    // Resource limits
    uint64_t memory_limit;          // Memory limit in bytes
    int cpu_limit_percent;          // CPU limit percentage
    int nice_level;                 // Process priority
    
    // Dependencies
    ServiceDependency dependencies[16];
    int dependency_count;
    
    // Runtime information
    int process_id;
    time_t start_time;
    time_t last_restart;
    uint64_t total_runtime;
    int exit_code;
    
    // Logging
    char log_file[256];
    int log_level;                  // 0=none, 1=error, 2=warn, 3=info, 4=debug
    
    // Status
    int enabled;                    // Service enabled for auto-start
    int auto_start;                 // Start at system boot
    int persistent;                 // Restart after crash
} ServiceConfig;

// Cron job configuration
typedef struct {
    char name[MAX_SERVICE_NAME];
    char description[256];
    char command[MAX_COMMAND_LENGTH];
    char user[32];
    char working_directory[256];
    
    // Schedule
    CronFrequency frequency;
    char cron_expression[64];       // Custom cron expression (minute hour day month weekday)
    
    // Timing
    time_t next_run;
    time_t last_run;
    int run_count;
    
    // Behavior
    int enabled;
    int catch_up;                   // Run missed jobs if system was down
    int exclusive;                  // Don't run if previous instance still running
    int timeout_seconds;            // Kill job if it runs too long
    
    // Logging
    char log_file[256];
    int log_stdout;
    int log_stderr;
    
    // Runtime
    int process_id;
    int exit_code;
    int is_running;
} CronJob;

// System log entry
typedef struct {
    time_t timestamp;
    char service_name[MAX_SERVICE_NAME];
    int level;                      // 0=error, 1=warn, 2=info, 3=debug
    char message[512];
} LogEntry;

// System services manager state
typedef struct {
    ServiceConfig services[MAX_SERVICES];
    int service_count;
    
    CronJob cron_jobs[MAX_CRON_JOBS];
    int cron_job_count;
    
    LogEntry logs[MAX_LOG_ENTRIES];
    int log_count;
    int log_index;                  // Circular buffer index
    
    // System state
    int services_enabled;
    int cron_enabled;
    time_t system_start_time;
    
    // Configuration
    char log_directory[256];
    int default_log_level;
    int max_log_size;
    int auto_cleanup_logs;
} SystemServicesManager;

// Core system services functions
int system_services_init(void);
void system_services_cleanup(void);
int system_services_start_all(void);
int system_services_stop_all(void);

// Service management
int service_create(const char* name, const char* description, const char* exec_start);
int service_delete(const char* name);
int service_start(const char* name);
int service_stop(const char* name);
int service_restart(const char* name);
int service_reload(const char* name);
int service_enable(const char* name);
int service_disable(const char* name);
ServiceState service_get_status(const char* name);
ServiceConfig* service_get_config(const char* name);

// Service configuration
int service_set_type(const char* name, ServiceType type);
int service_set_user(const char* name, const char* user);
int service_set_working_directory(const char* name, const char* directory);
int service_set_restart_policy(const char* name, int on_failure, int on_success, int delay);
int service_set_resource_limits(const char* name, uint64_t memory_limit, int cpu_percent);
int service_add_dependency(const char* name, const char* dependency, DependencyType type);
int service_remove_dependency(const char* name, const char* dependency);

// Service information
void service_list_all(void);
void service_show_status(const char* name);
void service_show_logs(const char* name, int tail_lines);
void service_show_dependencies(const char* name);

// Cron job management
int cron_create_job(const char* name, const char* command, CronFrequency frequency);
int cron_create_custom_job(const char* name, const char* command, const char* cron_expression);
int cron_delete_job(const char* name);
int cron_enable_job(const char* name);
int cron_disable_job(const char* name);
int cron_run_job_now(const char* name);
CronJob* cron_get_job(const char* name);

// Cron job configuration
int cron_set_user(const char* name, const char* user);
int cron_set_timeout(const char* name, int timeout_seconds);
int cron_set_logging(const char* name, const char* log_file, int log_stdout, int log_stderr);
int cron_set_behavior(const char* name, int catch_up, int exclusive);

// Cron job information
void cron_list_jobs(void);
void cron_show_job(const char* name);
void cron_show_schedule(void);
time_t cron_next_run_time(const char* name);

// Cron scheduler
int cron_scheduler_start(void);
int cron_scheduler_stop(void);
int cron_check_and_run_jobs(void);
time_t cron_parse_expression(const char* expression, time_t from_time);

// System logging
int log_message(const char* service_name, int level, const char* format, ...);
void log_show_recent(int count);
void log_show_service(const char* service_name);
void log_clear(void);
int log_save_to_file(const char* filename);

// System control commands
void systemctl_command(int argc, char** argv);
void service_command(int argc, char** argv);
void crontab_command(int argc, char** argv);
void journalctl_command(int argc, char** argv);

// Pre-defined system services
int create_default_services(void);
int create_network_service(void);
int create_log_rotation_service(void);
int create_system_monitor_service(void);
int create_package_update_service(void);

#endif // SYSTEM_SERVICES_H