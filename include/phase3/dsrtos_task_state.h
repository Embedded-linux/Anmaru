/*
 * @file dsrtos_task_state.h
 * @brief DSRTOS Task State Management Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_TASK_STATE_H
#define DSRTOS_TASK_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_task_manager.h"

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/* State change hook prototype */
typedef void (*dsrtos_state_change_hook_t)(dsrtos_tcb_t *tcb,
                                           dsrtos_task_state_t old_state,
                                           dsrtos_task_state_t new_state);

/* State statistics */
typedef struct {
    uint32_t total_transitions;
    uint32_t invalid_transitions;
    uint32_t transition_time_max;
} dsrtos_state_stats_t;

/*==============================================================================
 * PUBLIC API
 *============================================================================*/

/* Initialization */
dsrtos_error_t dsrtos_state_init(void);

/* State management */
dsrtos_error_t dsrtos_state_transition(dsrtos_tcb_t *tcb,
                                       dsrtos_task_state_t new_state);
dsrtos_error_t dsrtos_state_get(const dsrtos_tcb_t *tcb,
                                dsrtos_task_state_t *state);

/* State queries */
const char* dsrtos_state_get_name(dsrtos_task_state_t state);
bool dsrtos_state_is_runnable(const dsrtos_tcb_t *tcb);
bool dsrtos_state_is_blocked(const dsrtos_tcb_t *tcb);

/* Hooks and callbacks */
dsrtos_error_t dsrtos_state_register_hook(dsrtos_state_change_hook_t hook);

/* Statistics */
dsrtos_error_t dsrtos_state_get_stats(dsrtos_state_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_STATE_H */
