/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Pluggable Scheduler Core - Framework Interface
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

#ifndef DSRTOS_SCHEDULER_CORE_H
#define DSRTOS_SCHEDULER_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_config.h"
#include "dsrtos_error.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
/* Scheduler magic numbers for validation */
#define DSRTOS_SCHEDULER_MAGIC      (0x53434845U)  /* 'SCHE' */
#define DSRTOS_SCHEDULER_VALID      (0x5641414CU)  /* 'VAAL' */

/* Maximum values */
#define DSRTOS_MAX_SCHEDULERS       (16U)
#define DSRTOS_MAX_SCHEDULER_NAME   (32U)

/* Scheduler IDs */
typedef enum {
    DSRTOS_SCHED_ID_NONE = 0,
    DSRTOS_SCHED_ID_ROUND_ROBIN,
    DSRTOS_SCHED_ID_STATIC_PRIORITY,
    DSRTOS_SCHED_ID_EDF,
    DSRTOS_SCHED_ID_CFS,
    DSRTOS_SCHED_ID_RATE_MONOTONIC,
    DSRTOS_SCHED_ID_DEADLINE_MONOTONIC,
    DSRTOS_SCHED_ID_PRIORITY_INHERITANCE,
    DSRTOS_SCHED_ID_ADAPTIVE,
    DSRTOS_SCHED_ID_CUSTOM_BASE = 0x100
} dsrtos_scheduler_id_t;

/* Scheduler capabilities flags */
#define DSRTOS_SCHED_CAP_PREEMPTIVE        (1U << 0)
#define DSRTOS_SCHED_CAP_TIME_SLICING      (1U << 1)
#define DSRTOS_SCHED_CAP_PRIORITY           (1U << 2)
#define DSRTOS_SCHED_CAP_DEADLINE          (1U << 3)
#define DSRTOS_SCHED_CAP_SMP                (1U << 4)
#define DSRTOS_SCHED_CAP_RESOURCE_AWARE    (1U << 5)
#define DSRTOS_SCHED_CAP_IPC_AWARE         (1U << 6)
#define DSRTOS_SCHED_CAP_ENERGY_AWARE      (1U << 7)
#define DSRTOS_SCHED_CAP_REAL_TIME         (1U << 8)
#define DSRTOS_SCHED_CAP_DYNAMIC_PRIORITY  (1U << 9)

/* Scheduler states */
typedef enum {
    DSRTOS_SCHED_STATE_INACTIVE = 0,
    DSRTOS_SCHED_STATE_INITIALIZING,
    DSRTOS_SCHED_STATE_ACTIVE,
    DSRTOS_SCHED_STATE_SWITCHING,
    DSRTOS_SCHED_STATE_SUSPENDED,
    DSRTOS_SCHED_STATE_ERROR
} dsrtos_scheduler_state_t;

/*==============================================================================
 *                                  TYPES
 *============================================================================*/
/* Forward declarations */
typedef struct dsrtos_scheduler_plugin dsrtos_scheduler_plugin_t;
typedef struct dsrtos_scheduler_context dsrtos_scheduler_context_t;
typedef struct dsrtos_scheduler_metrics dsrtos_scheduler_metrics_t;

