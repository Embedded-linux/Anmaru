/* ============================================================================
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Dynamic Scheduler Switching Implementation
 * 
 * Implements runtime scheduler switching with safe task migration,
 * state preservation, and rollback capabilities.
 * ============================================================================ */

#include "dsrtos_switch.h"
#include "dsrtos_kernel.h"
#include "dsrtos_critical.h"
#include "dsrtos_memory.h"
#include <string.h>

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

/* Global switch controller instance */
dsrtos_switch_controller_t g_switch_controller = {0};

/* Static context for current switch */
static dsrtos_switch_context_t g_switch_context = {0};

/* State preservation buffer */
static uint8_t g_state_buffer[DSRTOS_STATE_BUFFER_SIZE] = {0};

/* Migration records buffer */
static dsrtos_migration_record_t g_migration_records[DSRTOS_MAX_TASKS] = {0};

/* Queue snapshot buffers */
static dsrtos_tcb_t* g_snapshot_tasks[DSRTOS_MAX_TASKS] = {0};
static uint32_t g_snapshot_priorities[DSRTOS_MAX_TASKS] = {0};
static uint32_t g_snapshot_states[DSRTOS_MAX_TASKS] = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

static uint32_t calculate_checksum(const void* data, uint32_t size);
static dsrtos_status_t save_scheduler_state(void* scheduler, uint16_t id, 
                                           void* buffer, uint32_t* size);
static dsrtos_status_t restore_scheduler_state(void* scheduler, uint16_t id,
                                              const void* buffer, uint32_t size);
static dsrtos_status_t migrate_single_task(dsrtos_tcb_t* task,
                                          void* from_sched, void* to_sched,
                                          dsrtos_migration_strategy_t strategy);
static dsrtos_status_t create_queue_snapshot(dsrtos_queue_snapshot_t* snapshot,
                                            dsrtos_tcb_t* queue_head);
static dsrtos_status_t restore_queue_snapshot(const dsrtos_queue_snapshot_t* snapshot,
                                             void* scheduler);
static void record_switch_history(uint16_t from, uint16_t to, 
                                 dsrtos_switch_reason_t reason,
                                 uint32_t duration, bool success);
static uint32_t get_system_time_us(void);
static void enter_critical_switch(void);
static void exit_critical_switch(void);

/* ============================================================================
 * CONTROLLER INITIALIZATION
 * ============================================================================ */

/**
 * @brief Initialize the switch controller
 * MISRA-C:2012 Dir 4.9: Function-like macros avoided
 */
