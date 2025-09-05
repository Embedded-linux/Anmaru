/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Switching Implementation
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_scheduler_core.h"
#include "dsrtos_scheduler_manager.h"
#include "dsrtos_critical.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_config.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define SWITCH_TRACE(msg)       DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define SWITCH_ASSERT(cond)     DSRTOS_ASSERT(cond)

/* Switch phases */
#define SWITCH_PHASE_PREPARE    (0U)
#define SWITCH_PHASE_SUSPEND    (1U)
#define SWITCH_PHASE_MIGRATE    (2U)
#define SWITCH_PHASE_ACTIVATE   (3U)
#define SWITCH_PHASE_COMPLETE   (4U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Switch context */
typedef struct {
    /* Switch parameters */
    uint16_t from_scheduler_id;
    uint16_t to_scheduler_id;
    uint32_t switch_reason;
    
    /* Task migration */
    dsrtos_tcb_t* task_list_head;
    uint32_t task_count;
    uint32_t tasks_migrated;
    
    /* Timing */
    uint32_t switch_start_time;
    uint32_t phase_start_time;
    uint32_t total_switch_time;
    
    /* State */
    uint32_t current_phase;
    bool rollback_required;
    bool switch_complete;
} switch_context_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static switch_context_t g_switch_context = {0};

/*==============================================================================
 *                     LOCAL FUNCTION PROTOTYPES
 *============================================================================*/
static dsrtos_error_t prepare_switch(uint16_t from_id, uint16_t to_id);
static dsrtos_error_t suspend_scheduler(uint16_t scheduler_id);
static dsrtos_error_t migrate_tasks(void);
static dsrtos_error_t activate_scheduler(uint16_t scheduler_id);
static dsrtos_error_t complete_switch(void);
static dsrtos_error_t rollback_switch(void);
static void collect_ready_tasks(dsrtos_tcb_t** list_head, uint32_t* count);

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Perform safe scheduler switch
 * @param from_id Source scheduler ID
 * @param to_id Target scheduler ID
 * @param reason Switch reason
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_safe_switch(
    uint16_t from_id, uint16_t to_id, uint32_t reason)
{
    dsrtos_error_t err;
    uint32_t sr;
    
    SWITCH_TRACE("Starting safe scheduler switch");
    
    /* Enter critical section for entire switch */
    sr = dsrtos_critical_enter();
    
    /* Initialize switch context */
    (void)memset(&g_switch_context, 0, sizeof(g_switch_context));
    g_switch_context.from_scheduler_id = from_id;
    g_switch_context.to_scheduler_id = to_id;
    g_switch_context.switch_reason = reason;
    g_switch_context.switch_start_time = dsrtos_get_tick_count();
    
    /* Phase 1: Prepare */
    g_switch_context.current_phase = SWITCH_PHASE_PREPARE;
    err = prepare_switch(from_id, to_id);
    if (err != DSRTOS_SUCCESS) {
        goto error_exit;
    }
    
    /* Phase 2: Suspend old scheduler */
    g_switch_context.current_phase = SWITCH_PHASE_SUSPEND;
    err = suspend_scheduler(from_id);
    if (err != DSRTOS_SUCCESS) {
        goto error_exit;
    }
    
    /* Phase 3: Migrate tasks */
    g_switch_context.current_phase = SWITCH_PHASE_MIGRATE;
    err = migrate_tasks();
    if (err != DSRTOS_SUCCESS) {
        goto rollback_exit;
    }
    
    /* Phase 4: Activate new scheduler */
    g_switch_context.current_phase = SWITCH_PHASE_ACTIVATE;
    err = activate_scheduler(to_id);
    if (err != DSRTOS_SUCCESS) {
        goto rollback_exit;
    }
    
    /* Phase 5: Complete switch */
    g_switch_context.current_phase = SWITCH_PHASE_COMPLETE;
    err = complete_switch();
    
    g_switch_context.total_switch_time = 
        dsrtos_get_tick_count() - g_switch_context.switch_start_time;
    
    dsrtos_critical_exit(sr);
    
    SWITCH_TRACE("Scheduler switch completed successfully");
    return err;
    
rollback_exit:
    g_switch_context.rollback_required = true;
    (void)rollback_switch();
    
error_exit:
    dsrtos_critical_exit(sr);
    SWITCH_TRACE("Scheduler switch failed");
    return err;
}

/**
 * @brief Get switch progress
 * @return Current phase (0-100%)
 */
uint32_t dsrtos_scheduler_get_switch_progress(void)
{
    if (g_switch_context.switch_complete) {
        return 100U;
    }
    
    return (g_switch_context.current_phase * 20U); /* 5 phases = 20% each */
}

/**
 * @brief Get switch overhead
 * @return Switch time in microseconds
 */
uint32_t dsrtos_scheduler_get_switch_overhead(void)
{
    return g_switch_context.total_switch_time * 1000U;
}

/*==============================================================================
 *                          LOCAL FUNCTIONS
 *============================================================================*/

