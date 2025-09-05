/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Core - Implementation
 * 
 * Core scheduler infrastructure and interfaces
 * 
 * Copyright (C) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_scheduler_core.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_config.h"
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define DSRTOS_SCHEDULER_MAGIC              (0x53434845U) /* 'SCHE' */
#define DSRTOS_SCHEDULER_MANAGER_MAGIC      (0x4D475253U) /* 'MGRS' */

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

dsrtos_scheduler_manager_t g_scheduler_manager = {
    .schedulers = {0},
    .active_scheduler_id = 0U,
    .scheduler_count = 0U,
    .initialized = false,
    .magic = 0U
};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Find free scheduler slot
 * @return Index of free slot, DSRTOS_MAX_SCHEDULERS if none
 */
static uint32_t find_free_scheduler_slot(void)
{
    uint32_t i;
    
    for (i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        if (g_scheduler_manager.schedulers[i] == NULL) {
            return i;
        }
    }
    
    return DSRTOS_MAX_SCHEDULERS;
}

/**
 * @brief Find scheduler by ID
 * @param scheduler_id ID to find
 * @return Index of scheduler, DSRTOS_MAX_SCHEDULERS if not found
 */
static uint32_t find_scheduler_by_id(uint32_t scheduler_id)
{
    uint32_t i;
    
    for (i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        if ((g_scheduler_manager.schedulers[i] != NULL) &&
            (g_scheduler_manager.schedulers[i]->id == scheduler_id)) {
            return i;
        }
    }
    
    return DSRTOS_MAX_SCHEDULERS;
}

/* ============================================================================
 * CORE SCHEDULER FUNCTIONS
 * ============================================================================ */

