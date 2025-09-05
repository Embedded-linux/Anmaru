/* ============================================================================
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Dynamic Scheduler Switching Header
 * 
 * Implements runtime scheduler switching with safe task migration,
 * state preservation, and rollback capabilities.
 * ============================================================================ */

#ifndef DSRTOS_SWITCH_H
#define DSRTOS_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_scheduler.h"

/* ============================================================================
 * CONFIGURATION CONSTANTS
 * ============================================================================ */

/* Switch timing constraints */
#define DSRTOS_MAX_SWITCH_TIME_US          500U    /* Maximum switch duration */
#define DSRTOS_SWITCH_CRITICAL_TIME_US     100U    /* Max critical section time */
#define DSRTOS_MIN_SWITCH_INTERVAL_MS      100U    /* Minimum between switches */

/* Migration limits */
#define DSRTOS_MAX_MIGRATION_BATCH         32U     /* Tasks per migration batch */
#define DSRTOS_MAX_MIGRATION_RETRIES       3U      /* Migration retry attempts */
#define DSRTOS_MIGRATION_TIMEOUT_US        50U     /* Per-task migration timeout */

/* State preservation */
#define DSRTOS_STATE_BUFFER_SIZE           4096U   /* State preservation buffer */
#define DSRTOS_STATE_CHECKSUM_SEED         0x5749U /* 'SW' in hex */

/* History tracking */
#define DSRTOS_SWITCH_HISTORY_SIZE         16U     /* Switch history records */
#define DSRTOS_MAX_ROLLBACK_DEPTH          2U      /* Rollback stack depth */

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Forward declarations */
typedef struct dsrtos_switch_context dsrtos_switch_context_t;
typedef struct dsrtos_switch_controller dsrtos_switch_controller_t;

/* Migration strategies */
typedef enum {
    MIGRATE_PRESERVE_ORDER = 0,    /* Keep relative task ordering */
    MIGRATE_PRIORITY_BASED,        /* Re-sort by priority */
    MIGRATE_DEADLINE_BASED,        /* Re-sort by deadline */
    MIGRATE_CUSTOM,                /* Custom migration function */
    MIGRATE_STRATEGY_COUNT
} dsrtos_migration_strategy_t;

/* Switch phases */
typedef enum {
    SWITCH_IDLE = 0,
    SWITCH_PREPARING,
    SWITCH_VALIDATING,
    SWITCH_ENTERING_CRITICAL,
    SWITCH_SAVING_STATE,
    SWITCH_MIGRATING_TASKS,
    SWITCH_ACTIVATING_NEW,
    SWITCH_EXITING_CRITICAL,
    SWITCH_VERIFYING,
    SWITCH_COMPLETE,
    SWITCH_FAILED,
    SWITCH_ROLLING_BACK
} dsrtos_switch_phase_t;

/* Switch reasons */
typedef enum {
    SWITCH_REASON_MANUAL = 0,      /* Manual switch request */
    SWITCH_REASON_PERFORMANCE,     /* Performance optimization */
    SWITCH_REASON_POWER,          /* Power management */
    SWITCH_REASON_DEADLINE,       /* Deadline requirements */
    SWITCH_REASON_OVERLOAD,       /* System overload */
    SWITCH_REASON_EMERGENCY,      /* Emergency condition */
    SWITCH_REASON_TEST            /* Testing purposes */
} dsrtos_switch_reason_t;

/* Task migration record */
typedef struct dsrtos_migration_record {
    dsrtos_tcb_t* task;                   /* Task being migrated */
    uint32_t original_state;               /* Original task state */
    uint32_t original_priority;            /* Original priority */
    void* scheduler_data;                  /* Scheduler-specific data */
    uint32_t migration_time_us;           /* Migration duration */
    bool success;                          /* Migration success flag */
} dsrtos_migration_record_t;

/* Queue snapshot for rollback */
typedef struct dsrtos_queue_snapshot {
    dsrtos_tcb_t** tasks;                 /* Array of task pointers */
    uint32_t count;                       /* Number of tasks */
    uint32_t* priorities;                 /* Task priorities */
    uint32_t* states;                     /* Task states */
    uint32_t timestamp;                   /* Snapshot timestamp */
    uint32_t checksum;                    /* Data integrity checksum */
} dsrtos_queue_snapshot_t;

/* Switch request structure */
typedef struct dsrtos_switch_request {
    uint16_t from_scheduler_id;           /* Source scheduler ID */
    uint16_t to_scheduler_id;             /* Target scheduler ID */
    dsrtos_switch_reason_t reason;        /* Switch reason */
    dsrtos_migration_strategy_t strategy; /* Migration strategy */
    uint32_t requested_time;              /* Request timestamp */
    uint32_t deadline_us;                 /* Switch deadline */
    bool forced;                          /* Force switch flag */
    bool atomic;                          /* Atomic switch required */
    void* custom_data;                    /* Custom migration data */
} dsrtos_switch_request_t;

