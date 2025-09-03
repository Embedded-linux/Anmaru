/*
 * DSRTOS Priority Management Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_priority_mgmt.h"
#include "dsrtos_task_scheduler_interface.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_trace.h"
#include <string.h>

/* Maximum resources for ceiling protocol */
#define MAX_RESOURCES 64U

/* Global resource ceiling table */
static dsrtos_resource_ceiling_t g_resource_ceilings[MAX_RESOURCES];

/* Priority inheritance chain pool */
static dsrtos_pi_chain_node_t g_pi_chain_pool[DSRTOS_MAX_TASKS];
static uint32_t g_pi_chain_bitmap[(DSRTOS_MAX_TASKS + 31U) / 32U];

/* Statistics */
static struct {
    uint32_t priority_inversions;
    uint32_t inheritance_applications;
    uint32_t ceiling_violations;
    uint32_t max_inheritance_depth;
} g_priority_stats = {0};

/* Static function prototypes */
static dsrtos_pi_chain_node_t* allocate_chain_node(void);
static void free_chain_node(dsrtos_pi_chain_node_t* node);
static dsrtos_error_t apply_priority_change(dsrtos_tcb_t* task, uint8_t new_priority);

/**
 * @brief Initialize priority management
 */
dsrtos_error_t dsrtos_priority_mgmt_init(void)
{
    uint32_t i;
    
    /* Initialize resource ceiling table */
    (void)memset(g_resource_ceilings, 0, sizeof(g_resource_ceilings));
    for (i = 0U; i < MAX_RESOURCES; i++) {
        g_resource_ceilings[i].resource_id = i;
        g_resource_ceilings[i].ceiling_priority = 0U;
        g_resource_ceilings[i].owner_count = 0U;
        g_resource_ceilings[i].in_use = false;
    }
    
    /* Initialize chain pool */
    (void)memset(g_pi_chain_pool, 0, sizeof(g_pi_chain_pool));
    (void)memset(g_pi_chain_bitmap, 0, sizeof(g_pi_chain_bitmap));
    
    /* Clear statistics */
    g_priority_stats.priority_inversions = 0U;
    g_priority_stats.inheritance_applications = 0U;
    g_priority_stats.ceiling_violations = 0U;
    g_priority_stats.max_inheritance_depth = 0U;
    
    DSRTOS_TRACE_PRIORITY("Priority management initialized");
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Set task priority with validation
 */
dsrtos_error_t dsrtos_priority_set(dsrtos_tcb_t* task, uint8_t priority)
{
    dsrtos_error_t result;
    uint8_t old_priority;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    if (!dsrtos_priority_is_valid(priority)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Check task state */
    if (task->state == DSRTOS_TASK_STATE_INVALID) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    dsrtos_critical_enter();
    
    old_priority = task->static_priority;
    task->static_priority = priority;
    
    /* Update effective priority if not inherited */
    if (task->effective_priority == old_priority) {
        result = apply_priority_change(task, priority);
    } else {
        /* Task has inherited priority, don't change effective */
        result = DSRTOS_SUCCESS;
    }
    
    dsrtos_critical_exit();
    
    if (result == DSRTOS_SUCCESS) {
        DSRTOS_TRACE_PRIORITY("Task %u priority changed %u -> %u",
                             task->task_id, old_priority, priority);
    }
    
    return result;
}

/**
 * @brief Get task priority
 */
dsrtos_error_t dsrtos_priority_get(const dsrtos_tcb_t* task, uint8_t* priority)
{
    if ((task == NULL) || (priority == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    if (task->magic_number != DSRTOS_TCB_MAGIC) {
        return DSRTOS_ERROR_CORRUPTED;
    }
    
    *priority = task->effective_priority;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Boost task priority temporarily
 */
dsrtos_error_t dsrtos_priority_boost(dsrtos_tcb_t* task, uint8_t boost_priority)
{
    dsrtos_error_t result;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    if (!dsrtos_priority_is_valid(boost_priority)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    
    /* Only boost if higher than current */
    if (boost_priority > task->effective_priority) {
        result = apply_priority_change(task, boost_priority);
        if (result == DSRTOS_SUCCESS) {
            DSRTOS_TRACE_PRIORITY("Task %u boosted to priority %u",
                                 task->task_id, boost_priority);
        }
    } else {
        result = DSRTOS_SUCCESS;
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Restore task to base priority
 */
dsrtos_error_t dsrtos_priority_restore(dsrtos_tcb_t* task)
{
    dsrtos_error_t result;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    
    if (task->effective_priority != task->static_priority) {
        result = apply_priority_change(task, task->static_priority);
        if (result == DSRTOS_SUCCESS) {
            DSRTOS_TRACE_PRIORITY("Task %u priority restored to %u",
                                 task->task_id, task->static_priority);
        }
    } else {
        result = DSRTOS_SUCCESS;
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Apply priority inheritance
 */
dsrtos_error_t dsrtos_priority_inherit_apply(
    dsrtos_tcb_t* owner,
    dsrtos_tcb_t* requester)
{
    dsrtos_pi_chain_node_t* node;
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((owner == NULL) || (requester == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if inheritance needed */
    if (requester->effective_priority <= owner->effective_priority) {
        return DSRTOS_SUCCESS;  /* No inheritance needed */
    }
    
    DSRTOS_TRACE_PRIORITY("Applying priority inheritance: %u -> %u",
                         owner->task_id, requester->effective_priority);
    
    dsrtos_critical_enter();
    
    /* Detect priority inversion */
    g_priority_stats.priority_inversions++;
    
    /* Allocate chain node */
    node = allocate_chain_node();
    if (node == NULL) {
        dsrtos_critical_exit();
        return DSRTOS_ERROR_NO_MEMORY;
    }
    
    /* Initialize node */
    node->task = requester;
    node->inherited_priority = requester->effective_priority;
    node->timestamp = dsrtos_get_tick_count();
    node->next = (dsrtos_pi_chain_node_t*)owner->priority_chain;
    
    /* Add to owner's chain */
    owner->priority_chain = node;
    
    /* Apply inherited priority */
    result = apply_priority_change(owner, requester->effective_priority);
    if (result == DSRTOS_SUCCESS) {
        g_priority_stats.inheritance_applications++;
    }
    
    /* Walk the chain if needed */
    if ((result == DSRTOS_SUCCESS) && (owner->blocked_on != NULL)) {
        result = dsrtos_priority_inherit_chain_walk(owner, 
                                                    requester->effective_priority);
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Release priority inheritance
 */
dsrtos_error_t dsrtos_priority_inherit_release(dsrtos_tcb_t* task)
{
    dsrtos_pi_chain_node_t* node;
    dsrtos_pi_chain_node_t* next;
    uint8_t highest_inherited = 0U;
    dsrtos_error_t result;
    
    if (task == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    
    /* Find highest inherited priority in chain */
    node = (dsrtos_pi_chain_node_t*)task->priority_chain;
    while (node != NULL) {
        if (node->inherited_priority > highest_inherited) {
            highest_inherited = node->inherited_priority;
        }
        next = node->next;
        free_chain_node(node);
        node = next;
    }
    
    /* Clear chain */
    task->priority_chain = NULL;
    
    /* Restore priority */
    if (highest_inherited > task->static_priority) {
        result = apply_priority_change(task, highest_inherited);
    } else {
        result = apply_priority_change(task, task->static_priority);
    }
    
    dsrtos_critical_exit();
    
    DSRTOS_TRACE_PRIORITY("Task %u inheritance released", task->task_id);
    
    return result;
}

/**
 * @brief Walk priority inheritance chain
 */
dsrtos_error_t dsrtos_priority_inherit_chain_walk(
    dsrtos_tcb_t* start_task,
    uint8_t priority)
{
    dsrtos_tcb_t* current = start_task;
    uint32_t depth = 0U;
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Walk chain and propagate priority */
    while ((current != NULL) && 
           (current->blocked_on != NULL) &&
           (depth < DSRTOS_MAX_INHERITANCE_CHAIN)) {
        
        /* Get owner of blocking resource */
        dsrtos_tcb_t* owner = (dsrtos_tcb_t*)current->blocked_on;
        
        /* Apply inheritance if needed */
        if (priority > owner->effective_priority) {
            result = apply_priority_change(owner, priority);
            if (result != DSRTOS_SUCCESS) {
                break;
            }
        }
        
        current = owner;
        depth++;
    }
    
    /* Update max depth statistic */
    if (depth > g_priority_stats.max_inheritance_depth) {
        g_priority_stats.max_inheritance_depth = depth;
    }
    
    /* Check for excessive chain */
    if (depth >= DSRTOS_MAX_INHERITANCE_CHAIN) {
        DSRTOS_TRACE_WARNING("Priority inheritance chain limit reached");
        return DSRTOS_ERROR_LIMIT_EXCEEDED;
    }
    
    return result;
}

/**
 * @brief Set resource ceiling priority
 */
dsrtos_error_t dsrtos_priority_ceiling_set(void* resource, uint8_t ceiling)
{
    uint32_t res_id;
    
    if (resource == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    if (!dsrtos_priority_is_valid(ceiling)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Simple hash for resource ID */
    res_id = ((uintptr_t)resource / 4U) % MAX_RESOURCES;
    
    dsrtos_critical_enter();
    
    g_resource_ceilings[res_id].ceiling_priority = ceiling;
    g_resource_ceilings[res_id].in_use = true;
    
    dsrtos_critical_exit();
    
    DSRTOS_TRACE_PRIORITY("Resource %p ceiling set to %u", resource, ceiling);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get resource ceiling priority
 */
dsrtos_error_t dsrtos_priority_ceiling_get(void* resource, uint8_t* ceiling)
{
    uint32_t res_id;
    
    if ((resource == NULL) || (ceiling == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    res_id = ((uintptr_t)resource / 4U) % MAX_RESOURCES;
    
    dsrtos_critical_enter();
    
    if (g_resource_ceilings[res_id].in_use) {
        *ceiling = g_resource_ceilings[res_id].ceiling_priority;
    } else {
        *ceiling = 0U;
    }
    
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Enter priority ceiling protocol
 */
dsrtos_error_t dsrtos_priority_ceiling_enter(dsrtos_tcb_t* task, void* resource)
{
    uint32_t res_id;
    uint8_t ceiling;
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((task == NULL) || (resource == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    res_id = ((uintptr_t)resource / 4U) % MAX_RESOURCES;
    
    dsrtos_critical_enter();
    
    if (!g_resource_ceilings[res_id].in_use) {
        dsrtos_critical_exit();
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    ceiling = g_resource_ceilings[res_id].ceiling_priority;
    
    /* Check for ceiling violation */
    if (task->static_priority > ceiling) {
        g_priority_stats.ceiling_violations++;
        dsrtos_critical_exit();
        return DSRTOS_ERROR_PRIORITY_CEILING_VIOLATED;
    }
    
    /* Raise priority to ceiling if needed */
    if (task->effective_priority < ceiling) {
        result = apply_priority_change(task, ceiling);
    }
    
    if (result == DSRTOS_SUCCESS) {
        g_resource_ceilings[res_id].owner_count++;
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Exit priority ceiling protocol
 */
dsrtos_error_t dsrtos_priority_ceiling_exit(dsrtos_tcb_t* task, void* resource)
{
    uint32_t res_id;
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((task == NULL) || (resource == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    res_id = ((uintptr_t)resource / 4U) % MAX_RESOURCES;
    
    dsrtos_critical_enter();
    
    if (g_resource_ceilings[res_id].owner_count > 0U) {
        g_resource_ceilings[res_id].owner_count--;
        
        /* Restore priority if no more resources held */
        if (g_resource_ceilings[res_id].owner_count == 0U) {
            result = dsrtos_priority_restore(task);
        }
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Check if priority is valid
 */
bool dsrtos_priority_is_valid(uint8_t priority)
{
    return priority < DSRTOS_PRIORITY_LEVELS;
}

/**
 * @brief Normalize priority to valid range
 */
uint8_t dsrtos_priority_normalize(uint8_t priority)
{
    if (priority >= DSRTOS_PRIORITY_LEVELS) {
        return DSRTOS_PRIORITY_LEVELS - 1U;
    }
    return priority;
}

/**
 * @brief Check for priority inversion
 */
bool dsrtos_priority_inversion_detected(const dsrtos_tcb_t* task)
{
    if (task == NULL) {
        return false;
    }
    
    /* Inversion detected if blocked by lower priority task */
    if ((task->blocked_on != NULL) &&
        (task->effective_priority > ((dsrtos_tcb_t*)task->blocked_on)->effective_priority)) {
        return true;
    }
    
    return false;
}

/**
 * @brief Get priority inheritance chain depth
 */
uint32_t dsrtos_priority_get_inheritance_depth(const dsrtos_tcb_t* task)
{
    uint32_t depth = 0U;
    dsrtos_pi_chain_node_t* node;
    
    if (task == NULL) {
        return 0U;
    }
    
    node = (dsrtos_pi_chain_node_t*)task->priority_chain;
    while (node != NULL) {
        depth++;
        node = node->next;
    }
    
    return depth;
}

/*==============================================================================
 * STATIC HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Allocate chain node from pool
 */
static dsrtos_pi_chain_node_t* allocate_chain_node(void)
{
    uint32_t i, j;
    uint32_t bit_mask;
    
    for (i = 0U; i < ((DSRTOS_MAX_TASKS + 31U) / 32U); i++) {
        if (g_pi_chain_bitmap[i] != 0xFFFFFFFFU) {
            for (j = 0U; j < 32U; j++) {
                bit_mask = 1UL << j;
                if ((g_pi_chain_bitmap[i] & bit_mask) == 0U) {
                    g_pi_chain_bitmap[i] |= bit_mask;
                    
                    uint32_t index = (i * 32U) + j;
                    if (index < DSRTOS_MAX_TASKS) {
                        (void)memset(&g_pi_chain_pool[index], 0,
                                   sizeof(dsrtos_pi_chain_node_t));
                        return &g_pi_chain_pool[index];
                    }
                }
            }
        }
    }
    
    return NULL;
}

/**
 * @brief Free chain node back to pool
 */
static void free_chain_node(dsrtos_pi_chain_node_t* node)
{
    uint32_t index;
    uint32_t word_index;
    uint32_t bit_mask;
    
    if ((node == NULL) || (node < g_pi_chain_pool) ||
        (node >= &g_pi_chain_pool[DSRTOS_MAX_TASKS])) {
        return;
    }
    
    index = (uint32_t)(node - g_pi_chain_pool);
    word_index = index / 32U;
    bit_mask = 1UL << (index % 32U);
    
    (void)memset(node, 0, sizeof(dsrtos_pi_chain_node_t));
    g_pi_chain_bitmap[word_index] &= ~bit_mask;
}

/**
 * @brief Apply priority change to task
 */
static dsrtos_error_t apply_priority_change(dsrtos_tcb_t* task, uint8_t new_priority)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    extern dsrtos_ready_queue_t g_ready_queue;
    
    if ((task->state == DSRTOS_TASK_STATE_READY) ||
        (task->state == DSRTOS_TASK_STATE_RUNNING)) {
        
        /* Remove from current priority */
        if (task->state == DSRTOS_TASK_STATE_READY) {
            result = dsrtos_ready_queue_remove(&g_ready_queue, task);
            if (result != DSRTOS_SUCCESS) {
                return result;
            }
        }
        
        /* Update priority */
        task->effective_priority = new_priority;
        
        /* Reinsert at new priority */
        if (task->state == DSRTOS_TASK_STATE_READY) {
            result = dsrtos_ready_queue_insert(&g_ready_queue, task);
        }
    } else {
        /* Just update priority */
        task->effective_priority = new_priority;
    }
    
    return result;
}
