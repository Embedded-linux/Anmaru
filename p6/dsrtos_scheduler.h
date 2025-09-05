/*
 * @file dsrtos_scheduler.h
 * @brief DSRTOS Scheduler Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_SCHEDULER_H
#define DSRTOS_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"
#include "dsrtos_task_manager.h"

/* Missing type definition */
typedef int32_t dsrtos_status_t;

/*==============================================================================
 * SCHEDULER TYPES
 *============================================================================*/

/* Scheduler types are defined in dsrtos_task_manager.h */

/* Scheduler state */
typedef enum {
    DSRTOS_SCHEDULER_STATE_STOPPED = 0U,
    DSRTOS_SCHEDULER_STATE_RUNNING = 1U,
    DSRTOS_SCHEDULER_STATE_SUSPENDED = 2U,
    DSRTOS_SCHEDULER_STATE_MAX = 3U
} dsrtos_scheduler_state_t;

/* Base scheduler structure */
typedef struct {
    uint32_t magic;                    /* Magic number for validation */
    dsrtos_scheduler_type_t type;      /* Scheduler type */
    dsrtos_scheduler_state_t state;    /* Current state */
    struct {
        uint32_t errors;               /* Error count */
        uint32_t operations;           /* Operation count */
    } stats;                           /* Statistics */
} dsrtos_scheduler_base_t;

/* Scheduler configuration is defined in dsrtos_task_manager.h */

/* Scheduler statistics */
typedef struct {
    uint32_t context_switches;
    uint32_t preemptions;
    uint32_t voluntary_yields;
    uint32_t scheduler_calls;
    uint32_t idle_time;
} dsrtos_scheduler_stats_t;

/*==============================================================================
 * SCHEDULER INTERFACE
 *============================================================================*/

/* Initialize scheduler */
dsrtos_error_t dsrtos_scheduler_init(const dsrtos_scheduler_config_t *config);

/* Start scheduler */
dsrtos_error_t dsrtos_scheduler_start(void);

/* Stop scheduler */
dsrtos_error_t dsrtos_scheduler_stop(void);

/* Suspend scheduler */
dsrtos_error_t dsrtos_scheduler_suspend(void);

/* Resume scheduler */
dsrtos_error_t dsrtos_scheduler_resume(void);

/* Get current scheduler type */
dsrtos_scheduler_type_t dsrtos_scheduler_get_type(void);

/* Get scheduler state */
dsrtos_scheduler_state_t dsrtos_scheduler_get_state(void);

/* Get scheduler statistics */
dsrtos_error_t dsrtos_scheduler_get_stats(dsrtos_scheduler_stats_t *stats);

/* Reset scheduler statistics */
dsrtos_error_t dsrtos_scheduler_reset_stats(void);

/* Configure scheduler */
dsrtos_error_t dsrtos_scheduler_configure(const dsrtos_scheduler_config_t *config);

/* Switch to next task */
dsrtos_error_t dsrtos_scheduler_schedule(void);

/* Yield current task */
dsrtos_error_t dsrtos_scheduler_yield(void);

/* Add task to ready queue */
dsrtos_error_t dsrtos_scheduler_add_task(dsrtos_tcb_t *task);

/* Remove task from ready queue */
dsrtos_error_t dsrtos_scheduler_remove_task(dsrtos_tcb_t *task);

/* Get next task to run */
dsrtos_tcb_t* dsrtos_scheduler_get_next_task(void);

/* Check if preemption is needed */
bool dsrtos_scheduler_needs_preemption(dsrtos_tcb_t *current_task, dsrtos_tcb_t *new_task);

/* Perform context switch */
dsrtos_error_t dsrtos_scheduler_context_switch(dsrtos_tcb_t *from, dsrtos_tcb_t *to);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_H */



