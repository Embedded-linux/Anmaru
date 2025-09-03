/*
 * @file dsrtos_task_creation.h
 * @brief DSRTOS Task Creation and Lifecycle Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_TASK_CREATION_H
#define DSRTOS_TASK_CREATION_H

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

/* Extended task creation parameters */
typedef struct {
    dsrtos_task_params_t base;          /* Base parameters */
    dsrtos_task_exit_handler_t exit_handler; /* Custom exit handler */
    uint32_t initial_delay;              /* Initial delay before start */
    bool use_pool;                       /* Use static pool allocation */
    void *tls_data;                      /* Thread-local storage */
    uint32_t tls_size;                   /* TLS size */
} dsrtos_task_create_extended_t;

/* Task pool statistics */
typedef struct {
    uint32_t total_created;
    uint32_t total_deleted;
    uint32_t create_failures;
    uint32_t delete_failures;
    uint32_t pool_allocations;
    uint32_t dynamic_allocations;
    uint32_t peak_pool_usage;
    uint32_t stack_overflow_detections;
} task_creation_stats_t;

/*==============================================================================
 * PUBLIC API
 *============================================================================*/

/* Initialization */
dsrtos_error_t dsrtos_task_creation_init(void);

/* Extended creation functions */
dsrtos_tcb_t* dsrtos_task_create_extended(const dsrtos_task_create_extended_t *params);
dsrtos_tcb_t* dsrtos_task_clone(const dsrtos_tcb_t *source_task, const char *new_name);
dsrtos_error_t dsrtos_task_restart(dsrtos_tcb_t *task);

/* Pool management */
dsrtos_error_t dsrtos_task_pool_get_stats(task_creation_stats_t *stats);
dsrtos_error_t dsrtos_task_pool_defragment(void);

/* Statistics */
dsrtos_error_t dsrtos_task_get_creation_stats(task_creation_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_CREATION_H */