dsrtos_status_t dsrtos_switch_controller_init(dsrtos_switch_controller_t* controller)
{
    /* MISRA-C:2012 Rule 14.3: Validate parameters */
    if (controller == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Initialize controller structure */
    (void)memset(controller, 0, sizeof(dsrtos_switch_controller_t));
    
    /* Set default policies */
    controller->policies.min_interval_ms = DSRTOS_MIN_SWITCH_INTERVAL_MS;
    controller->policies.max_duration_us = DSRTOS_MAX_SWITCH_TIME_US;
    controller->policies.max_critical_us = DSRTOS_SWITCH_CRITICAL_TIME_US;
    controller->policies.allow_forced = false;
    controller->policies.allow_runtime = true;
    controller->policies.require_idle = false;
    
    /* Initialize statistics */
    controller->stats.min_switch_time_us = UINT32_MAX;
    
    /* Mark as initialized */
    controller->initialized = true;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Deinitialize the switch controller
 */
dsrtos_status_t dsrtos_switch_controller_deinit(dsrtos_switch_controller_t* controller)
{
    if (controller == NULL || !controller->initialized) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Abort any ongoing switch */
    if (controller->switch_in_progress) {
        (void)dsrtos_switch_abort();
    }
    
    /* Clear controller state */
    (void)memset(controller, 0, sizeof(dsrtos_switch_controller_t));
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Reset switch controller statistics
 */
dsrtos_status_t dsrtos_switch_controller_reset(dsrtos_switch_controller_t* controller)
{
    if (controller == NULL || !controller->initialized) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (controller->switch_in_progress) {
        return DSRTOS_BUSY;
    }
    
    /* Reset statistics */
    (void)memset(&controller->stats, 0, sizeof(dsrtos_switch_stats_t));
    controller->stats.min_switch_time_us = UINT32_MAX;
    
    /* Clear history */
    (void)memset(controller->history, 0, sizeof(controller->history));
    controller->history_index = 0U;
    
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * SWITCH EXECUTION
 * ============================================================================ */

/**
 * @brief Execute scheduler switch
 * MISRA-C:2012 Rule 8.13: Parameters not pointed to are const-qualified
 */
dsrtos_status_t dsrtos_switch_scheduler(uint16_t from_id, uint16_t to_id, bool forced)
{
    dsrtos_switch_request_t request = {0};
    
    /* Build switch request */
    request.from_scheduler_id = from_id;
    request.to_scheduler_id = to_id;
    request.reason = forced ? SWITCH_REASON_MANUAL : SWITCH_REASON_PERFORMANCE;
    request.strategy = MIGRATE_PRESERVE_ORDER;
    request.requested_time = get_system_time_us();
    request.deadline_us = DSRTOS_MAX_SWITCH_TIME_US;
    request.forced = forced;
    request.atomic = true;
    
    return dsrtos_switch_scheduler_ex(&request);
}

/**
 * @brief Execute scheduler switch with extended parameters
 */
dsrtos_status_t dsrtos_switch_scheduler_ex(const dsrtos_switch_request_t* request)
{
    dsrtos_status_t status;
    uint32_t start_time;
    
    if (request == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    if (g_switch_controller.switch_in_progress) {
        return DSRTOS_BUSY;
    }
    
    /* Validate request */
    status = dsrtos_switch_validate_request(request);
    if (status != DSRTOS_SUCCESS) {
        return status;
    }
    
    /* Check if switch is allowed */
    if (!dsrtos_switch_is_allowed(request->from_scheduler_id, 
                                  request->to_scheduler_id)) {
        return DSRTOS_NOT_ALLOWED;
    }
    
    /* Initialize switch context */
    (void)memset(&g_switch_context, 0, sizeof(dsrtos_switch_context_t));
    g_switch_context.request = *request;
    g_switch_context.preservation.state_buffer = g_state_buffer;
    g_switch_context.preservation.buffer_size = DSRTOS_STATE_BUFFER_SIZE;
    g_switch_context.migration.records = g_migration_records;
    
    /* Set up as current switch */
    g_switch_controller.current_switch = &g_switch_context;
    g_switch_controller.switch_in_progress = true;
    
    /* Record start time */
    start_time = get_system_time_us();
    g_switch_context.timing.start_time = start_time;
    
    /* Phase 1: Prepare */
    g_switch_context.phase = SWITCH_PREPARING;
    status = dsrtos_switch_prepare(&g_switch_context);
    if (status != DSRTOS_SUCCESS) {
        goto error_exit;
    }
    
    /* Phase 2: Validate */
    g_switch_context.phase = SWITCH_VALIDATING;
    status = dsrtos_switch_validate(&g_switch_context);
    if (status != DSRTOS_SUCCESS) {
        goto error_exit;
    }
    
    /* Phase 3: Execute switch */
    status = dsrtos_switch_execute(&g_switch_context);
    if (status != DSRTOS_SUCCESS) {
        /* Attempt rollback */
        if (g_switch_context.rollback.rollback_possible) {
            (void)dsrtos_switch_rollback(&g_switch_context);
        }
        goto error_exit;
    }
    
    /* Phase 4: Complete */
    g_switch_context.phase = SWITCH_COMPLETE;
    status = dsrtos_switch_complete(&g_switch_context);
    
    /* Record completion time */
    g_switch_context.timing.completion_time = get_system_time_us();
    uint32_t duration = g_switch_context.timing.completion_time - start_time;
    
    /* Update statistics */
    g_switch_controller.stats.total_switches++;
    g_switch_controller.stats.successful_switches++;
    g_switch_controller.stats.last_switch_time_us = duration;
    
    if (duration < g_switch_controller.stats.min_switch_time_us) {
        g_switch_controller.stats.min_switch_time_us = duration;
    }
    if (duration > g_switch_controller.stats.max_switch_time_us) {
        g_switch_controller.stats.max_switch_time_us = duration;
    }
    
    /* Update average */
    uint64_t total_time = g_switch_controller.stats.avg_switch_time_us * 
                         (g_switch_controller.stats.successful_switches - 1U);
    g_switch_controller.stats.avg_switch_time_us = 
        (uint32_t)((total_time + duration) / g_switch_controller.stats.successful_switches);
    
    /* Record in history */
    record_switch_history(request->from_scheduler_id, request->to_scheduler_id,
                         request->reason, duration, true);
    
    /* Clear current switch */
    g_switch_controller.current_switch = NULL;
    g_switch_controller.switch_in_progress = false;
    
    return DSRTOS_SUCCESS;
    
error_exit:
    /* Update failure statistics */
    g_switch_controller.stats.total_switches++;
    g_switch_controller.stats.failed_switches++;
    
    /* Record failure in history */
    uint32_t fail_duration = get_system_time_us() - start_time;
    record_switch_history(request->from_scheduler_id, request->to_scheduler_id,
                         request->reason, fail_duration, false);
    
    /* Clear current switch */
    g_switch_controller.current_switch = NULL;
    g_switch_controller.switch_in_progress = false;
    g_switch_context.phase = SWITCH_FAILED;
    
    return status;
}

/**
 * @brief Abort ongoing scheduler switch
 */
dsrtos_status_t dsrtos_switch_abort(void)
{
    if (!g_switch_controller.switch_in_progress) {
        return DSRTOS_NOT_ACTIVE;
    }
    
    /* Attempt rollback if possible */
    if (g_switch_context.rollback.rollback_possible) {
        return dsrtos_switch_rollback(&g_switch_context);
    }
    
    /* Force abort */
    g_switch_context.phase = SWITCH_FAILED;
    g_switch_controller.switch_in_progress = false;
    g_switch_controller.current_switch = NULL;
    
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * SWITCH PHASES
 * ============================================================================ */

/**
 * @brief Prepare for scheduler switch
 */
dsrtos_status_t dsrtos_switch_prepare(dsrtos_switch_context_t* context)
{
    dsrtos_kernel_state_t* kernel;
    dsrtos_scheduler_plugin_t* from_scheduler;
    dsrtos_scheduler_plugin_t* to_scheduler;
    uint32_t task_count = 0U;
    
    if (context == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Get kernel state */
    kernel = dsrtos_kernel_get_state();
    if (kernel == NULL) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    /* Get schedulers */
    from_scheduler = dsrtos_scheduler_get_by_id(context->request.from_scheduler_id);
    to_scheduler = dsrtos_scheduler_get_by_id(context->request.to_scheduler_id);
    
    if (from_scheduler == NULL || to_scheduler == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    /* Count tasks to migrate */
    dsrtos_tcb_t* task = dsrtos_task_get_list();
    while (task != NULL) {
        if (task->state != TASK_STATE_DELETED) {
            task_count++;
        }
        task = task->next;
    }
    
    /* Allocate task list */
    context->migration.task_count = task_count;
    context->migration.task_list = g_snapshot_tasks;  /* Use static buffer */
    
    /* Build task list */
    uint32_t index = 0U;
    task = dsrtos_task_get_list();
    while (task != NULL && index < task_count) {
        if (task->state != TASK_STATE_DELETED) {
            context->migration.task_list[index++] = task;
        }
        task = task->next;
    }
    
    /* Determine batch size */
    context->migration.batch_size = (task_count > DSRTOS_MAX_MIGRATION_BATCH) ?
                                    DSRTOS_MAX_MIGRATION_BATCH : task_count;
    
    /* Initialize rollback state */
    context->rollback.rollback_possible = true;
    context->rollback.saved_state = NULL;
    context->rollback.rollback_point = get_system_time_us();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Validate switch request
 */
dsrtos_status_t dsrtos_switch_validate(dsrtos_switch_context_t* context)
{
    uint32_t estimated_time;
    
    if (context == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Check timing constraints */
    estimated_time = dsrtos_switch_estimate_time(
        context->request.from_scheduler_id,
        context->request.to_scheduler_id);
    
    if (estimated_time > context->request.deadline_us) {
        return DSRTOS_TIMEOUT;
    }
    
    /* Run custom validation if registered */
    if (g_switch_controller.validation.pre_switch_check != NULL) {
        if (!g_switch_controller.validation.pre_switch_check(&context->request)) {
            return DSRTOS_VALIDATION_FAILED;
        }
    }
    
    /* Validate each task migration */
    if (g_switch_controller.validation.validate_migration != NULL) {
        for (uint32_t i = 0U; i < context->migration.task_count; i++) {
            if (!g_switch_controller.validation.validate_migration(
                    context->migration.task_list[i],
                    context->request.to_scheduler_id)) {
                return DSRTOS_VALIDATION_FAILED;
            }
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Execute the scheduler switch
 */
dsrtos_status_t dsrtos_switch_execute(dsrtos_switch_context_t* context)
{
    dsrtos_status_t status;
    dsrtos_kernel_state_t* kernel;
    dsrtos_scheduler_plugin_t* from_scheduler;
    dsrtos_scheduler_plugin_t* to_scheduler;
    uint32_t critical_start, critical_end;
    
    if (context == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Get kernel and schedulers */
    kernel = dsrtos_kernel_get_state();
    from_scheduler = dsrtos_scheduler_get_by_id(context->request.from_scheduler_id);
    to_scheduler = dsrtos_scheduler_get_by_id(context->request.to_scheduler_id);
    
    /* Phase: Save current state */
    context->phase = SWITCH_SAVING_STATE;
    uint32_t state_size = context->preservation.buffer_size;
    status = save_scheduler_state(from_scheduler, 
                                 context->request.from_scheduler_id,
                                 context->preservation.state_buffer,
                                 &state_size);
    if (status != DSRTOS_SUCCESS) {
        return status;
    }
    context->preservation.used_size = state_size;
    context->preservation.state_saved = true;
    
    /* Create queue snapshots for rollback */
    status = create_queue_snapshot(&context->rollback.ready_snapshot,
                                  kernel->ready_list);
    if (status != DSRTOS_SUCCESS) {
        return status;
    }
    
    status = create_queue_snapshot(&context->rollback.blocked_snapshot,
                                  kernel->blocked_list);
    if (status != DSRTOS_SUCCESS) {
        return status;
    }
    
    /* Enter critical section */
    context->phase = SWITCH_ENTERING_CRITICAL;
    critical_start = get_system_time_us();
    enter_critical_switch();
    context->timing.critical_entry = critical_start;
    
    /* Phase: Migrate tasks */
    context->phase = SWITCH_MIGRATING_TASKS;
    uint32_t batch_start = 0U;
    
    while (batch_start < context->migration.task_count) {
        uint32_t batch_end = batch_start + context->migration.batch_size;
        if (batch_end > context->migration.task_count) {
            batch_end = context->migration.task_count;
        }
        
        /* Migrate batch */
        for (uint32_t i = batch_start; i < batch_end; i++) {
            dsrtos_tcb_t* task = context->migration.task_list[i];
            
            /* Record migration start */
            context->migration.records[i].task = task;
            context->migration.records[i].original_state = task->state;
            context->migration.records[i].original_priority = task->priority;
            
            /* Perform migration */
            status = migrate_single_task(task, from_scheduler, to_scheduler,
                                        context->request.strategy);
            
            if (status == DSRTOS_SUCCESS) {
                context->migration.migrated_count++;
                context->migration.records[i].success = true;
            } else {
                context->migration.failed_count++;
                context->migration.records[i].success = false;
                
                /* Exit critical section and rollback */
                exit_critical_switch();
                return status;
            }
        }
        
        batch_start = batch_end;
    }
    
    /* Phase: Activate new scheduler */
    context->phase = SWITCH_ACTIVATING_NEW;
    kernel->active_scheduler = to_scheduler;
    kernel->scheduler_state.active_scheduler_id = context->request.to_scheduler_id;
    
    /* Initialize new scheduler if needed */
    if (to_scheduler->operations.init != NULL) {
        status = to_scheduler->operations.init(to_scheduler);
        if (status != DSRTOS_SUCCESS) {
            exit_critical_switch();
            return status;
        }
    }
    
    /* Exit critical section */
    context->phase = SWITCH_EXITING_CRITICAL;
    critical_end = get_system_time_us();
    exit_critical_switch();
    context->timing.critical_exit = critical_end;
    
    /* Record critical section time */
    uint32_t critical_time = critical_end - critical_start;
    context->metrics.critical_time = critical_time;
    
    if (critical_time > g_switch_controller.stats.max_critical_time_us) {
        g_switch_controller.stats.max_critical_time_us = critical_time;
    }
    
    /* Update task migration statistics */
    g_switch_controller.stats.total_tasks_migrated += context->migration.migrated_count;
    g_switch_controller.stats.migration_failures += context->migration.failed_count;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Complete the scheduler switch
 */
dsrtos_status_t dsrtos_switch_complete(dsrtos_switch_context_t* context)
{
    if (context == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Phase: Verify switch success */
    context->phase = SWITCH_VERIFYING;
    
    /* Run post-switch verification */
    if (g_switch_controller.validation.post_switch_verify != NULL) {
        if (!g_switch_controller.validation.post_switch_verify()) {
            /* Verification failed - attempt rollback */
            if (context->rollback.rollback_possible) {
                return dsrtos_switch_rollback(context);
            }
            return DSRTOS_VERIFICATION_FAILED;
        }
    }
    
    /* Clear rollback state as switch is successful */
    context->rollback.rollback_possible = false;
    
    /* Mark switch as complete */
    context->phase = SWITCH_COMPLETE;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Rollback failed scheduler switch
 */
dsrtos_status_t dsrtos_switch_rollback(dsrtos_switch_context_t* context)
{
    dsrtos_status_t status;
    dsrtos_kernel_state_t* kernel;
    dsrtos_scheduler_plugin_t* original_scheduler;
    
    if (context == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (!context->rollback.rollback_possible) {
        return DSRTOS_NOT_ALLOWED;
    }
    
    context->phase = SWITCH_ROLLING_BACK;
    g_switch_controller.stats.rollback_count++;
    
    /* Get kernel and original scheduler */
    kernel = dsrtos_kernel_get_state();
    original_scheduler = dsrtos_scheduler_get_by_id(context->request.from_scheduler_id);
    
    if (kernel == NULL || original_scheduler == NULL) {
        return DSRTOS_INVALID_STATE;
    }
    
    /* Enter critical section for rollback */
    enter_critical_switch();
    
    /* Restore original scheduler */
    kernel->active_scheduler = original_scheduler;
    kernel->scheduler_state.active_scheduler_id = context->request.from_scheduler_id;
    
    /* Restore scheduler state if saved */
    if (context->preservation.state_saved) {
        status = restore_scheduler_state(original_scheduler,
                                       context->request.from_scheduler_id,
                                       context->preservation.state_buffer,
                                       context->preservation.used_size);
        if (status != DSRTOS_SUCCESS) {
            exit_critical_switch();
            return status;
        }
    }
    
    /* Restore queue snapshots */
    status = restore_queue_snapshot(&context->rollback.ready_snapshot,
                                   original_scheduler);
    if (status != DSRTOS_SUCCESS) {
        exit_critical_switch();
        return status;
    }
    
    status = restore_queue_snapshot(&context->rollback.blocked_snapshot,
                                   original_scheduler);
    if (status != DSRTOS_SUCCESS) {
        exit_critical_switch();
        return status;
    }
    
    /* Exit critical section */
    exit_critical_switch();
    
    /* Mark rollback complete */
    context->rollback.rollback_possible = false;
    context->phase = SWITCH_IDLE;
    
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * TASK MIGRATION
 * ============================================================================ */

/**
 * @brief Migrate single task between schedulers
 */
dsrtos_status_t dsrtos_migrate_task(dsrtos_tcb_t* task,
                                   uint16_t from_scheduler,
                                   uint16_t to_scheduler,
                                   dsrtos_migration_strategy_t strategy)
{
    dsrtos_scheduler_plugin_t* from_sched;
    dsrtos_scheduler_plugin_t* to_sched;
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Get schedulers */
    from_sched = dsrtos_scheduler_get_by_id(from_scheduler);
    to_sched = dsrtos_scheduler_get_by_id(to_scheduler);
    
    if (from_sched == NULL || to_sched == NULL) {
        return DSRTOS_INVALID_SCHEDULER;
    }
    
    return migrate_single_task(task, from_sched, to_sched, strategy);
}

/**
 * @brief Migrate batch of tasks
 */
dsrtos_status_t dsrtos_migrate_task_batch(dsrtos_tcb_t** tasks,
                                         uint32_t count,
                                         uint16_t from_scheduler,
                                         uint16_t to_scheduler,
                                         dsrtos_migration_strategy_t strategy)
{
    dsrtos_status_t status;
    uint32_t migrated = 0U;
    
    if (tasks == NULL || count == 0U) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Migrate each task */
    for (uint32_t i = 0U; i < count; i++) {
        status = dsrtos_migrate_task(tasks[i], from_scheduler, 
                                    to_scheduler, strategy);
        if (status == DSRTOS_SUCCESS) {
            migrated++;
        } else if (!g_switch_controller.policies.allow_forced) {
            /* Stop on first failure if not forced */
            return status;
        }
    }
    
    return (migrated == count) ? DSRTOS_SUCCESS : DSRTOS_PARTIAL_SUCCESS;
}

/* ============================================================================
 * STATE MANAGEMENT
 * ============================================================================ */

/**
 * @brief Save scheduler state
 */
dsrtos_status_t dsrtos_switch_save_state(dsrtos_switch_context_t* context,
                                        void* scheduler,
                                        uint16_t scheduler_id)
{
    uint32_t size;
    
    if (context == NULL || scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    size = context->preservation.buffer_size;
    return save_scheduler_state(scheduler, scheduler_id,
                               context->preservation.state_buffer, &size);
}

/**
 * @brief Restore scheduler state
 */
dsrtos_status_t dsrtos_switch_restore_state(dsrtos_switch_context_t* context,
                                           void* scheduler,
                                           uint16_t scheduler_id)
{
    if (context == NULL || scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    return restore_scheduler_state(scheduler, scheduler_id,
                                 context->preservation.state_buffer,
                                 context->preservation.used_size);
}

/**
 * @brief Create queue snapshot
 */
dsrtos_status_t dsrtos_switch_snapshot_queues(dsrtos_queue_snapshot_t* snapshot,
                                             void* scheduler)
{
    if (snapshot == NULL || scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Implementation depends on scheduler type */
    /* This is a placeholder - actual implementation would query scheduler */
    return create_queue_snapshot(snapshot, NULL);
}

/* ============================================================================
 * VALIDATION AND SAFETY
 * ============================================================================ */

/**
 * @brief Check if switch is safe
 */
bool dsrtos_switch_is_safe(void)
{
    dsrtos_kernel_state_t* kernel;
    uint32_t critical_count;
    
    /* Check if controller initialized */
    if (!g_switch_controller.initialized) {
        return false;
    }
    
    /* Check if switch already in progress */
    if (g_switch_controller.switch_in_progress) {
        return false;
    }
    
    /* Get kernel state */
    kernel = dsrtos_kernel_get_state();
    if (kernel == NULL) {
        return false;
    }
    
    /* Check critical section count */
    critical_count = dsrtos_critical_get_nesting();
    if (critical_count > 0U) {
        return false;
    }
    
    /* Check if idle required */
    if (g_switch_controller.policies.require_idle) {
        if (kernel->current_task != kernel->idle_task) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Check if switch is allowed
 */
bool dsrtos_switch_is_allowed(uint16_t from_id, uint16_t to_id)
{
    static uint32_t last_switch_time = 0U;
    uint32_t current_time;
    
    /* Check same scheduler */
    if (from_id == to_id) {
        return false;
    }
    
    /* Check if runtime switches allowed */
    if (!g_switch_controller.policies.allow_runtime) {
        return false;
    }
    
    /* Check minimum interval */
    current_time = get_system_time_us() / 1000U;  /* Convert to ms */
    if ((current_time - last_switch_time) < g_switch_controller.policies.min_interval_ms) {
        return false;
    }
    
    /* Update last switch time */
    last_switch_time = current_time;
    
    return true;
}

/**
 * @brief Estimate switch time
 */
uint32_t dsrtos_switch_estimate_time(uint16_t from_id, uint16_t to_id)
{
    uint32_t task_count;
    uint32_t base_time = 50U;  /* Base overhead in microseconds */
    uint32_t per_task_time = 5U;  /* Time per task migration */
    
    /* Unused parameters */
    (void)from_id;
    (void)to_id;
    
    /* Count tasks */
    task_count = dsrtos_task_get_count();
    
    /* Calculate estimate */
    return base_time + (task_count * per_task_time);
}

/**
 * @brief Validate switch request
 */
dsrtos_status_t dsrtos_switch_validate_request(const dsrtos_switch_request_t* request)
{
    if (request == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Validate scheduler IDs */
    if (request->from_scheduler_id == request->to_scheduler_id) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Validate strategy */
    if (request->strategy >= MIGRATE_STRATEGY_COUNT) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Validate deadline */
    if (request->deadline_us == 0U || 
        request->deadline_us > g_switch_controller.policies.max_duration_us) {
        return DSRTOS_INVALID_PARAM;
    }
    
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * POLICY CONFIGURATION
 * ============================================================================ */

/**
 * @brief Set switch policies
 */
dsrtos_status_t dsrtos_switch_set_policy(uint32_t min_interval_ms,
                                        uint32_t max_duration_us,
                                        bool allow_forced)
{
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    if (g_switch_controller.switch_in_progress) {
        return DSRTOS_BUSY;
    }
    
    /* Update policies */
    g_switch_controller.policies.min_interval_ms = min_interval_ms;
    g_switch_controller.policies.max_duration_us = max_duration_us;
    g_switch_controller.policies.allow_forced = allow_forced;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Register validation callback
 */
dsrtos_status_t dsrtos_switch_register_validator(
    bool (*validator)(const dsrtos_switch_request_t*))
{
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    g_switch_controller.validation.pre_switch_check = validator;
    return DSRTOS_SUCCESS;
}

/**
 * @brief Register custom migrator
 */
dsrtos_status_t dsrtos_switch_register_migrator(
    dsrtos_status_t (*migrator)(dsrtos_tcb_t*, void*, void*))
{
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    g_switch_controller.custom_migrate = migrator;
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * STATISTICS AND HISTORY
 * ============================================================================ */

/**
 * @brief Get switch statistics
 */
dsrtos_status_t dsrtos_switch_get_stats(dsrtos_switch_stats_t* stats)
{
    if (stats == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    *stats = g_switch_controller.stats;
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get switch history
 */
dsrtos_status_t dsrtos_switch_get_history(dsrtos_switch_history_t* history,
                                         uint32_t* count)
{
    uint32_t history_count;
    
    if (history == NULL || count == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    /* Determine number of records to copy */
    history_count = (*count < DSRTOS_SWITCH_HISTORY_SIZE) ? 
                    *count : DSRTOS_SWITCH_HISTORY_SIZE;
    
    /* Copy history records */
    (void)memcpy(history, g_switch_controller.history,
                history_count * sizeof(dsrtos_switch_history_t));
    
    *count = history_count;
    return DSRTOS_SUCCESS;
}

/**
 * @brief Clear switch history
 */
dsrtos_status_t dsrtos_switch_clear_history(void)
{
    if (!g_switch_controller.initialized) {
        return DSRTOS_NOT_INITIALIZED;
    }
    
    if (g_switch_controller.switch_in_progress) {
        return DSRTOS_BUSY;
    }
    
    (void)memset(g_switch_controller.history, 0, 
                sizeof(g_switch_controller.history));
    g_switch_controller.history_index = 0U;
    
    return DSRTOS_SUCCESS;
}

/* ============================================================================
 * DEBUGGING AND DIAGNOSTICS
 * ============================================================================ */

/**
 * @brief Get current switch phase
 */
dsrtos_switch_phase_t dsrtos_switch_get_phase(void)
{
    if (g_switch_controller.current_switch != NULL) {
        return g_switch_controller.current_switch->phase;
    }
    return SWITCH_IDLE;
}

/**
 * @brief Get switch phase name
 */
const char* dsrtos_switch_get_phase_name(dsrtos_switch_phase_t phase)
{
    static const char* const phase_names[] = {
        "IDLE",
        "PREPARING",
        "VALIDATING",
        "ENTERING_CRITICAL",
        "SAVING_STATE",
        "MIGRATING_TASKS",
        "ACTIVATING_NEW",
        "EXITING_CRITICAL",
        "VERIFYING",
        "COMPLETE",
        "FAILED",
        "ROLLING_BACK"
    };
    
    if (phase < (sizeof(phase_names) / sizeof(phase_names[0]))) {
        return phase_names[phase];
    }
    return "UNKNOWN";
}

/**
 * @brief Get last error information
 */
dsrtos_status_t dsrtos_switch_get_last_error(uint32_t* error_code,
                                            const char** error_msg)
{
    if (error_code == NULL || error_msg == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (g_switch_controller.current_switch != NULL) {
        *error_code = g_switch_controller.current_switch->error.error_code;
        *error_msg = g_switch_controller.current_switch->error.error_msg;
    } else {
        *error_code = 0U;
        *error_msg = "No error";
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Dump switch context for debugging
 */
void dsrtos_switch_dump_context(const dsrtos_switch_context_t* context)
{
    if (context == NULL) {
        return;
    }
    
    /* This would typically output to debug console */
    /* Implementation depends on debug infrastructure */
    (void)context;  /* Suppress unused warning */
}

/* ============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Calculate checksum for data integrity
 */
static uint32_t calculate_checksum(const void* data, uint32_t size)
{
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = DSRTOS_STATE_CHECKSUM_SEED;
    
    for (uint32_t i = 0U; i < size; i++) {
        checksum = ((checksum << 1U) | (checksum >> 31U)) ^ bytes[i];
    }
    
    return checksum;
}

/**
 * @brief Save scheduler state to buffer
 */
static dsrtos_status_t save_scheduler_state(void* scheduler, uint16_t id,
                                           void* buffer, uint32_t* size)
{
    dsrtos_scheduler_plugin_t* sched = (dsrtos_scheduler_plugin_t*)scheduler;
    
    if (scheduler == NULL || buffer == NULL || size == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Call scheduler's save state operation if available */
    if (sched->operations.save_state != NULL) {
        return sched->operations.save_state(sched, buffer, size);
    }
    
    /* Default: save scheduler context */
    if (*size < sizeof(dsrtos_scheduler_plugin_t)) {
        return DSRTOS_BUFFER_TOO_SMALL;
    }
    
    (void)memcpy(buffer, scheduler, sizeof(dsrtos_scheduler_plugin_t));
    *size = sizeof(dsrtos_scheduler_plugin_t);
    
    /* Unused parameter */
    (void)id;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Restore scheduler state from buffer
 */
static dsrtos_status_t restore_scheduler_state(void* scheduler, uint16_t id,
                                              const void* buffer, uint32_t size)
{
    dsrtos_scheduler_plugin_t* sched = (dsrtos_scheduler_plugin_t*)scheduler;
    
    if (scheduler == NULL || buffer == NULL || size == 0U) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Call scheduler's restore state operation if available */
    if (sched->operations.restore_state != NULL) {
        return sched->operations.restore_state(sched, buffer, size);
    }
    
    /* Default: restore scheduler context */
    if (size < sizeof(dsrtos_scheduler_plugin_t)) {
        return DSRTOS_BUFFER_TOO_SMALL;
    }
    
    (void)memcpy(scheduler, buffer, sizeof(dsrtos_scheduler_plugin_t));
    
    /* Unused parameter */
    (void)id;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate single task between schedulers
 */
static dsrtos_status_t migrate_single_task(dsrtos_tcb_t* task,
                                          void* from_sched,
                                          void* to_sched,
                                          dsrtos_migration_strategy_t strategy)
{
    dsrtos_scheduler_plugin_t* from = (dsrtos_scheduler_plugin_t*)from_sched;
    dsrtos_scheduler_plugin_t* to = (dsrtos_scheduler_plugin_t*)to_sched;
    dsrtos_status_t status;
    
    if (task == NULL || from == NULL || to == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Use custom migrator if registered */
    if (g_switch_controller.custom_migrate != NULL) {
        return g_switch_controller.custom_migrate(task, from_sched, to_sched);
    }
    
    /* Remove from source scheduler */
    if (from->operations.remove_task != NULL) {
        status = from->operations.remove_task(from, task);
        if (status != DSRTOS_SUCCESS) {
            return status;
        }
    }
    
    /* Apply migration strategy */
    switch (strategy) {
        case MIGRATE_PRESERVE_ORDER:
            /* Keep task priority unchanged */
            break;
            
        case MIGRATE_PRIORITY_BASED:
            /* Adjust priority based on new scheduler */
            if (to->operations.adjust_priority != NULL) {
                (void)to->operations.adjust_priority(to, task);
            }
            break;
            
        case MIGRATE_DEADLINE_BASED:
            /* Sort by deadline - requires deadline information */
            /* This would need task deadline field */
            break;
            
        case MIGRATE_CUSTOM:
            /* Custom strategy - handled above */
            break;
            
        default:
            break;
    }
    
    /* Add to target scheduler */
    if (to->operations.add_task != NULL) {
        status = to->operations.add_task(to, task);
        if (status != DSRTOS_SUCCESS) {
            /* Try to restore to original scheduler */
            if (from->operations.add_task != NULL) {
                (void)from->operations.add_task(from, task);
            }
            return status;
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Create queue snapshot for rollback
 */
static dsrtos_status_t create_queue_snapshot(dsrtos_queue_snapshot_t* snapshot,
                                            dsrtos_tcb_t* queue_head)
{
    dsrtos_tcb_t* task;
    uint32_t count = 0U;
    
    if (snapshot == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Initialize snapshot */
    snapshot->tasks = g_snapshot_tasks;
    snapshot->priorities = g_snapshot_priorities;
    snapshot->states = g_snapshot_states;
    snapshot->count = 0U;
    snapshot->timestamp = get_system_time_us();
    
    /* Traverse queue and record tasks */
    task = queue_head;
    while (task != NULL && count < DSRTOS_MAX_TASKS) {
        snapshot->tasks[count] = task;
        snapshot->priorities[count] = task->priority;
        snapshot->states[count] = task->state;
        count++;
        task = task->next;
    }
    
    snapshot->count = count;
    
    /* Calculate checksum */
    snapshot->checksum = calculate_checksum(snapshot->tasks,
                                           count * sizeof(dsrtos_tcb_t*));
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Restore queue from snapshot
 */
static dsrtos_status_t restore_queue_snapshot(const dsrtos_queue_snapshot_t* snapshot,
                                             void* scheduler)
{
    dsrtos_scheduler_plugin_t* sched = (dsrtos_scheduler_plugin_t*)scheduler;
    uint32_t checksum;
    
    if (snapshot == NULL || scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Verify checksum */
    checksum = calculate_checksum(snapshot->tasks,
                                 snapshot->count * sizeof(dsrtos_tcb_t*));
    if (checksum != snapshot->checksum) {
        return DSRTOS_CHECKSUM_ERROR;
    }
    
    /* Clear current queues in scheduler */
    if (sched->operations.clear_queues != NULL) {
        (void)sched->operations.clear_queues(sched);
    }
    
    /* Restore tasks to scheduler */
    for (uint32_t i = 0U; i < snapshot->count; i++) {
        dsrtos_tcb_t* task = snapshot->tasks[i];
        
        /* Restore task state */
        task->priority = snapshot->priorities[i];
        task->state = snapshot->states[i];
        
        /* Add task back to scheduler */
        if (sched->operations.add_task != NULL) {
            (void)sched->operations.add_task(sched, task);
        }
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Record switch in history
 */
static void record_switch_history(uint16_t from, uint16_t to,
                                 dsrtos_switch_reason_t reason,
                                 uint32_t duration, bool success)
{
    dsrtos_switch_history_t* record;
    
    /* Get next history slot */
    record = &g_switch_controller.history[g_switch_controller.history_index];
    
    /* Record switch information */
    record->timestamp = get_system_time_us();
    record->from_id = from;
    record->to_id = to;
    record->reason = reason;
    record->duration_us = duration;
    record->tasks_migrated = g_switch_context.migration.migrated_count;
    record->success = success;
    record->error_code = g_switch_context.error.error_code;
    
    /* Advance history index */
    g_switch_controller.history_index = 
        (g_switch_controller.history_index + 1U) % DSRTOS_SWITCH_HISTORY_SIZE;
}

/**
 * @brief Get system time in microseconds
 */
static uint32_t get_system_time_us(void)
{
    /* This would typically read from a hardware timer */
    /* For now, return tick count converted to microseconds */
    extern uint64_t g_system_tick_count;
    return (uint32_t)(g_system_tick_count * 1000U);
}

/**
 * @brief Enter critical section for switching
 */
static void enter_critical_switch(void)
{
    /* Disable interrupts and enter critical section */
    dsrtos_critical_enter();
    
    /* Additional switch-specific critical setup */
    /* Could include stopping schedulers, freezing timers, etc. */
}

/**
 * @brief Exit critical section after switching
 */
static void exit_critical_switch(void)
{
    /* Switch-specific critical cleanup */
    /* Could include restarting schedulers, timers, etc. */
    
    /* Re-enable interrupts and exit critical section */
    dsrtos_critical_exit();
}
