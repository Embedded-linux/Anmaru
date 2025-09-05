/* ============================================================================
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Task Migration Engine Implementation
 * 
 * Implements advanced task migration strategies for scheduler switching.
 * ============================================================================ */

#include "dsrtos_switch.h"
#include "dsrtos_migration.h"
#include "dsrtos_kernel.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_scheduler.h"
#include <string.h>

/* ============================================================================
 * MIGRATION STRATEGY DEFINITIONS
 * ============================================================================ */

/* Migration context for advanced strategies */
typedef struct migration_context {
    dsrtos_tcb_t** task_list;             /* Tasks to migrate */
    uint32_t task_count;                   /* Number of tasks */
    uint16_t from_scheduler_id;            /* Source scheduler */
    uint16_t to_scheduler_id;              /* Target scheduler */
    dsrtos_migration_strategy_t strategy;  /* Migration strategy */
    uint32_t* priority_map;               /* Priority mapping table */
    uint32_t* deadline_map;               /* Deadline mapping table */
    void* custom_data;                     /* Strategy-specific data */
} migration_context_t;

/* Task comparison function for sorting */
typedef int (*task_compare_fn)(const dsrtos_tcb_t*, const dsrtos_tcb_t*);

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

static dsrtos_status_t migrate_preserve_order(migration_context_t* ctx);
static dsrtos_status_t migrate_priority_based(migration_context_t* ctx);
static dsrtos_status_t migrate_deadline_based(migration_context_t* ctx);
static dsrtos_status_t migrate_custom_strategy(migration_context_t* ctx);

static void sort_tasks(dsrtos_tcb_t** tasks, uint32_t count, task_compare_fn compare);
static int compare_by_priority(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b);
static int compare_by_deadline(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b);
static int compare_by_creation_time(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b);

static uint32_t map_priority(uint32_t priority, uint16_t from_id, uint16_t to_id);
static uint32_t calculate_deadline_priority(dsrtos_tcb_t* task);
static bool validate_task_migration(dsrtos_tcb_t* task, uint16_t to_scheduler);

static dsrtos_status_t prepare_task_for_migration(dsrtos_tcb_t* task);
static dsrtos_status_t finalize_task_migration(dsrtos_tcb_t* task, uint16_t to_scheduler);
static dsrtos_status_t handle_migration_failure(dsrtos_tcb_t* task, uint16_t from_scheduler);

/* ============================================================================
 * PUBLIC MIGRATION API
 * ============================================================================ */

/**
 * @brief Execute task migration with specified strategy
 * MISRA-C:2012 Dir 4.9: Function-like macros avoided
 */
dsrtos_status_t dsrtos_migrate_tasks_strategic(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy,
    void* custom_data)
{
    migration_context_t context = {0};
    dsrtos_status_t status;
    
    /* Validate parameters */
    if (tasks == NULL || count == 0U) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (from_scheduler == to_scheduler) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (strategy >= MIGRATE_STRATEGY_COUNT) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Initialize migration context */
    context.task_list = tasks;
    context.task_count = count;
    context.from_scheduler_id = from_scheduler;
    context.to_scheduler_id = to_scheduler;
    context.strategy = strategy;
    context.custom_data = custom_data;
    
    /* Execute migration based on strategy */
    switch (strategy) {
        case MIGRATE_PRESERVE_ORDER:
            status = migrate_preserve_order(&context);
            break;
            
        case MIGRATE_PRIORITY_BASED:
            status = migrate_priority_based(&context);
            break;
            
        case MIGRATE_DEADLINE_BASED:
            status = migrate_deadline_based(&context);
            break;
            
        case MIGRATE_CUSTOM:
            status = migrate_custom_strategy(&context);
            break;
            
        default:
            status = DSRTOS_INVALID_PARAM;
            break;
    }
    
    return status;
}

/**
 * @brief Batch migrate tasks with progress tracking
 */