/* Scheduler operation function pointers */
typedef struct {
    /* Core scheduling operations */
    dsrtos_tcb_t* (*select_next_task)(void* context);
    void (*enqueue_task)(void* context, dsrtos_tcb_t* task);
    void (*dequeue_task)(void* context, dsrtos_tcb_t* task);
    void (*requeue_task)(void* context, dsrtos_tcb_t* task);
    
    /* Task operations */
    void (*task_tick)(void* context, dsrtos_tcb_t* task);
    void (*task_yield)(void* context, dsrtos_tcb_t* task);
    void (*task_block)(void* context, dsrtos_tcb_t* task);
    void (*task_unblock)(void* context, dsrtos_tcb_t* task);
    
    /* Priority operations */
    void (*priority_changed)(void* context, dsrtos_tcb_t* task, 
                           uint8_t old_priority, uint8_t new_priority);
    void (*priority_inherit)(void* context, dsrtos_tcb_t* task, 
                           uint8_t inherited_priority);
    void (*priority_disinherit)(void* context, dsrtos_tcb_t* task);
    
    /* Resource awareness */
    void (*resource_acquired)(void* context, dsrtos_tcb_t* task, 
                            uint32_t resource_id);
    void (*resource_released)(void* context, dsrtos_tcb_t* task, 
                            uint32_t resource_id);
    void (*resource_contention)(void* context, uint32_t resource_id, 
                               uint32_t contention_level);
    
    /* IPC awareness */
    void (*ipc_send)(void* context, dsrtos_tcb_t* sender, 
                    dsrtos_tcb_t* receiver);
    void (*ipc_receive)(void* context, dsrtos_tcb_t* task);
    void (*ipc_blocked)(void* context, dsrtos_tcb_t* task);
    
    /* Timer awareness */
    void (*timer_expired)(void* context, dsrtos_tcb_t* task);
    void (*deadline_updated)(void* context, dsrtos_tcb_t* task, 
                           uint32_t new_deadline);
    
    /* Adaptation triggers */
    bool (*should_switch_scheduler)(void* context);
    uint16_t (*recommend_next_scheduler)(void* context);
    void (*adaptation_hint)(void* context, uint32_t hint_type, 
                          void* hint_data);
} dsrtos_scheduler_ops_t;

/* Scheduler plugin structure */
struct dsrtos_scheduler_plugin {
    /* Identification */
    uint32_t magic;                         /* Magic number for validation */
    uint16_t id;                           /* Unique scheduler ID */
    uint16_t version;                      /* Scheduler version */
    char name[DSRTOS_MAX_SCHEDULER_NAME];  /* Scheduler name */
    
    /* Capabilities */
    uint32_t capabilities;                 /* Capability flags */
    uint32_t max_tasks;                   /* Maximum supported tasks */
    uint32_t priority_levels;             /* Number of priority levels */
    uint32_t time_slice_us;               /* Default time slice in microseconds */
    
    /* Operations */
    dsrtos_scheduler_ops_t ops;           /* Scheduler operations */
    
    /* Lifecycle management */
    dsrtos_error_t (*init)(void** context);
    dsrtos_error_t (*deinit)(void* context);
    dsrtos_error_t (*start)(void* context);
    dsrtos_error_t (*stop)(void* context);
    dsrtos_error_t (*suspend)(void* context);
    dsrtos_error_t (*resume)(void* context);
    
    /* Configuration */
    dsrtos_error_t (*configure)(void* context, const void* config);
    dsrtos_error_t (*get_config)(void* context, void* config);
    
    /* Statistics */
    dsrtos_error_t (*get_statistics)(void* context, 
                                    dsrtos_scheduler_metrics_t* metrics);
    dsrtos_error_t (*reset_statistics)(void* context);
    
    /* Private data */
    void* private_data;                   /* Scheduler-specific private data */
    size_t private_data_size;             /* Size of private data */
    
    /* Validation */
    uint32_t checksum;                    /* Plugin integrity checksum */
};

/* Scheduler metrics structure */
struct dsrtos_scheduler_metrics {
    /* Timing metrics */
    uint64_t total_scheduling_time_ns;    /* Total time in scheduler */
    uint64_t total_idle_time_ns;          /* Total idle time */
    uint32_t scheduling_decisions;        /* Number of scheduling decisions */
    uint32_t context_switches;            /* Number of context switches */
    uint32_t average_decision_time_ns;    /* Average decision time */
    uint32_t max_decision_time_ns;        /* Maximum decision time */
    uint32_t min_decision_time_ns;        /* Minimum decision time */
    
    /* Task metrics */
    uint32_t tasks_scheduled;             /* Tasks scheduled */
    uint32_t tasks_preempted;             /* Tasks preempted */
    uint32_t tasks_yielded;               /* Tasks that yielded */
    uint32_t tasks_blocked;               /* Tasks blocked */
    
    /* Priority metrics */
    uint32_t priority_inversions;         /* Priority inversions detected */
    uint32_t priority_inheritances;       /* Priority inheritances */
    uint32_t priority_changes;            /* Priority changes */
    
    /* Deadline metrics */
    uint32_t deadline_misses;             /* Deadline misses */
    uint32_t deadline_met;                /* Deadlines met */
    
    /* Resource metrics */
    uint32_t resource_contentions;        /* Resource contentions */
    uint32_t resource_acquisitions;       /* Resource acquisitions */
    