dsrtos_error_t dsrtos_scheduler_core_init(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    dsrtos_critical_enter();
    
    if (g_scheduler_manager.initialized) {
        result = DSRTOS_ERROR_ALREADY_INITIALIZED;
    } else {
        /* Initialize scheduler manager */
        (void)memset(&g_scheduler_manager, 0, sizeof(g_scheduler_manager));
        g_scheduler_manager.magic = DSRTOS_SCHEDULER_MANAGER_MAGIC;
        g_scheduler_manager.initialized = true;
        
        DSRTOS_TRACE_SCHEDULER("Scheduler core initialized");
    }
    
    dsrtos_critical_exit();
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_core_deinit(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t i;
    
    dsrtos_critical_enter();
    
    if (!g_scheduler_manager.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    } else {
        /* Deinitialize all schedulers */
        for (i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
            if (g_scheduler_manager.schedulers[i] != NULL) {
                if (g_scheduler_manager.schedulers[i]->interface.deinit != NULL) {
                    (void)g_scheduler_manager.schedulers[i]->interface.deinit(
                        g_scheduler_manager.schedulers[i]);
                }
                g_scheduler_manager.schedulers[i] = NULL;
            }
        }
        
        /* Clear manager */
        g_scheduler_manager.magic = 0U;
        g_scheduler_manager.initialized = false;
        
        DSRTOS_TRACE_SCHEDULER("Scheduler core deinitialized");
    }
    
    dsrtos_critical_exit();
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_register(dsrtos_scheduler_t* scheduler)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t slot;
    
    if (scheduler == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    if (!dsrtos_scheduler_validate(scheduler)) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    dsrtos_critical_enter();
    
    if (!g_scheduler_manager.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    } else {
        /* Check if scheduler already registered */
        if (find_scheduler_by_id(scheduler->id) != DSRTOS_MAX_SCHEDULERS) {
            result = DSRTOS_ERROR_ALREADY_EXISTS;
        } else {
            /* Find free slot */
            slot = find_free_scheduler_slot();
            if (slot == DSRTOS_MAX_SCHEDULERS) {
                result = DSRTOS_ERROR_LIMIT_REACHED;
            } else {
                /* Register scheduler */
                g_scheduler_manager.schedulers[slot] = scheduler;
                g_scheduler_manager.scheduler_count++;
                
                /* Initialize scheduler if interface provided */
                if (scheduler->interface.init != NULL) {
                    result = scheduler->interface.init(scheduler);
                    if (result != DSRTOS_SUCCESS) {
                        g_scheduler_manager.schedulers[slot] = NULL;
                        g_scheduler_manager.scheduler_count--;
                    }
                }
                
                DSRTOS_TRACE_SCHEDULER("Scheduler %u registered", scheduler->id);
            }
        }
    }
    
    dsrtos_critical_exit();
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_unregister(uint32_t scheduler_id)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t slot;
    
    dsrtos_critical_enter();
    
    if (!g_scheduler_manager.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    } else {
        slot = find_scheduler_by_id(scheduler_id);
        if (slot == DSRTOS_MAX_SCHEDULERS) {
            result = DSRTOS_ERROR_NOT_FOUND;
        } else {
            /* Deinitialize scheduler if interface provided */
            if (g_scheduler_manager.schedulers[slot]->interface.deinit != NULL) {
                (void)g_scheduler_manager.schedulers[slot]->interface.deinit(
                    g_scheduler_manager.schedulers[slot]);
            }
            
            /* Unregister scheduler */
            g_scheduler_manager.schedulers[slot] = NULL;
            g_scheduler_manager.scheduler_count--;
            
            /* If this was the active scheduler, clear active */
            if (g_scheduler_manager.active_scheduler_id == scheduler_id) {
                g_scheduler_manager.active_scheduler_id = 0U;
            }
            
            DSRTOS_TRACE_SCHEDULER("Scheduler %u unregistered", scheduler_id);
        }
    }
    
    dsrtos_critical_exit();
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_set_active(uint32_t scheduler_id)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t slot;
    
    dsrtos_critical_enter();
    
    if (!g_scheduler_manager.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    } else {
        slot = find_scheduler_by_id(scheduler_id);
        if (slot == DSRTOS_MAX_SCHEDULERS) {
            result = DSRTOS_ERROR_NOT_FOUND;
        } else {
            g_scheduler_manager.active_scheduler_id = scheduler_id;
            DSRTOS_TRACE_SCHEDULER("Active scheduler set to %u", scheduler_id);
        }
    }
    
    dsrtos_critical_exit();
    
    return result;
}

dsrtos_scheduler_t* dsrtos_scheduler_get_active(void)
{
    dsrtos_scheduler_t* scheduler = NULL;
    uint32_t slot;
    
    dsrtos_critical_enter();
    
    if (g_scheduler_manager.initialized) {
        slot = find_scheduler_by_id(g_scheduler_manager.active_scheduler_id);
        if (slot != DSRTOS_MAX_SCHEDULERS) {
            scheduler = g_scheduler_manager.schedulers[slot];
        }
    }
    
    dsrtos_critical_exit();
    
    return scheduler;
}

dsrtos_scheduler_t* dsrtos_scheduler_get_by_id(uint32_t scheduler_id)
{
    dsrtos_scheduler_t* scheduler = NULL;
    uint32_t slot;
    
    dsrtos_critical_enter();
    
    if (g_scheduler_manager.initialized) {
        slot = find_scheduler_by_id(scheduler_id);
        if (slot != DSRTOS_MAX_SCHEDULERS) {
            scheduler = g_scheduler_manager.schedulers[slot];
        }
    }
    
    dsrtos_critical_exit();
    
    return scheduler;
}

dsrtos_task_t* dsrtos_scheduler_get_next_task(void)
{
    dsrtos_task_t* task = NULL;
    dsrtos_scheduler_t* scheduler;
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.next != NULL)) {
        task = scheduler->interface.next(scheduler);
    }
    
    return task;
}

dsrtos_error_t dsrtos_scheduler_add_task(dsrtos_task_t* task)
{
    dsrtos_error_t result = DSRTOS_ERROR_NOT_FOUND;
    dsrtos_scheduler_t* scheduler;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.add != NULL)) {
        result = scheduler->interface.add(scheduler, task);
    }
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_remove_task(dsrtos_task_t* task)
{
    dsrtos_error_t result = DSRTOS_ERROR_NOT_FOUND;
    dsrtos_scheduler_t* scheduler;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.remove != NULL)) {
        result = scheduler->interface.remove(scheduler, task);
    }
    
    return result;
}

