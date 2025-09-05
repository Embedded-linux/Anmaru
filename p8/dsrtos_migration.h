/* ============================================================================
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Task Migration Engine Header
 * 
 * Advanced task migration strategies and utilities for scheduler switching.
 * ============================================================================ */

#ifndef DSRTOS_MIGRATION_H
#define DSRTOS_MIGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task.h"
#include "dsrtos_switch.h"

/* ============================================================================
 * MIGRATION CONFIGURATION
 * ============================================================================ */

/* Migration batch configuration */
#define DSRTOS_MIGRATION_MAX_BATCH_SIZE    32U
#define DSRTOS_MIGRATION_MIN_BATCH_SIZE    4U
#define DSRTOS_MIGRATION_OPTIMAL_BATCH     16U

/* Migration timing */
#define DSRTOS_MIGRATION_TASK_TIMEOUT_US   50U
#define DSRTOS_MIGRATION_BATCH_TIMEOUT_US  500U
#define DSRTOS_MIGRATION_MAX_RETRIES       3U

/* Priority mapping ranges */
#define DSRTOS_PRIORITY_MAP_URGENT         0U
#define DSRTOS_PRIORITY_MAP_HIGH          32U
#define DSRTOS_PRIORITY_MAP_NORMAL        128U
#define DSRTOS_PRIORITY_MAP_LOW           192U
#define DSRTOS_PRIORITY_MAP_IDLE          255U

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Migration options */
typedef struct dsrtos_migration_options {
    bool preserve_state;                  /* Preserve task state */
    bool preserve_priority;               /* Keep original priority */
    bool preserve_affinity;               /* Keep CPU affinity */
    bool atomic_migration;                /* Atomic operation required */
    bool allow_preemption;               /* Allow preemption during migration */
    uint32_t timeout_us;                  /* Migration timeout */
} dsrtos_migration_options_t;

/* Migration statistics */
typedef struct dsrtos_migration_stats {
    uint32_t tasks_migrated;             /* Successfully migrated */
    uint32_t migration_failures;         /* Failed migrations */
    uint32_t total_migration_time_us;    /* Total time spent */
    uint32_t avg_migration_time_us;      /* Average per task */
    uint32_t max_migration_time_us;      /* Maximum time for single task */
    uint32_t priority_adjustments;       /* Priority changes made */
    uint32_t state_preservations;        /* State saves performed */
} dsrtos_migration_stats_t;

/* Priority mapping entry */
typedef struct dsrtos_priority_map_entry {
    uint16_t from_scheduler;             /* Source scheduler ID */
    uint16_t to_scheduler;               /* Target scheduler ID */
    uint32_t from_priority;              /* Source priority */
    uint32_t to_priority;                /* Mapped priority */
} dsrtos_priority_map_entry_t;

/* Migration filter function */
typedef bool (*dsrtos_migration_filter_fn)(dsrtos_tcb_t* task, void* context);

/* Migration transform function */
typedef dsrtos_status_t (*dsrtos_migration_transform_fn)(dsrtos_tcb_t* task, 
                                                         uint16_t from_sched,
                                                         uint16_t to_sched,
                                                         void* context);

/* ============================================================================
 * PUBLIC API - STRATEGIC MIGRATION
 * ============================================================================ */

/* Advanced migration with options */
dsrtos_status_t dsrtos_migrate_tasks_strategic(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy,
    void* custom_data);

/* Batch migration with progress tracking */
dsrtos_status_t dsrtos_migrate_batch_with_progress(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy,
    void (*progress_callback)(uint32_t completed, uint32_t total));

/* Filtered migration */
dsrtos_status_t dsrtos_migrate_filtered_tasks(
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_filter_fn filter,
    void* filter_context,
    dsrtos_migration_strategy_t strategy);

/* ============================================================================
 * PUBLIC API - PRIORITY MAPPING
 * ============================================================================ */

/* Priority mapping configuration */
dsrtos_status_t dsrtos_migration_set_priority_map(
    const dsrtos_priority_map_entry_t* map,
    uint32_t map_size);

dsrtos_status_t dsrtos_migration_get_priority_map(
    dsrtos_priority_map_entry_t* map,
    uint32_t* map_size);