/* Switch statistics */
typedef struct dsrtos_switch_stats {
    uint64_t total_switches;              /* Total switches performed */
    uint64_t successful_switches;         /* Successful switches */
    uint64_t failed_switches;             /* Failed switches */
    uint64_t rollback_count;              /* Number of rollbacks */
    uint32_t min_switch_time_us;          /* Minimum switch time */
    uint32_t max_switch_time_us;          /* Maximum switch time */
    uint32_t avg_switch_time_us;          /* Average switch time */
    uint32_t last_switch_time_us;         /* Last switch duration */
    uint32_t max_critical_time_us;        /* Max critical section time */
    uint32_t total_tasks_migrated;        /* Total tasks migrated */
    uint32_t migration_failures;          /* Task migration failures */
} dsrtos_switch_stats_t;

/* Switch history record */
typedef struct dsrtos_switch_history {
    uint32_t timestamp;                   /* Switch timestamp */
    uint16_t from_id;                     /* From scheduler ID */
    uint16_t to_id;                       /* To scheduler ID */
    dsrtos_switch_reason_t reason;        /* Switch reason */
    uint32_t duration_us;                 /* Switch duration */
    uint32_t tasks_migrated;              /* Number of tasks migrated */
    bool success;                          /* Switch success */
    uint32_t error_code;                  /* Error code if failed */
} dsrtos_switch_history_t;

/* Rollback state */
typedef struct dsrtos_rollback_state {
    bool rollback_possible;               /* Rollback available */
    void* saved_state;                    /* Saved scheduler state */
    uint32_t state_size;                  /* State buffer size */
    dsrtos_queue_snapshot_t ready_snapshot;  /* Ready queue snapshot */
    dsrtos_queue_snapshot_t blocked_snapshot; /* Blocked queue snapshot */
    uint32_t rollback_point;              /* Rollback decision point */
    uint32_t checksum;                    /* State checksum */
} dsrtos_rollback_state_t;

/* Switch context - complete switch state */
struct dsrtos_switch_context {
    /* Request information */
    dsrtos_switch_request_t request;
    
    /* Current phase */
    dsrtos_switch_phase_t phase;
    
    /* Task migration */
    struct {
        dsrtos_tcb_t** task_list;        /* Tasks to migrate */
        uint32_t task_count;              /* Total tasks */
        uint32_t migrated_count;          /* Successfully migrated */
        uint32_t failed_count;            /* Failed migrations */
        uint32_t batch_size;              /* Migration batch size */
        dsrtos_migration_record_t* records; /* Migration records */
    } migration;
    
    /* State preservation */
    struct {
        uint8_t* state_buffer;            /* State preservation buffer */
        uint32_t buffer_size;             /* Buffer size */
        uint32_t used_size;               /* Used buffer space */
        uint32_t checksum;                /* State checksum */
        bool state_saved;                 /* State saved flag */
    } preservation;
    
    /* Timing information */
    struct {
        uint32_t start_time;              /* Switch start time */
        uint32_t critical_entry;          /* Critical section entry */
        uint32_t critical_exit;           /* Critical section exit */
        uint32_t completion_time;         /* Switch completion */
        uint32_t phase_times[12];         /* Time per phase */
    } timing;
    
    /* Rollback support */
    dsrtos_rollback_state_t rollback;
    
    /* Error handling */
    struct {
        uint32_t error_code;              /* Error code */
        uint32_t error_phase;             /* Phase where error occurred */
        const char* error_msg;            /* Error message */
        uint32_t retry_count;             /* Retry attempts */
    } error;
    
    /* Performance metrics */
    struct {
        uint32_t interrupt_latency;       /* Max IRQ latency */
        uint32_t task_migration_time;     /* Migration time */
        uint32_t state_save_time;         /* State save time */
        uint32_t state_restore_time;      /* State restore time */
        uint32_t critical_time;           /* Critical section time */
    } metrics;
};

/* Switch controller - manages all switches */
struct dsrtos_switch_controller {
    /* Controller state */
    bool initialized;                     /* Controller initialized */
    bool switch_in_progress;              /* Switch active flag */
    uint32_t switch_count;                /* Total switches */
    
    /* Active switch */
    dsrtos_switch_context_t* current_switch;
    
    /* Switch history */
    dsrtos_switch_history_t history[DSRTOS_SWITCH_HISTORY_SIZE];
    uint32_t history_index;
    
    /* Switch policies */
    struct {
        uint32_t min_interval_ms;         /* Minimum between switches */
        uint32_t max_duration_us;         /* Maximum switch duration */
        uint32_t max_critical_us;         /* Max critical section */
        bool allow_forced;                /* Allow forced switches */
        bool allow_runtime;               /* Allow runtime switches */
        bool require_idle;                /* Require idle for switch */
    } policies;
    