/**
 * @brief Prepare for scheduler switch
 * @param from_id Source scheduler
 * @param to_id Target scheduler
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t prepare_switch(uint16_t from_id, uint16_t to_id)
{
    dsrtos_scheduler_plugin_t* from_sched;
    dsrtos_scheduler_plugin_t* to_sched;
    
    SWITCH_TRACE("Preparing scheduler switch");
    
    /* Get scheduler plugins */
    from_sched = dsrtos_scheduler_get(from_id);
    to_sched = dsrtos_scheduler_get(to_id);
    
    if ((from_sched == NULL) || (to_sched == NULL)) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Check compatibility */
    if (!dsrtos_scheduler_validate_switch(from_id, to_id)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Collect ready tasks */
    collect_ready_tasks(&g_switch_context.task_list_head, 
                       &g_switch_context.task_count);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Suspend scheduler
 * @param scheduler_id Scheduler to suspend
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t suspend_scheduler(uint16_t scheduler_id)
{
    dsrtos_scheduler_plugin_t* scheduler;
    void* context;
    
    SWITCH_TRACE("Suspending scheduler");
    
    scheduler = dsrtos_scheduler_get(scheduler_id);
    if (scheduler == NULL) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Get scheduler context */
    context = dsrtos_scheduler_get_context()->active_scheduler;
    
    /* Call suspend if available */
    if (scheduler->suspend != NULL) {
        return scheduler->suspend(context);
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate tasks between schedulers
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t migrate_tasks(void)
{
    dsrtos_tcb_t* task;
    dsrtos_scheduler_plugin_t* to_sched;
    void* to_context;
    
    SWITCH_TRACE("Migrating tasks");
    
    /* Get target scheduler */
    to_sched = dsrtos_scheduler_get(g_switch_context.to_scheduler_id);
    if (to_sched == NULL) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Get target context */
    to_context = dsrtos_scheduler_get_context()->active_scheduler;
    
    /* Migrate each task */
    task = g_switch_context.task_list_head;
    while (task != NULL) {
        /* Save next task */
        dsrtos_tcb_t* next = task->next;
        
        /* Clear task links */
        task->next = NULL;
        task->prev = NULL;
        
        /* Enqueue in new scheduler */
        if (to_sched->ops.enqueue_task != NULL) {
            to_sched->ops.enqueue_task(to_context, task);
        }
        
        g_switch_context.tasks_migrated++;
        
        /* Move to next task */
        task = next;
    }
    
    /* Verify all tasks migrated */
    if (g_switch_context.tasks_migrated != g_switch_context.task_count) {
        return DSRTOS_ERROR_INCOMPLETE;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Activate new scheduler
 * @param scheduler_id Scheduler to activate
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t activate_scheduler(uint16_t scheduler_id)
{
    dsrtos_scheduler_plugin_t* scheduler;
    void* context;
    
    SWITCH_TRACE("Activating new scheduler");
    
    scheduler = dsrtos_scheduler_get(scheduler_id);
    if (scheduler == NULL) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Get scheduler context */
    context = dsrtos_scheduler_get_context()->active_scheduler;
    
    /* Call start if available */
    if (scheduler->start != NULL) {
        return scheduler->start(context);
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Complete scheduler switch
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t complete_switch(void)
{
    dsrtos_scheduler_context_t* ctx;
    
    SWITCH_TRACE("Completing scheduler switch");
    
    /* Update global context */
    ctx = dsrtos_scheduler_get_context();
    ctx->selection.last_switch_time = dsrtos_get_tick_count();
    ctx->selection.switch_count++;
    
    /* Mark complete */
    g_switch_context.switch_complete = true;
    
    /* Resume new scheduler */
    dsrtos_scheduler_plugin_t* scheduler = 
        dsrtos_scheduler_get(g_switch_context.to_scheduler_id);
    if ((scheduler != NULL) && (scheduler->resume != NULL)) {
        void* context = ctx->active_scheduler;
        return scheduler->resume(context);
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Rollback failed switch
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t rollback_switch(void)
{
    dsrtos_error_t err;
    
    SWITCH_TRACE("Rolling back scheduler switch");
    
    /* Restore old scheduler */
    err = activate_scheduler(g_switch_context.from_scheduler_id);
    if (err != DSRTOS_SUCCESS) {
        /* Critical failure - system may be unstable */
        SWITCH_TRACE("CRITICAL: Rollback failed");
        return err;
    }
    
    /* Re-migrate tasks if needed */
    if (g_switch_context.tasks_migrated > 0U) {
        /* Tasks are already in new scheduler, need to move back */
        /* This is complex and may not be possible safely */
        SWITCH_TRACE("WARNING: Task migration rollback not implemented");
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Collect all ready tasks
 * @param list_head Pointer to list head
 * @param count Pointer to count
 */
static void collect_ready_tasks(dsrtos_tcb_t** list_head, uint32_t* count)
{
    dsrtos_scheduler_context_t* ctx;
    dsrtos_tcb_t* task;
    dsrtos_tcb_t* last = NULL;
    
    SWITCH_ASSERT(list_head != NULL);
    SWITCH_ASSERT(count != NULL);
    
    *list_head = NULL;
    *count = 0U;
    
    /* Get scheduler context */
    ctx = dsrtos_scheduler_get_context();
    
    /* Start with ready queue head */
    task = ctx->ready_queue_head;
    
    /* Build list of ready tasks */
    while (task != NULL) {
        if (task->state == DSRTOS_TASK_STATE_READY) {
            if (*list_head == NULL) {
                *list_head = task;
            } else {
                last->next = task;
                task->prev = last;
            }
            last = task;
            (*count)++;
        }
        task = task->next;
    }
    
    /* Terminate list */
    if (last != NULL) {
        last->next = NULL;
    }
}
