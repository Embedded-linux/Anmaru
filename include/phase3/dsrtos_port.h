/*
 * @file dsrtos_port.h
 * @brief DSRTOS Hardware Port Interface for Phase 3
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_PORT_H
#define DSRTOS_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dsrtos_types.h"

/*==============================================================================
 * PORT INTERFACE FUNCTIONS
 *============================================================================*/

/* Stack initialization */
void* dsrtos_port_init_stack(void *stack_top,
                             void (*entry)(void *),
                             void *param,
                             void (*exit)(void));

/* Scheduler control */
void dsrtos_port_start_scheduler(void);
void dsrtos_port_yield(void);

/* Idle processing */
void dsrtos_port_idle(void);

/* Task exit */
void dsrtos_task_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_PORT_H */
