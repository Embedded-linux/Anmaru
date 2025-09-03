/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Hook Functions Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_hooks.h"
#include "dsrtos_kernel_init.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include <string.h>
#include <stdlib.h>
#include "core_cm4.h"

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define HOOK_MAGIC              0x484F4F4BU  /* 'HOOK' */
#define HOOK_MAX_CHAINS         64U
#define HOOK_POOL_SIZE          128U

/*=============================================================================
 * PRIVATE TYPES
 *============================================================================*/
typedef struct {
    dsrtos_hook_node_t* chains[HOOK_MAX_CHAINS];
    dsrtos_hook_stats_t stats[HOOK_MAX_CHAINS];
    dsrtos_hook_node_t node_pool[HOOK_POOL_SIZE];
    uint32_t node_pool_used;
    uint32_t magic;
    bool initialized;
} hook_manager_t;

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
/* Hook manager - aligned for performance */
static hook_manager_t g_hook_mgr __attribute__((aligned(32))) = {0};

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static uint32_t hook_type_to_index(dsrtos_hook_type_t type);
static dsrtos_hook_node_t* hook_allocate_node(void);
static void hook_free_node(dsrtos_hook_node_t* node);
static void hook_insert_sorted(dsrtos_hook_node_t** chain, dsrtos_hook_node_t* node);
static void hook_update_stats(uint32_t index, uint32_t duration_cycles);
static uint32_t hook_get_timestamp(void);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize hook system
 */
dsrtos_error_t dsrtos_hooks_init(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check if already initialized */
    if (g_hook_mgr.initialized) {
        result = DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    else {
        /* Clear hook manager */
        memset(&g_hook_mgr, 0, sizeof(g_hook_mgr));
        
        /* Initialize manager */
        g_hook_mgr.magic = HOOK_MAGIC;
        g_hook_mgr.node_pool_used = 0U;
        g_hook_mgr.initialized = true;
        
        /* Initialize node pool */
        for (uint32_t i = 0U; i < HOOK_POOL_SIZE; i++) {
            g_hook_mgr.node_pool[i].next = NULL;
        }
        
        /* Register with kernel */
        result = dsrtos_kernel_register_service(
            DSRTOS_SERVICE_HOOKS,
            &g_hook_mgr
        );
    }
    
    return result;
}

/**
 * @brief Register a hook function
 */
void* dsrtos_hook_register(const dsrtos_hook_entry_t* entry)
{
    void* handle = NULL;
    
    if ((entry == NULL) || (entry->function == NULL)) {
        return NULL;
    }
    
    if (!g_hook_mgr.initialized) {
        return NULL;
    }
    
    /* Get chain index */
    uint32_t index = hook_type_to_index(entry->type);
    if (index >= HOOK_MAX_CHAINS) {
        return NULL;
    }
    
    /* Allocate node */
    dsrtos_hook_node_t* node = hook_allocate_node();
    if (node == NULL) {
        return NULL;
    }
    
    /* Initialize node */
    node->entry = *entry;
    node->next = NULL;
    
    /* Insert into chain (sorted by priority) */
    dsrtos_critical_enter();
    hook_insert_sorted(&g_hook_mgr.chains[index], node);
    dsrtos_critical_exit();
    
    /* Return node as handle */
    handle = (void*)node;
    
    return handle;
}

/**
 * @brief Unregister a hook function
 */
dsrtos_error_t dsrtos_hook_unregister(void* handle)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (handle == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_hook_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_hook_node_t* node = (dsrtos_hook_node_t*)handle;
        uint32_t index = hook_type_to_index(node->entry.type);
        
        if (index >= HOOK_MAX_CHAINS) {
            result = DSRTOS_ERROR_INVALID_PARAM;
        }
        else {
            /* Remove from chain */
            dsrtos_critical_enter();
            
            dsrtos_hook_node_t** current = &g_hook_mgr.chains[index];
            while (*current != NULL) {
                if (*current == node) {
                    *current = node->next;
                    hook_free_node(node);
                    break;
                }
                current = &(*current)->next;
            }
            
            dsrtos_critical_exit();
        }
    }
    
    return result;
}

/**
 * @brief Call hook functions for a specific type
 */
void* dsrtos_hook_call(dsrtos_hook_type_t type, void* params)
{
    void* result = NULL;
    
    if (!g_hook_mgr.initialized) {
        return NULL;
    }
    
    uint32_t index = hook_type_to_index(type);
    if (index >= HOOK_MAX_CHAINS) {
        return NULL;
    }
    
    /* Get chain head */
    dsrtos_hook_node_t* node = g_hook_mgr.chains[index];
    
    /* Call each hook in chain */
    while (node != NULL) {
        if (node->entry.enabled) {
            /* Record start time */
            uint32_t start_cycles = hook_get_timestamp();
            
            /* Call hook function */
            void* hook_result = node->entry.function(type, params);
            
            /* Update statistics */
            uint32_t duration_cycles = hook_get_timestamp() - start_cycles;
            hook_update_stats(index, duration_cycles);
            
            /* Combine results (hook-specific logic) */
            if (hook_result != NULL) {
                result = hook_result;
            }
        }
        
        node = node->next;
    }
    
    return result;
}

/**
 * @brief Enable a hook
 */
dsrtos_error_t dsrtos_hook_enable(void* handle)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (handle == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else {
        dsrtos_hook_node_t* node = (dsrtos_hook_node_t*)handle;
        node->entry.enabled = true;
    }
    
    return result;
}

