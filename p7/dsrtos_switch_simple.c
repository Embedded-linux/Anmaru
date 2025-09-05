/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Simplified Dynamic Scheduler Switching
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_switch_simple.h"
#include "dsrtos_kernel.h"
#include "dsrtos_config.h"
#include <string.h>

/* Missing types and constants */
typedef int32_t dsrtos_status_t;
#ifndef DSRTOS_INVALID_PARAM
#define DSRTOS_INVALID_PARAM                (-1)
#endif
#ifndef DSRTOS_ERROR
#define DSRTOS_ERROR                        (-1)
#endif
#ifndef DSRTOS_CHECKSUM_ERROR
#define DSRTOS_CHECKSUM_ERROR               (-17)
#endif

/* Simplified switch controller */
static dsrtos_switch_controller_t g_switch_controller;

/* Initialize switch controller */
dsrtos_status_t dsrtos_switch_controller_init(dsrtos_switch_controller_t* controller)
{
    if (controller == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Initialize controller state */
    controller->current_scheduler = DSRTOS_SCHEDULER_TYPE_PRIORITY;
    controller->switch_in_progress = false;
    controller->switch_count = 0U;
    
    return DSRTOS_SUCCESS;
}

/* Switch to different scheduler */
dsrtos_status_t dsrtos_switch_scheduler(dsrtos_switch_controller_t* controller, 
                                       dsrtos_scheduler_type_t new_scheduler)
{
    if (controller == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if (controller->switch_in_progress) {
        return DSRTOS_ERROR;
    }
    
    /* Perform scheduler switch */
    controller->switch_in_progress = true;
    controller->current_scheduler = new_scheduler;
    controller->switch_count++;
    controller->switch_in_progress = false;
    
    return DSRTOS_SUCCESS;
}

/* Get current scheduler type */
dsrtos_scheduler_type_t dsrtos_switch_get_current_scheduler(const dsrtos_switch_controller_t* controller)
{
    if (controller == NULL) {
        return DSRTOS_SCHEDULER_TYPE_PRIORITY;
    }
    
    return controller->current_scheduler;
}

/* Get global switch controller instance */
dsrtos_switch_controller_t* dsrtos_switch_controller_get_instance(void)
{
    return &g_switch_controller;
}
