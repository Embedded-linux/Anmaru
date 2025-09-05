/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Dynamic Scheduler Manager Interface
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

#ifndef DSRTOS_SCHEDULER_MANAGER_H
#define DSRTOS_SCHEDULER_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_scheduler_core.h"
#include "dsrtos_task_manager.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
/* Decision thresholds */
#define DSRTOS_HIGH_CPU_THRESHOLD      (80U)   /* 80% CPU usage */
#define DSRTOS_LOW_CPU_THRESHOLD       (30U)   /* 30% CPU usage */
#define DSRTOS_HIGH_IPC_RATE           (1000U) /* 1000 msgs/sec */
#define DSRTOS_HIGH_CONTENTION         (50U)   /* 50% contention */
#define DSRTOS_SWITCH_HYSTERESIS_MS    (100U)  /* 100ms minimum between switches */

/* Scheduler selection matrix dimensions */
#define DSRTOS_CPU_LEVELS              (5U)    /* CPU usage levels */
#define DSRTOS_IPC_LEVELS              (5U)    /* IPC rate levels */
#define DSRTOS_WORKLOAD_TYPES          (8U)    /* Workload classifications */

/*==============================================================================
 *                                  TYPES
 *============================================================================*/
/* Workload types */
typedef enum {
    DSRTOS_WORKLOAD_IDLE = 0,
    DSRTOS_WORKLOAD_PERIODIC,
    DSRTOS_WORKLOAD_APERIODIC,
    DSRTOS_WORKLOAD_MIXED,
    DSRTOS_WORKLOAD_REAL_TIME,
    DSRTOS_WORKLOAD_INTERACTIVE,
    DSRTOS_WORKLOAD_BATCH,
    DSRTOS_WORKLOAD_ADAPTIVE
} dsrtos_workload_type_t;

/* Scheduler decision factors */
typedef struct {
    /* CPU metrics */
    uint8_t cpu_usage;
    uint8_t cpu_variance;
    uint32_t context_switch_rate;
    
    /* Memory metrics */
    uint32_t memory_pressure;
    uint32_t fragmentation;
    
    /* IPC metrics */
    uint32_t ipc_rate;
    uint32_t ipc_latency;
    uint32_t message_queue_depth;
    
    /* Real-time metrics */
    uint32_t deadline_miss_rate;
    uint32_t jitter;
    uint32_t worst_case_latency;
    
    /* Resource metrics */
    uint32_t resource_contention;
    uint32_t priority_inversions;
    
    /* Task metrics */
    uint32_t task_count;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
} dsrtos_decision_factors_t;

/* Scheduler selection policy */
typedef struct {
    /* Thresholds */
    uint8_t cpu_threshold_high;
    uint8_t cpu_threshold_low;
    uint32_t ipc_threshold_high;
    uint32_t contention_threshold;
    uint32_t deadline_miss_threshold;
    
    /* Hysteresis */
    uint32_t switch_hysteresis_ms;
    uint32_t stability_threshold;
    
    /* Weights for decision factors */
    uint8_t cpu_weight;
    uint8_t ipc_weight;
    uint8_t deadline_weight;
    uint8_t contention_weight;
    
    /* Preferred schedulers */
    uint16_t preferred_rt_scheduler;
    uint16_t preferred_general_scheduler;
    uint16_t fallback_scheduler;
} dsrtos_selection_policy_t;

/* Scheduler transition state */
typedef struct {
    /* Current transition */
    uint16_t from_scheduler_id;
    uint16_t to_scheduler_id;
    uint32_t transition_start_time;
    uint32_t expected_duration_ms;
    
    /* Task migration */
    uint32_t tasks_to_migrate;
    uint32_t tasks_migrated;
    dsrtos_tcb_t* migration_list;
    
    /* Synchronization */
    uint32_t sync_point_reached;
    uint32_t rollback_possible;
} dsrtos_transition_state_t;

/*==============================================================================
 *                           FUNCTION PROTOTYPES
 *============================================================================*/
/* Manager initialization */
dsrtos_error_t dsrtos_scheduler_manager_init(void);
dsrtos_error_t dsrtos_scheduler_manager_deinit(void);

/* Policy management */
dsrtos_error_t dsrtos_scheduler_set_policy(
    const dsrtos_selection_policy_t* policy);
dsrtos_error_t dsrtos_scheduler_get_policy(
    dsrtos_selection_policy_t* policy);
dsrtos_error_t dsrtos_scheduler_reset_policy(void);

/* Workload analysis */
dsrtos_workload_type_t dsrtos_scheduler_analyze_workload(void);
void dsrtos_scheduler_get_decision_factors(
    dsrtos_decision_factors_t* factors);

/* Dynamic selection */
uint16_t dsrtos_scheduler_select_optimal(
    const dsrtos_decision_factors_t* factors);
bool dsrtos_scheduler_should_switch(void);
dsrtos_error_t dsrtos_scheduler_request_switch(
    uint16_t target_id, uint32_t reason);

/* Transition management */
dsrtos_error_t dsrtos_scheduler_begin_transition(
    uint16_t from_id, uint16_t to_id);
dsrtos_error_t dsrtos_scheduler_migrate_tasks(void);
dsrtos_error_t dsrtos_scheduler_complete_transition(void);
dsrtos_error_t dsrtos_scheduler_rollback_transition(void);
void dsrtos_scheduler_get_transition_state(
    dsrtos_transition_state_t* state);

/* Performance monitoring */
void dsrtos_scheduler_update_metrics(void);
uint32_t dsrtos_scheduler_get_switch_overhead(void);
uint32_t dsrtos_scheduler_get_decision_time(void);

/* Validation */
bool dsrtos_scheduler_validate_switch(
    uint16_t from_id, uint16_t to_id);
bool dsrtos_scheduler_is_compatible(
    uint16_t scheduler_id, dsrtos_workload_type_t workload);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_MANAGER_H */
