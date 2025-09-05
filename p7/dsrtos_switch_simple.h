/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Simplified Dynamic Scheduler Switching Header
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_SWITCH_SIMPLE_H
#define DSRTOS_SWITCH_SIMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Switch controller structure */
typedef struct {
    dsrtos_scheduler_type_t current_scheduler;  /* Current scheduler type */
    bool switch_in_progress;                    /* Switch in progress flag */
    uint32_t switch_count;                      /* Number of switches performed */
} dsrtos_switch_controller_t;

/* Function prototypes */
dsrtos_status_t dsrtos_switch_controller_init(dsrtos_switch_controller_t* controller);
dsrtos_status_t dsrtos_switch_scheduler(dsrtos_switch_controller_t* controller, 
                                       dsrtos_scheduler_type_t new_scheduler);
dsrtos_scheduler_type_t dsrtos_switch_get_current_scheduler(const dsrtos_switch_controller_t* controller);
dsrtos_switch_controller_t* dsrtos_switch_controller_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SWITCH_SIMPLE_H */