/**
 * @brief Disable a hook
 */
dsrtos_error_t dsrtos_hook_disable(void* handle)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (handle == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else {
        dsrtos_hook_node_t* node = (dsrtos_hook_node_t*)handle;
        node->entry.enabled = false;
    }
    
    return result;
}

/**
 * @brief Check if a hook is enabled
 */
bool dsrtos_hook_is_enabled(void* handle)
{
    bool enabled = false;
    
    if (handle != NULL) {
        dsrtos_hook_node_t* node = (dsrtos_hook_node_t*)handle;
        enabled = node->entry.enabled;
    }
    
    return enabled;
}

/**
 * @brief Get hook statistics
 */
dsrtos_error_t dsrtos_hook_get_stats(dsrtos_hook_type_t type,
                                     dsrtos_hook_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_hook_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        uint32_t index = hook_type_to_index(type);
        if (index >= HOOK_MAX_CHAINS) {
            result = DSRTOS_ERROR_INVALID_PARAM;
        }
        else {
            dsrtos_critical_enter();
            *stats = g_hook_mgr.stats[index];
            dsrtos_critical_exit();
        }
    }
    
    return result;
}

/**
 * @brief Reset hook statistics
 */
dsrtos_error_t dsrtos_hook_reset_stats(dsrtos_hook_type_t type)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (!g_hook_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        if (type == DSRTOS_HOOK_MAX) {
            /* Reset all statistics */
            memset(g_hook_mgr.stats, 0, sizeof(g_hook_mgr.stats));
        }
        else {
            uint32_t index = hook_type_to_index(type);
            if (index >= HOOK_MAX_CHAINS) {
                result = DSRTOS_ERROR_INVALID_PARAM;
            }
            else {
                memset(&g_hook_mgr.stats[index], 0, sizeof(dsrtos_hook_stats_t));
            }
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Get number of registered hooks
 */
uint32_t dsrtos_hook_get_count(dsrtos_hook_type_t type)
{
    uint32_t count = 0U;
    
    if (!g_hook_mgr.initialized) {
        return 0U;
    }
    
    dsrtos_critical_enter();
    
    if (type == DSRTOS_HOOK_MAX) {
        /* Count all hooks */
        for (uint32_t i = 0U; i < HOOK_MAX_CHAINS; i++) {
            dsrtos_hook_node_t* node = g_hook_mgr.chains[i];
            while (node != NULL) {
                count++;
                node = node->next;
            }
        }
    }
    else {
        /* Count specific type */
        uint32_t index = hook_type_to_index(type);
        if (index < HOOK_MAX_CHAINS) {
            dsrtos_hook_node_t* node = g_hook_mgr.chains[index];
            while (node != NULL) {
                count++;
                node = node->next;
            }
        }
    }
    
    dsrtos_critical_exit();
    
    return count;
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Convert hook type to chain index
 */
static uint32_t hook_type_to_index(dsrtos_hook_type_t type)
{
    /* Simple hash function for hook types */
    return (uint32_t)type % HOOK_MAX_CHAINS;
}

/**
 * @brief Allocate a hook node from pool
 */
static dsrtos_hook_node_t* hook_allocate_node(void)
{
    dsrtos_hook_node_t* node = NULL;
    
    dsrtos_critical_enter();
    
    if (g_hook_mgr.node_pool_used < HOOK_POOL_SIZE) {
        node = &g_hook_mgr.node_pool[g_hook_mgr.node_pool_used];
        g_hook_mgr.node_pool_used++;
    }
    
    dsrtos_critical_exit();
    
    return node;
}

/**
 * @brief Free a hook node back to pool
 */
static void hook_free_node(dsrtos_hook_node_t* node)
{
    /* In this simple implementation, we don't free nodes */
    /* A more sophisticated implementation would maintain a free list */
    if (node != NULL) {
        memset(node, 0, sizeof(dsrtos_hook_node_t));
    }
}

/**
 * @brief Insert node into chain sorted by priority
 */
static void hook_insert_sorted(dsrtos_hook_node_t** chain, dsrtos_hook_node_t* node)
{
    /* Find insertion point based on priority */
    while (*chain != NULL && (*chain)->entry.priority <= node->entry.priority) {
        chain = &(*chain)->next;
    }
    
    /* Insert node */
    node->next = *chain;
    *chain = node;
}

/**
 * @brief Update hook statistics
 */
static void hook_update_stats(uint32_t index, uint32_t duration_cycles)
{
    dsrtos_hook_stats_t* stats = &g_hook_mgr.stats[index];
    
    stats->call_count++;
    stats->total_duration_cycles += duration_cycles;
    
    if (duration_cycles > stats->max_duration_cycles) {
        stats->max_duration_cycles = duration_cycles;
    }
    
    if ((stats->min_duration_cycles == 0U) || 
        (duration_cycles < stats->min_duration_cycles)) {
        stats->min_duration_cycles = duration_cycles;
    }
}

/**
 * @brief Get current timestamp
 */
static uint32_t hook_get_timestamp(void)
{
    /* Use DWT cycle counter if available */
    #ifdef DWT
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) != 0U) {
        return DWT->CYCCNT;
    }
    #endif
    
    /* Fallback to SysTick */
    return (0xFFFFFFU - SysTick->VAL);
}