uint32_t dsrtos_migration_map_priority(
    uint32_t priority,
    uint16_t from_scheduler,
    uint16_t to_scheduler);

/* Automatic priority mapping */
dsrtos_status_t dsrtos_migration_auto_map_priorities(
    uint16_t from_scheduler,
    uint16_t to_scheduler);

/* ============================================================================
 * PUBLIC API - MIGRATION VALIDATION
 * ============================================================================ */

/* Check if task can be migrated */
bool dsrtos_can_migrate_task(
    dsrtos_tcb_t* task,
    uint16_t to_scheduler);

/* Validate migration batch */
dsrtos_status_t dsrtos_validate_migration_batch(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler);

/* Estimate migration time */
uint32_t dsrtos_estimate_migration_time(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    dsrtos_migration_strategy_t strategy);

/* Check migration safety */
bool dsrtos_is_migration_safe(
    uint16_t from_scheduler,
    uint16_t to_scheduler);

/* ============================================================================
 * PUBLIC API - MIGRATION OPTIONS
 * ============================================================================ */

/* Set migration options */
dsrtos_status_t dsrtos_migration_set_options(
    const dsrtos_migration_options_t* options);

dsrtos_status_t dsrtos_migration_get_options(
    dsrtos_migration_options_t* options);

/* Configure batch size */
dsrtos_status_t dsrtos_migration_set_batch_size(
    uint32_t batch_size);

uint32_t dsrtos_migration_get_optimal_batch_size(
    uint32_t total_tasks);

/* ============================================================================
 * PUBLIC API - CUSTOM MIGRATION
 * ============================================================================ */

/* Register custom migration transform */
dsrtos_status_t dsrtos_migration_register_transform(
    dsrtos_migration_transform_fn transform,
    void* context);

/* Execute custom migration */
dsrtos_status_t dsrtos_migration_execute_custom(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_transform_fn transform,
    void* context);

/* ============================================================================
 * PUBLIC API - MIGRATION STATISTICS
 * ============================================================================ */

/* Get migration statistics */
dsrtos_status_t dsrtos_migration_get_stats(
    dsrtos_migration_stats_t* stats);

dsrtos_status_t dsrtos_migration_reset_stats(void);

/* Performance metrics */
uint32_t dsrtos_migration_get_last_duration(void);
uint32_t dsrtos_migration_get_average_duration(void);

/* ============================================================================
 * PUBLIC API - TASK STATE PRESERVATION
 * ============================================================================ */

/* Save task state for migration */
dsrtos_status_t dsrtos_migration_save_task_state(
    dsrtos_tcb_t* task,
    void* buffer,
    uint32_t* size);

/* Restore task state after migration */
dsrtos_status_t dsrtos_migration_restore_task_state(
    dsrtos_tcb_t* task,
    const void* buffer,
    uint32_t size);

/* Preserve task context */
dsrtos_status_t dsrtos_migration_preserve_context(
    dsrtos_tcb_t* task);

/* ============================================================================
 * INLINE UTILITIES
 * ============================================================================ */

/**
 * @brief Check if migration strategy is valid
 */
static inline bool dsrtos_migration_is_valid_strategy(
    dsrtos_migration_strategy_t strategy)
{
    return (strategy < MIGRATE_STRATEGY_COUNT);
}

/**
 * @brief Calculate migration batch count
 */
static inline uint32_t dsrtos_migration_calculate_batches(
    uint32_t total_tasks,
    uint32_t batch_size)
{
    if (batch_size == 0U) {
        batch_size = DSRTOS_MIGRATION_OPTIMAL_BATCH;
    }
    return (total_tasks + batch_size - 1U) / batch_size;
}

/**
 * @brief Check if task is migratable
 */
static inline bool dsrtos_migration_task_is_migratable(dsrtos_tcb_t* task)
{
    if (task == NULL) {
        return false;
    }
    
    /* Cannot migrate deleted or suspended tasks */
    return (task->state != TASK_STATE_DELETED &&
            task->state != TASK_STATE_SUSPENDED);
}

#endif /* DSRTOS_MIGRATION_H */
