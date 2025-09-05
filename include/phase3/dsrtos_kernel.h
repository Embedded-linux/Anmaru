/*
 * @file dsrtos_kernel.h
 * @brief DSRTOS Kernel Interface for Phase 3
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_KERNEL_H
#define DSRTOS_KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"

/*==============================================================================
 * KERNEL INTERFACE FUNCTIONS
 *============================================================================*/

/* Time management */
uint64_t dsrtos_get_system_time(void);
uint32_t dsrtos_get_tick_count(void);

/* Memory management stubs for Phase 3 */
void* dsrtos_malloc(size_t size);
void dsrtos_free(void *ptr);

/* Scheduler interface stub */
dsrtos_error_t dsrtos_scheduler_yield(void);

/* Task helper */
dsrtos_tcb_t* dsrtos_task_get_by_index(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_KERNEL_H */