    /* IPC metrics */
    uint32_t ipc_operations;              /* IPC operations */
    uint32_t ipc_blocks;                  /* IPC blocks */
};

/* Scheduler context for decision making */
struct dsrtos_scheduler_context {
    /* Current state */
    dsrtos_scheduler_plugin_t* active_scheduler;
    dsrtos_scheduler_state_t state;
    dsrtos_tcb_t* current_task;
    dsrtos_tcb_t* ready_queue_head;
    uint32_t ready_task_count;
    
    /* Resource utilization */
    struct {
        uint8_t cpu_usage_percent;
        uint32_t memory_free;
        uint32_t memory_fragmentation;
        uint32_t resource_contention;
        uint32_t cache_misses;
    } resources;
    
    /* IPC patterns */
    struct {
        uint32_t ipc_rate;
        uint32_t ipc_queue_depth;
        uint32_t ipc_blocking_time;
        uint32_t producer_consumer_ratio;
    } ipc_stats;
    
    /* Timer information */
    struct {
        uint32_t active_timers;
        uint32_t timer_resolution_us;
        uint32_t next_timer_expiry;
        uint32_t high_precision_required;
    } timers;
    
    /* Real-time constraints */
    struct {
        uint32_t hard_deadlines;
        uint32_t soft_deadlines;
        uint32_t deadline_misses;
        uint32_t schedulability_index;
        uint32_t utilization_bound;
    } rt_constraints;
    
    /* Scheduler selection */
    struct {
        uint32_t last_switch_time;
        uint32_t switch_hysteresis_ms;
        uint32_t stability_counter;
        uint16_t preferred_scheduler_id;
        uint32_t switch_count;
    } selection;
    
    /* Performance tracking */
    dsrtos_scheduler_metrics_t metrics;
};

/* Scheduler switch request */
typedef struct {
    uint16_t target_scheduler_id;
    uint32_t switch_reason;
    uint32_t switch_time_ms;
    void* switch_context;
} dsrtos_scheduler_switch_request_t;

/*==============================================================================
 *                           FUNCTION PROTOTYPES
 *============================================================================*/
/* Core scheduler management */
dsrtos_error_t dsrtos_scheduler_core_init(void);
dsrtos_error_t dsrtos_scheduler_core_start(void);
dsrtos_error_t dsrtos_scheduler_core_stop(void);
dsrtos_error_t dsrtos_scheduler_core_deinit(void);

/* Plugin registration */
dsrtos_error_t dsrtos_scheduler_register(dsrtos_scheduler_plugin_t* plugin);
dsrtos_error_t dsrtos_scheduler_unregister(uint16_t scheduler_id);
dsrtos_scheduler_plugin_t* dsrtos_scheduler_get(uint16_t scheduler_id);
uint32_t dsrtos_scheduler_get_count(void);

/* Scheduler activation */
dsrtos_error_t dsrtos_scheduler_activate(uint16_t scheduler_id);
dsrtos_error_t dsrtos_scheduler_deactivate(void);
uint16_t dsrtos_scheduler_get_active_id(void);

/* Dynamic switching */
dsrtos_error_t dsrtos_scheduler_switch(uint16_t new_scheduler_id);
dsrtos_error_t dsrtos_scheduler_switch_request(
    const dsrtos_scheduler_switch_request_t* request);
bool dsrtos_scheduler_is_switching(void);

/* Adaptation */
void dsrtos_scheduler_adapt(void);
void dsrtos_scheduler_adaptation_hint(uint32_t hint_type, void* hint_data);
uint16_t dsrtos_scheduler_get_recommended(void);

/* Context access */
dsrtos_scheduler_context_t* dsrtos_scheduler_get_context(void);
dsrtos_scheduler_metrics_t* dsrtos_scheduler_get_metrics(void);

/* Statistics */
dsrtos_error_t dsrtos_scheduler_get_statistics(
    uint16_t scheduler_id, dsrtos_scheduler_metrics_t* metrics);
dsrtos_error_t dsrtos_scheduler_reset_statistics(uint16_t scheduler_id);

/* Validation */
bool dsrtos_scheduler_validate_plugin(const dsrtos_scheduler_plugin_t* plugin);
uint32_t dsrtos_scheduler_calculate_checksum(
    const dsrtos_scheduler_plugin_t* plugin);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_CORE_H */
