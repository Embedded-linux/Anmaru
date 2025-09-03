/*
 * @file dsrtos_stack_manager.h
 * @brief DSRTOS Stack Management Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_STACK_MANAGER_H
#define DSRTOS_STACK_MANAGER_H

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

/* Stack overflow hook prototype */
typedef void (*dsrtos_stack_overflow_hook_t)(dsrtos_tcb_t *task);

/* Stack statistics */
typedef struct {
    uint32_t total_stacks;
    uint32_t total_stack_memory;
    uint32_t overflow_detections;
    uint32_t underflow_detections;
    uint32_t guard_violations;
    uint32_t watermark_checks;
    uint32_t peak_total_usage;
} stack_manager_stats_t;

/*==============================================================================
 * PUBLIC API
 *============================================================================*/

/* Initialization */
dsrtos_error_t dsrtos_stack_manager_init(void);

/* Stack operations */
dsrtos_error_t dsrtos_stack_init(dsrtos_tcb_t *tcb,
                                 void *stack_base,
                                 uint32_t stack_size,
                                 dsrtos_task_entry_t entry_point,
                                 void *param);

/* Stack checking */
dsrtos_error_t dsrtos_stack_check(const dsrtos_tcb_t *tcb);
dsrtos_error_t dsrtos_stack_check_all(void);

/* Stack usage */
dsrtos_error_t dsrtos_stack_get_usage(const dsrtos_tcb_t *tcb,
                                      uint32_t *used,
                                      uint32_t *free);
dsrtos_error_t dsrtos_stack_get_watermark(const dsrtos_tcb_t *tcb,
                                          uint32_t *watermark);

/* Hooks and callbacks */
dsrtos_error_t dsrtos_stack_register_overflow_hook(dsrtos_stack_overflow_hook_t hook);

/* Statistics */
dsrtos_error_t dsrtos_stack_get_stats(stack_manager_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_STACK_MANAGER_H */
