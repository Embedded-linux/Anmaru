/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Core - Header File
 * 
 * Core scheduler infrastructure and interfaces
 * 
 * Copyright (C) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_SCHEDULER_CORE_H
#define DSRTOS_SCHEDULER_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SCHEDULER CORE CONFIGURATION
 * ============================================================================ */

/* Scheduler types */
#define DSRTOS_SCHEDULER_TYPE_RR           (0U)    /* Round Robin */
#define DSRTOS_SCHEDULER_TYPE_PRIORITY     (1U)    /* Priority-based */
#define DSRTOS_SCHEDULER_TYPE_EDF          (2U)    /* Earliest Deadline First */
#define DSRTOS_SCHEDULER_TYPE_RMS          (3U)    /* Rate Monotonic */

/* Scheduler states */
#define DSRTOS_SCHEDULER_STATE_INACTIVE    (0U)
#define DSRTOS_SCHEDULER_STATE_ACTIVE      (1U)
#define DSRTOS_SCHEDULER_STATE_SUSPENDED   (2U)
#define DSRTOS_SCHEDULER_STATE_ERROR       (3U)

/* Core limits */
#define DSRTOS_MAX_SCHEDULERS              (8U)
#define DSRTOS_MAX_TASKS_PER_SCHEDULER     (64U)
#define DSRTOS_SCHEDULER_NAME_LEN          (32U)

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Forward declarations */
typedef struct dsrtos_task dsrtos_task_t;
typedef struct dsrtos_scheduler dsrtos_scheduler_t;

/* Scheduler interface function pointers */
typedef dsrtos_error_t (*dsrtos_scheduler_init_fn_t)(dsrtos_scheduler_t* scheduler);
typedef dsrtos_error_t (*dsrtos_scheduler_deinit_fn_t)(dsrtos_scheduler_t* scheduler);
typedef dsrtos_task_t* (*dsrtos_scheduler_next_fn_t)(dsrtos_scheduler_t* scheduler);
typedef dsrtos_error_t (*dsrtos_scheduler_add_fn_t)(dsrtos_scheduler_t* scheduler, dsrtos_task_t* task);
typedef dsrtos_error_t (*dsrtos_scheduler_remove_fn_t)(dsrtos_scheduler_t* scheduler, dsrtos_task_t* task);
typedef dsrtos_error_t (*dsrtos_scheduler_yield_fn_t)(dsrtos_scheduler_t* scheduler, dsrtos_task_t* task);
typedef uint32_t (*dsrtos_scheduler_count_fn_t)(const dsrtos_scheduler_t* scheduler);
typedef bool (*dsrtos_scheduler_is_empty_fn_t)(const dsrtos_scheduler_t* scheduler);

/* Scheduler interface structure */
typedef struct {
    dsrtos_scheduler_init_fn_t     init;       /* Initialize scheduler */
    dsrtos_scheduler_deinit_fn_t   deinit;     /* Deinitialize scheduler */
    dsrtos_scheduler_next_fn_t     next;       /* Get next task to run */
    dsrtos_scheduler_add_fn_t      add;        /* Add task to scheduler */
    dsrtos_scheduler_remove_fn_t   remove;     /* Remove task from scheduler */
    dsrtos_scheduler_yield_fn_t    yield;      /* Yield current task */
    dsrtos_scheduler_count_fn_t    count;      /* Get task count */
    dsrtos_scheduler_is_empty_fn_t is_empty;   /* Check if scheduler is empty */
} dsrtos_scheduler_interface_t;

/* Scheduler structure */
typedef struct dsrtos_scheduler {
    uint32_t                       id;         /* Scheduler ID */
    uint32_t                       type;       /* Scheduler type */
    uint32_t                       state;      /* Current state */
    char                          name[DSRTOS_SCHEDULER_NAME_LEN]; /* Name */
    dsrtos_scheduler_interface_t   interface;  /* Function interface */
    void*                         data;        /* Scheduler-specific data */
    uint32_t                       task_count; /* Number of tasks */
    uint32_t                       magic;      /* Magic number for validation */
} dsrtos_scheduler_t;

/* Scheduler manager structure */
typedef struct {
    dsrtos_scheduler_t*           schedulers[DSRTOS_MAX_SCHEDULERS];
    uint32_t                       active_scheduler_id;
    uint32_t                       scheduler_count;
    bool                           initialized;
    uint32_t                       magic;      /* Magic number for validation */
} dsrtos_scheduler_manager_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

extern dsrtos_scheduler_manager_t g_scheduler_manager;

/* ============================================================================
 * CORE SCHEDULER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize the scheduler core system
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_core_init(void);

/**
 * @brief Deinitialize the scheduler core system
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_core_deinit(void);

/**
 * @brief Register a scheduler with the core system
 * @param scheduler Pointer to scheduler structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_register(dsrtos_scheduler_t* scheduler);

/**
 * @brief Unregister a scheduler from the core system
 * @param scheduler_id ID of scheduler to unregister
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_unregister(uint32_t scheduler_id);

/**
 * @brief Set the active scheduler
 * @param scheduler_id ID of scheduler to activate
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_set_active(uint32_t scheduler_id);

/**
 * @brief Get the active scheduler
 * @return Pointer to active scheduler, NULL if none
 */
dsrtos_scheduler_t* dsrtos_scheduler_get_active(void);

/**
 * @brief Get scheduler by ID
 * @param scheduler_id ID of scheduler to get
 * @return Pointer to scheduler, NULL if not found
 */
dsrtos_scheduler_t* dsrtos_scheduler_get_by_id(uint32_t scheduler_id);

/**
 * @brief Get next task to run from active scheduler
 * @return Pointer to next task, NULL if none
 */
dsrtos_task_t* dsrtos_scheduler_get_next_task(void);

/**
 * @brief Add task to active scheduler
 * @param task Pointer to task to add
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_add_task(dsrtos_task_t* task);

/**
 * @brief Remove task from active scheduler
 * @param task Pointer to task to remove
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_remove_task(dsrtos_task_t* task);

/**
 * @brief Yield current task in active scheduler
 * @param task Pointer to task yielding
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_yield_task(dsrtos_task_t* task);

/**
 * @brief Get number of tasks in active scheduler
 * @return Number of tasks
 */
uint32_t dsrtos_scheduler_get_task_count(void);

/**
 * @brief Check if active scheduler is empty
 * @return true if empty, false otherwise
 */
bool dsrtos_scheduler_is_empty(void);

/* ============================================================================
 * SCHEDULER VALIDATION FUNCTIONS
 * ============================================================================ */

/**
 * @brief Validate scheduler structure
 * @param scheduler Pointer to scheduler to validate
 * @return true if valid, false otherwise
 */
bool dsrtos_scheduler_validate(const dsrtos_scheduler_t* scheduler);

/**
 * @brief Validate scheduler manager
 * @return true if valid, false otherwise
 */
bool dsrtos_scheduler_manager_validate(void);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Get scheduler type name
 * @param type Scheduler type
 * @return String name of scheduler type
 */
const char* dsrtos_scheduler_get_type_name(uint32_t type);

/**
 * @brief Get scheduler state name
 * @param state Scheduler state
 * @return String name of scheduler state
 */
const char* dsrtos_scheduler_get_state_name(uint32_t state);

/**
 * @brief Get scheduler statistics
 * @param scheduler_id ID of scheduler
 * @param stats Pointer to statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_scheduler_get_stats(uint32_t scheduler_id, void* stats);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_CORE_H */