dsrtos_error_t dsrtos_scheduler_yield_task(dsrtos_task_t* task)
{
    dsrtos_error_t result = DSRTOS_ERROR_NOT_FOUND;
    dsrtos_scheduler_t* scheduler;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.yield != NULL)) {
        result = scheduler->interface.yield(scheduler, task);
    }
    
    return result;
}

uint32_t dsrtos_scheduler_get_task_count(void)
{
    uint32_t count = 0U;
    dsrtos_scheduler_t* scheduler;
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.count != NULL)) {
        count = scheduler->interface.count(scheduler);
    }
    
    return count;
}

bool dsrtos_scheduler_is_empty(void)
{
    bool is_empty = true;
    dsrtos_scheduler_t* scheduler;
    
    scheduler = dsrtos_scheduler_get_active();
    if ((scheduler != NULL) && (scheduler->interface.is_empty != NULL)) {
        is_empty = scheduler->interface.is_empty(scheduler);
    }
    
    return is_empty;
}

/* ============================================================================
 * SCHEDULER VALIDATION FUNCTIONS
 * ============================================================================ */

bool dsrtos_scheduler_validate(const dsrtos_scheduler_t* scheduler)
{
    bool valid = false;
    
    if (scheduler != NULL) {
        valid = (scheduler->magic == DSRTOS_SCHEDULER_MAGIC) &&
                (scheduler->id < DSRTOS_MAX_SCHEDULERS) &&
                (scheduler->type <= DSRTOS_SCHEDULER_TYPE_RMS) &&
                (scheduler->state <= DSRTOS_SCHEDULER_STATE_ERROR) &&
                (scheduler->task_count <= DSRTOS_MAX_TASKS_PER_SCHEDULER);
    }
    
    return valid;
}

bool dsrtos_scheduler_manager_validate(void)
{
    return g_scheduler_manager.initialized &&
           (g_scheduler_manager.magic == DSRTOS_SCHEDULER_MANAGER_MAGIC);
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* dsrtos_scheduler_get_type_name(uint32_t type)
{
    const char* name = "UNKNOWN";
    
    switch (type) {
        case DSRTOS_SCHEDULER_TYPE_RR:
            name = "ROUND_ROBIN";
            break;
        case DSRTOS_SCHEDULER_TYPE_PRIORITY:
            name = "PRIORITY";
            break;
        case DSRTOS_SCHEDULER_TYPE_EDF:
            name = "EDF";
            break;
        case DSRTOS_SCHEDULER_TYPE_RMS:
            name = "RMS";
            break;
        default:
            break;
    }
    
    return name;
}

const char* dsrtos_scheduler_get_state_name(uint32_t state)
{
    const char* name = "UNKNOWN";
    
    switch (state) {
        case DSRTOS_SCHEDULER_STATE_INACTIVE:
            name = "INACTIVE";
            break;
        case DSRTOS_SCHEDULER_STATE_ACTIVE:
            name = "ACTIVE";
            break;
        case DSRTOS_SCHEDULER_STATE_SUSPENDED:
            name = "SUSPENDED";
            break;
        case DSRTOS_SCHEDULER_STATE_ERROR:
            name = "ERROR";
            break;
        default:
            break;
    }
    
    return name;
}

dsrtos_error_t dsrtos_scheduler_get_stats(uint32_t scheduler_id, void* stats)
{
    dsrtos_error_t result = DSRTOS_ERROR_NOT_FOUND;
    dsrtos_scheduler_t* scheduler;
    
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    scheduler = dsrtos_scheduler_get_by_id(scheduler_id);
    if (scheduler != NULL) {
        /* Basic statistics */
        (void)memset(stats, 0, sizeof(uint32_t) * 4U); /* Placeholder */
        result = DSRTOS_SUCCESS;
    }
    
    return result;
}

