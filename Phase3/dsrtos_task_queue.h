/*
 * @file dsrtos_task_queue.h
 * @brief DSRTOS Task Queue Management Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_TASK_QUEUE_H
#define DSRTOS_TASK_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/* Queue statistics */
typedef struct {
    uint32_t ready_count;
    uint32_t blocked_count;
    uint32_t suspended_count;
    uint32_t delayed_count;
    uint32_t total_enqueues;
    uint32_t total_dequeues;
    uint32_t max_ready_count;
    uint32_t max_blocked_count;
} dsrtos_queue_stats_t;

/*==============================================================================
 * PUBLIC API
 *============================================================================*/

/* Initialization */
dsrtos_error_t dsrtos_queue_init(void);

/* Ready queue operations */
dsrtos_error_t dsrtos_queue_ready_insert(dsrtos_tcb_t *tcb);
dsrtos_error_t dsrtos_queue_ready_remove(dsrtos_tcb_t *tcb);
dsrtos_tcb_t* dsrtos_queue_ready_get_highest(void);
dsrtos_error_t dsrtos_queue_ready_reorder(dsrtos_tcb_t *tcb);

/* Blocked queue operations */
dsrtos_error_t dsrtos_queue_blocked_insert(dsrtos_tcb_t *tcb, uint32_t timeout);
dsrtos_error_t dsrtos_queue_blocked_remove(dsrtos_tcb_t *tcb);

/* Suspended queue operations */
dsrtos_error_t dsrtos_queue_suspended_insert(dsrtos_tcb_t *tcb);
dsrtos_error_t dsrtos_queue_suspended_remove(dsrtos_tcb_t *tcb);

/* Delayed queue operations */
uint32_t dsrtos_queue_process_delayed(void);

/* Statistics */
dsrtos_error_t dsrtos_queue_get_stats(dsrtos_queue_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_QUEUE_H */