dsrtos_status_t dsrtos_migrate_batch_with_progress(
    dsrtos_tcb_t** tasks,
    uint32_t count,
    uint16_t from_scheduler,
    uint16_t to_scheduler,
    dsrtos_migration_strategy_t strategy,
    void (*progress_callback)(uint32_t completed, uint32_t total))
{
    dsrtos_status_t status;
    uint32_t batch_size = DSRTOS_MAX_MIGRATION_BATCH;
    uint32_t completed = 0U;
    
    /* Validate parameters */
    if (tasks == NULL || count == 0U) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Process in batches */
    while (completed < count) {
        uint32_t batch_count = (count - completed);
        if (batch_count > batch_size) {
            batch_count = batch_size;
        }
        
        /* Migrate batch */
        status = dsrtos_migrate_tasks_strategic(
            &tasks[completed],
            batch_count,
            from_scheduler,
            to_scheduler,
            strategy,
            NULL);
        
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
        
        completed += batch_count;
        
        /* Report progress */
        if (progress_callback != NULL) {
            progress_callback(completed, count);
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Validate task migration feasibility
 */
bool dsrtos_can_migrate_task(dsrtos_tcb_t* task, uint16_t to_scheduler)
{
    dsrtos_scheduler_plugin_t* scheduler;
    
    if (task == NULL) {
        return false;
    }
    
    /* Check task state */
    if (task->state == TASK_STATE_DELETED ||
        task->state == TASK_STATE_SUSPENDED) {
        return false;
    }
    
    /* Get target scheduler */
    scheduler = dsrtos_scheduler_get_by_id(to_scheduler);
    if (scheduler == NULL) {
        return false;
    }
    
    /* Check scheduler-specific constraints */
    if (scheduler->operations.can_accept_task != NULL) {
        return scheduler->operations.can_accept_task(scheduler, task);
    }
    
    /* Additional validation */
    return validate_task_migration(task, to_scheduler);
}

/* ============================================================================
 * MIGRATION STRATEGIES
 * ============================================================================ */

/**
 * @brief Migrate tasks preserving relative order
 */
static dsrtos_status_t migrate_preserve_order(migration_context_t* ctx)
{
    dsrtos_status_t status;
    dsrtos_scheduler_plugin_t* from_sched;
    dsrtos_scheduler_plugin_t* to_sched;
    
    /* Get schedulers */
    from_sched = dsrtos_scheduler_get_by_id(ctx->from_scheduler_id);
    to_sched = dsrtos_scheduler_get_by_id(ctx->to_scheduler_id);
    
    if (from_sched == NULL || to_sched == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    /* Sort tasks by creation time to preserve order */
    sort_tasks(ctx->task_list, ctx->task_count, compare_by_creation_time);
    
    /* Migrate each task in order */
    for (uint32_t i = 0U; i < ctx->task_count; i++) {
        dsrtos_tcb_t* task = ctx->task_list[i];
        
        /* Prepare task for migration */
        status = prepare_task_for_migration(task);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
        
        /* Remove from source scheduler */
        if (from_sched->operations.remove_task != NULL) {
            status = from_sched->operations.remove_task(from_sched, task);
            if (status != DSRTOS_SUCCESS) {
                return status;
            }
        }
        
        /* Add to target scheduler preserving relative position */
        if (to_sched->operations.add_task != NULL) {
            status = to_sched->operations.add_task(to_sched, task);
            if (status != DSRTOS_SUCCESS) {
                /* Rollback - try to restore to original scheduler */
                (void)handle_migration_failure(task, ctx->from_scheduler_id);
                return status;
            }
        }
        
        /* Finalize migration */
        status = finalize_task_migration(task, ctx->to_scheduler_id);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate tasks based on priority mapping
 */
static dsrtos_status_t migrate_priority_based(migration_context_t* ctx)
{
    dsrtos_status_t status;
    dsrtos_scheduler_plugin_t* from_sched;
    dsrtos_scheduler_plugin_t* to_sched;
    
    /* Get schedulers */
    from_sched = dsrtos_scheduler_get_by_id(ctx->from_scheduler_id);
    to_sched = dsrtos_scheduler_get_by_id(ctx->to_scheduler_id);
    
    if (from_sched == NULL || to_sched == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    /* Sort tasks by priority */
    sort_tasks(ctx->task_list, ctx->task_count, compare_by_priority);
    
    /* Migrate tasks with priority mapping */
    for (uint32_t i = 0U; i < ctx->task_count; i++) {
        dsrtos_tcb_t* task = ctx->task_list[i];
        uint32_t original_priority = task->priority;
        
        /* Prepare task */
        status = prepare_task_for_migration(task);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
        
        /* Map priority for target scheduler */
        task->priority = map_priority(original_priority, 
                                     ctx->from_scheduler_id,
                                     ctx->to_scheduler_id);
        
        /* Remove from source */
        if (from_sched->operations.remove_task != NULL) {
            status = from_sched->operations.remove_task(from_sched, task);
            if (status != DSRTOS_SUCCESS) {
                task->priority = original_priority;  /* Restore priority */
                return status;
            }
        }
        
        /* Add to target with new priority */
        if (to_sched->operations.add_task != NULL) {
            status = to_sched->operations.add_task(to_sched, task);
            if (status != DSRTOS_SUCCESS) {
                task->priority = original_priority;  /* Restore priority */
                (void)handle_migration_failure(task, ctx->from_scheduler_id);
                return status;
            }
        }
        
        /* Finalize migration */
        status = finalize_task_migration(task, ctx->to_scheduler_id);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate tasks based on deadline urgency
 */
static dsrtos_status_t migrate_deadline_based(migration_context_t* ctx)
{
    dsrtos_status_t status;
    dsrtos_scheduler_plugin_t* from_sched;
    dsrtos_scheduler_plugin_t* to_sched;
    
    /* Get schedulers */
    from_sched = dsrtos_scheduler_get_by_id(ctx->from_scheduler_id);
    to_sched = dsrtos_scheduler_get_by_id(ctx->to_scheduler_id);
    
    if (from_sched == NULL || to_sched == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    /* Sort tasks by deadline */
    sort_tasks(ctx->task_list, ctx->task_count, compare_by_deadline);
    
    /* Migrate tasks with deadline-based priority adjustment */
    for (uint32_t i = 0U; i < ctx->task_count; i++) {
        dsrtos_tcb_t* task = ctx->task_list[i];
        uint32_t original_priority = task->priority;
        
        /* Prepare task */
        status = prepare_task_for_migration(task);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
        
        /* Calculate deadline-based priority */
        task->priority = calculate_deadline_priority(task);
        
        /* Remove from source */
        if (from_sched->operations.remove_task != NULL) {
            status = from_sched->operations.remove_task(from_sched, task);
            if (status != DSRTOS_SUCCESS) {
                task->priority = original_priority;
                return status;
            }
        }
        
        /* Add to target with deadline priority */
        if (to_sched->operations.add_task != NULL) {
            status = to_sched->operations.add_task(to_sched, task);
            if (status != DSRTOS_SUCCESS) {
                task->priority = original_priority;
                (void)handle_migration_failure(task, ctx->from_scheduler_id);
                return status;
            }
        }
        
        /* Finalize migration */
        status = finalize_task_migration(task, ctx->to_scheduler_id);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate tasks using custom strategy
 */
static dsrtos_status_t migrate_custom_strategy(migration_context_t* ctx)
{
    /* Custom strategy implementation would go here */
    /* This could call a user-provided migration function */
    
    if (ctx->custom_data != NULL) {
        /* Use custom data for migration logic */
        typedef dsrtos_status_t (*custom_migrate_fn)(migration_context_t*);
        custom_migrate_fn custom_fn = (custom_migrate_fn)ctx->custom_data;
        return custom_fn(ctx);
    }
    
    /* Default to preserve order if no custom function */
    return migrate_preserve_order(ctx);
}

/* ============================================================================
 * TASK SORTING AND COMPARISON
 * ============================================================================ */

/**
 * @brief Sort tasks using comparison function
 * Simple bubble sort for small task counts
 */
static void sort_tasks(dsrtos_tcb_t** tasks, uint32_t count, task_compare_fn compare)
{
    bool swapped;
    
    if (tasks == NULL || count <= 1U || compare == NULL) {
        return;
    }
    
    /* Bubble sort - acceptable for small task counts */
    for (uint32_t i = 0U; i < count - 1U; i++) {
        swapped = false;
        
        for (uint32_t j = 0U; j < count - i - 1U; j++) {
            if (compare(tasks[j], tasks[j + 1U]) > 0) {
                /* Swap tasks */
                dsrtos_tcb_t* temp = tasks[j];
                tasks[j] = tasks[j + 1U];
                tasks[j + 1U] = temp;
                swapped = true;
            }
        }
        
        /* Early exit if no swaps */
        if (!swapped) {
            break;
        }
    }
}

/**
 * @brief Compare tasks by priority
 */
static int compare_by_priority(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    
    /* Higher priority (lower number) comes first */
    if (a->priority < b->priority) {
        return -1;
    } else if (a->priority > b->priority) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief Compare tasks by deadline
 */
static int compare_by_deadline(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b)
{
    uint32_t deadline_a, deadline_b;
    
    if (a == NULL || b == NULL) {
        return 0;
    }
    
    /* Get deadline from task extended data */
    /* This assumes deadline is stored in task structure */
    deadline_a = a->runtime_stats.total_runtime;  /* Placeholder */
    deadline_b = b->runtime_stats.total_runtime;  /* Placeholder */
    
    /* Earlier deadline comes first */
    if (deadline_a < deadline_b) {
        return -1;
    } else if (deadline_a > deadline_b) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief Compare tasks by creation time
 */
static int compare_by_creation_time(const dsrtos_tcb_t* a, const dsrtos_tcb_t* b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    
    /* Earlier created task comes first */
    if (a->task_id < b->task_id) {  /* Assuming task_id increases with time */
        return -1;
    } else if (a->task_id > b->task_id) {
        return 1;
    }
    
    return 0;
}

/* ============================================================================
 * PRIORITY MAPPING AND CALCULATION
 * ============================================================================ */

/**
 * @brief Map priority between schedulers
 */
static uint32_t map_priority(uint32_t priority, uint16_t from_id, uint16_t to_id)
{
    /* Priority mapping logic based on scheduler types */
    /* This is a simplified linear mapping */
    
    /* Round Robin to Priority scheduler */
    if (from_id == SCHEDULER_ROUND_ROBIN && to_id == SCHEDULER_PRIORITY) {
        /* RR tasks get middle priority range */
        return 128U;  /* Middle priority */
    }
    
    /* Priority to Round Robin */
    if (from_id == SCHEDULER_PRIORITY && to_id == SCHEDULER_ROUND_ROBIN) {
        /* All priorities map to same level in RR */
        return 0U;  /* RR doesn't use priority */
    }
    
    /* EDF to Priority */
    if (from_id == SCHEDULER_EDF && to_id == SCHEDULER_PRIORITY) {
        /* Map based on deadline urgency */
        return (priority < 64U) ? priority : 64U;
    }
    
    /* Default: keep same priority */
    return priority;
}

/**
 * @brief Calculate priority based on deadline
 */
static uint32_t calculate_deadline_priority(dsrtos_tcb_t* task)
{
    uint32_t current_time;
    uint32_t deadline;
    uint32_t time_remaining;
    
    if (task == NULL) {
        return 255U;  /* Lowest priority */
    }
    
    /* Get current time and task deadline */
    current_time = dsrtos_kernel_get_tick_count();
    deadline = task->runtime_stats.total_runtime;  /* Placeholder for deadline */
    
    /* Calculate time remaining */
    if (deadline > current_time) {
        time_remaining = deadline - current_time;
    } else {
        /* Deadline passed - highest priority */
        return 0U;
    }
    
    /* Map time remaining to priority (0-255) */
    /* Less time remaining = higher priority (lower number) */
    if (time_remaining < 10U) {
        return 0U;  /* Urgent */
    } else if (time_remaining < 100U) {
        return 32U;  /* High priority */
    } else if (time_remaining < 1000U) {
        return 128U;  /* Medium priority */
    } else {
        return 192U;  /* Low priority */
    }
}

/* ============================================================================
 * MIGRATION HELPERS
 * ============================================================================ */

/**
 * @brief Validate task migration feasibility
 */
static bool validate_task_migration(dsrtos_tcb_t* task, uint16_t to_scheduler)
{
    /* Check task state compatibility */
    if (task->state == TASK_STATE_RUNNING) {
        /* Running task needs special handling */
        return false;  /* Cannot migrate running task directly */
    }
    
    /* Check scheduler type compatibility */
    if (to_scheduler == SCHEDULER_EDF) {
        /* EDF requires deadline information */
        /* Check if task has deadline data */
        if (task->runtime_stats.total_runtime == 0U) {
            return false;  /* No deadline info */
        }
    }
    
    /* Check resource constraints */
    if (task->stack_size > DSRTOS_MAX_STACK_SIZE) {
        return false;  /* Stack too large */
    }
    
    return true;
}

/**
 * @brief Prepare task for migration
 */
static dsrtos_status_t prepare_task_for_migration(dsrtos_tcb_t* task)
{
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Save task context if needed */
    if (task->state == TASK_STATE_READY) {
        /* Task is ready - save any scheduler-specific data */
        task->sched_data = NULL;  /* Clear scheduler data */
    }
    
    /* Clear any pending events */
    task->event_flags = 0U;
    
    /* Reset timing statistics for new scheduler */
    task->runtime_stats.last_scheduled = 0U;
    task->runtime_stats.last_preempted = 0U;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Finalize task migration
 */
static dsrtos_status_t finalize_task_migration(dsrtos_tcb_t* task, uint16_t to_scheduler)
{
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Update task scheduler assignment */
    /* This would be stored in task structure */
    
    /* Reset scheduler-specific counters */
    task->runtime_stats.context_switches = 0U;
    
    /* Notify task of migration if needed */
    /* Could set a flag for task to check */
    
    /* Unused parameter */
    (void)to_scheduler;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Handle migration failure
 */
static dsrtos_status_t handle_migration_failure(dsrtos_tcb_t* task, uint16_t from_scheduler)
{
    dsrtos_scheduler_plugin_t* scheduler;
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Get original scheduler */
    scheduler = dsrtos_scheduler_get_by_id(from_scheduler);
    if (scheduler == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    /* Try to restore task to original scheduler */
    if (scheduler->operations.add_task != NULL) {
        return scheduler->operations.add_task(scheduler, task);
    }
    
    return DSRTOS_ERROR;
}