    /* Validation callbacks */
    struct {
        bool (*pre_switch_check)(const dsrtos_switch_request_t* req);
        bool (*validate_migration)(dsrtos_tcb_t* task, uint16_t to_id);
        bool (*post_switch_verify)(void);
    } validation;
    
    /* Custom migration handlers */
    dsrtos_status_t (*custom_migrate)(dsrtos_tcb_t* task, 
                                      void* from_sched, 
                                      void* to_sched);
    
    /* Statistics */
    dsrtos_switch_stats_t stats;
    
    /* Synchronization */
    uint32_t lock;                        /* Controller lock */
};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/* Controller initialization and management */
dsrtos_status_t dsrtos_switch_controller_init(
    dsrtos_switch_controller_t* controller);

dsrtos_status_t dsrtos_switch_controller_deinit(
    dsrtos_switch_controller_t* controller);

dsrtos_status_t dsrtos_switch_controller_reset(
    dsrtos_switch_controller_t* controller);

/* Switch execution */
dsrtos_status_t dsrtos_switch_scheduler(
    uint16_t from_id, 
    uint16_t to_id, 
    bool forced);

dsrtos_status_t dsrtos_switch_scheduler_ex(
    const dsrtos_switch_request_t* request);

dsrtos_status_t dsrtos_switch_abort(void);

/* Switch phases */
dsrtos_status_t dsrtos_switch_prepare(
    dsrtos_switch_context_t* context);

dsrtos_status_t dsrtos_switch_validate(
    dsrtos_switch_context_t* context);

dsrtos_status_t dsrtos_switch_execute(
    dsrtos_switch_context_t* context);

dsrtos_status_t dsrtos_switch_complete(
    dsrtos_switch_context_t* context);

dsrtos_status_t dsrtos_switch_rollback(
    dsrtos_switch_context_t* context);

/* Task migration */
dsrtos_status_t dsrtos_migrate_task(
    dsrtos_tcb_t* task,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy);

dsrtos_status_t dsrtos_migrate_task_batch(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy);

/* State management */
dsrtos_status_t dsrtos_switch_save_state(
    dsrtos_switch_context_t* context,
    void* scheduler,
    uint16_t scheduler_id);

dsrtos_status_t dsrtos_switch_restore_state(
    dsrtos_switch_context_t* context,
    void* scheduler,
    uint16_t scheduler_id);

dsrtos_status_t dsrtos_switch_snapshot_queues(
    dsrtos_queue_snapshot_t* snapshot,
    void* scheduler);

/* Validation and safety */
bool dsrtos_switch_is_safe(void);

bool dsrtos_switch_is_allowed(
    uint16_t from_id, 
    uint16_t to_id);

uint32_t dsrtos_switch_estimate_time(
    uint16_t from_id, 
    uint16_t to_id);

dsrtos_status_t dsrtos_switch_validate_request(
    const dsrtos_switch_request_t* request);

/* Policy configuration */
dsrtos_status_t dsrtos_switch_set_policy(
    uint32_t min_interval_ms,
    uint32_t max_duration_us,
    bool allow_forced);

dsrtos_status_t dsrtos_switch_register_validator(
    bool (*validator)(const dsrtos_switch_request_t*));

dsrtos_status_t dsrtos_switch_register_migrator(
    dsrtos_status_t (*migrator)(dsrtos_tcb_t*, void*, void*));

/* Statistics and history */
dsrtos_status_t dsrtos_switch_get_stats(
    dsrtos_switch_stats_t* stats);

dsrtos_status_t dsrtos_switch_get_history(
    dsrtos_switch_history_t* history,
    uint32_t* count);

dsrtos_status_t dsrtos_switch_clear_history(void);

/* Debugging and diagnostics */
dsrtos_switch_phase_t dsrtos_switch_get_phase(void);

const char* dsrtos_switch_get_phase_name(
    dsrtos_switch_phase_t phase);

dsrtos_status_t dsrtos_switch_get_last_error(
    uint32_t* error_code,
    const char** error_msg);

void dsrtos_switch_dump_context(
    const dsrtos_switch_context_t* context);

/* ============================================================================
 * INLINE UTILITIES
 * ============================================================================ */

/* Check if switch is in progress */
static inline bool dsrtos_switch_in_progress(void)
{
    extern dsrtos_switch_controller_t g_switch_controller;
    return g_switch_controller.switch_in_progress;
}

/* Get current switch duration */
static inline uint32_t dsrtos_switch_get_duration(void)
{
    extern dsrtos_switch_controller_t g_switch_controller;
    if (g_switch_controller.current_switch != NULL) {
        return g_switch_controller.current_switch->timing.completion_time - 
               g_switch_controller.current_switch->timing.start_time;
    }
    return 0U;
}

#endif /* DSRTOS_SWITCH_H */
