/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Simplified Task Migration Header
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_MIGRATION_SIMPLE_H
#define DSRTOS_MIGRATION_SIMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_scheduler.h"
#include "dsrtos_task_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Migration controller structure */
typedef struct {
    bool migration_enabled;                    /* Migration enabled flag */
    uint32_t migration_count;                  /* Total migrations performed */
    uint32_t active_migrations;                /* Currently active migrations */
} dsrtos_migration_controller_t;

/* Function prototypes */
dsrtos_status_t dsrtos_migration_controller_init(dsrtos_migration_controller_t* controller);
dsrtos_status_t dsrtos_migration_enable(dsrtos_migration_controller_t* controller);
dsrtos_status_t dsrtos_migration_disable(dsrtos_migration_controller_t* controller);
dsrtos_status_t dsrtos_migrate_task(dsrtos_migration_controller_t* controller,
                                   dsrtos_tcb_t* task,
                                   dsrtos_scheduler_type_t target_scheduler);
dsrtos_migration_controller_t* dsrtos_migration_controller_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_MIGRATION_SIMPLE_H */

