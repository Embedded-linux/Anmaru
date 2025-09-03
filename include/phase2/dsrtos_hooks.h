/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Hook Functions Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_HOOKS_H
#define DSRTOS_HOOKS_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"

/*=============================================================================
 * HOOK TYPES
 *============================================================================*/
typedef enum {
    /* Kernel Hooks */
    DSRTOS_HOOK_KERNEL_PRE_START       = 0x0000U,
    DSRTOS_HOOK_KERNEL_POST_START      = 0x0001U,
    DSRTOS_HOOK_KERNEL_PRE_SHUTDOWN    = 0x0002U,
    DSRTOS_HOOK_KERNEL_POST_SHUTDOWN   = 0x0003U,
    DSRTOS_HOOK_KERNEL_IDLE            = 0x0004U,
    DSRTOS_HOOK_KERNEL_TICK            = 0x0005U,
    
    /* Task Hooks */
    DSRTOS_HOOK_TASK_CREATE            = 0x0010U,
    DSRTOS_HOOK_TASK_DELETE            = 0x0011U,
    DSRTOS_HOOK_TASK_SWITCH_IN         = 0x0012U,
    DSRTOS_HOOK_TASK_SWITCH_OUT        = 0x0013U,
    DSRTOS_HOOK_TASK_STACK_OVERFLOW    = 0x0014U,
    
    /* Memory Hooks */
    DSRTOS_HOOK_MEM_ALLOC              = 0x0020U,
    DSRTOS_HOOK_MEM_FREE               = 0x0021U,
    DSRTOS_HOOK_MEM_CORRUPTION         = 0x0022U,
    
    /* Error Hooks */
    DSRTOS_HOOK_ERROR_FATAL            = 0x0030U,
    DSRTOS_HOOK_ERROR_ASSERT           = 0x0031U,
    DSRTOS_HOOK_ERROR_DEADLINE_MISS    = 0x0032U,
    
    /* Application Hooks */
    DSRTOS_HOOK_APP_INIT               = 0x0040U,
    DSRTOS_HOOK_APP_START              = 0x0041U,
    DSRTOS_HOOK_APP_STOP               = 0x0042U,
    
    /* Debug Hooks */
    DSRTOS_HOOK_DEBUG_TRACE            = 0x0050U,
    DSRTOS_HOOK_DEBUG_BREAK            = 0x0051U,
    
    /* Custom Hooks */
    DSRTOS_HOOK_CUSTOM_BASE            = 0x1000U,
    
    DSRTOS_HOOK_MAX                    = 0xFFFFU
} dsrtos_hook_type_t;

/*=============================================================================
 * HOOK FUNCTION TYPES
 *============================================================================*/

/**
 * @brief Generic hook function type
 * @param[in] type Hook type
 * @param[in] params Hook-specific parameters
 * @return Hook-specific return value
 */
typedef void* (*dsrtos_hook_func_t)(dsrtos_hook_type_t type, void* params);

/**
 * @brief Kernel hook function type
 * @param[in] params Kernel-specific parameters
 */
typedef void (*dsrtos_kernel_hook_t)(void* params);

/**
 * @brief Task hook function type
 * @param[in] task Task control block pointer
 */
typedef void (*dsrtos_task_hook_t)(void* task);

/**
 * @brief Memory hook function type
 * @param[in] address Memory address
 * @param[in] size Memory size
 */
typedef void (*dsrtos_mem_hook_t)(void* address, size_t size);

/**
 * @brief Error hook function type
 * @param[in] error Error code
 * @param[in] context Error context
 */
typedef void (*dsrtos_error_hook_t)(dsrtos_error_t error, void* context);

/*=============================================================================
 * HOOK REGISTRATION STRUCTURE
 *============================================================================*/
typedef struct {
    dsrtos_hook_type_t type;           /* Hook type */
    dsrtos_hook_func_t function;       /* Hook function */
    void* user_data;                    /* User data */
    uint32_t priority;                  /* Hook priority (lower = higher priority) */
    bool enabled;                       /* Hook enabled flag */
    const char* name;                   /* Hook name for debugging */
} dsrtos_hook_entry_t;

/*=============================================================================
 * HOOK CHAIN STRUCTURE
 *============================================================================*/
typedef struct dsrtos_hook_node {
    dsrtos_hook_entry_t entry;         /* Hook entry */
    struct dsrtos_hook_node* next;     /* Next hook in chain */
} dsrtos_hook_node_t;

/*=============================================================================
 * HOOK STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t call_count;                /* Total calls */
    uint32_t total_duration_cycles;    /* Total execution time */
    uint32_t max_duration_cycles;      /* Maximum execution time */
    uint32_t min_duration_cycles;      /* Minimum execution time */
} dsrtos_hook_stats_t;

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize hook system
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-001: Initialization
 * @safety Must be called during kernel initialization
 */
dsrtos_error_t dsrtos_hooks_init(void);

/**
 * @brief Register a hook function
 * 
 * @param[in] entry Hook entry to register
 * @return Handle to registered hook or NULL on failure
 * 
 * @requirements REQ-HOOK-002: Registration
 * @safety Thread-safe
 */
void* dsrtos_hook_register(const dsrtos_hook_entry_t* entry);

/**
 * @brief Unregister a hook function
 * 
 * @param[in] handle Hook handle returned by register
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-003: Unregistration
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_hook_unregister(void* handle);

/**
 * @brief Call hook functions for a specific type
 * 
 * @param[in] type Hook type to call
 * @param[in] params Parameters to pass to hooks
 * @return Combined return value from hooks
 * 
 * @requirements REQ-HOOK-004: Hook execution
 * @safety May be called from ISR depending on hook type
 */
void* dsrtos_hook_call(dsrtos_hook_type_t type, void* params);

/**
 * @brief Enable a hook
 * 
 * @param[in] handle Hook handle
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-005: Hook control
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_hook_enable(void* handle);

/**
 * @brief Disable a hook
 * 
 * @param[in] handle Hook handle
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-006: Hook control
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_hook_disable(void* handle);

/**
 * @brief Check if a hook is enabled
 * 
 * @param[in] handle Hook handle
 * @return true if enabled, false otherwise
 * 
 * @requirements REQ-HOOK-007: Hook status
 * @safety Thread-safe
 */
bool dsrtos_hook_is_enabled(void* handle);

/**
 * @brief Get hook statistics
 * 
 * @param[in] type Hook type
 * @param[out] stats Statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-008: Statistics
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_hook_get_stats(dsrtos_hook_type_t type,
                                     dsrtos_hook_stats_t* stats);

/**
 * @brief Reset hook statistics
 * 
 * @param[in] type Hook type (DSRTOS_HOOK_MAX for all)
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-HOOK-009: Statistics reset
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_hook_reset_stats(dsrtos_hook_type_t type);

/**
 * @brief Get number of registered hooks
 * 
 * @param[in] type Hook type (DSRTOS_HOOK_MAX for all)
 * @return Number of registered hooks
 * 
 * @requirements REQ-HOOK-010: Hook count
 * @safety Thread-safe
 */
uint32_t dsrtos_hook_get_count(dsrtos_hook_type_t type);

/*=============================================================================
 * CONVENIENCE MACROS
 *============================================================================*/

/**
 * @brief Call kernel idle hook
 */
#define DSRTOS_HOOK_IDLE() \
    dsrtos_hook_call(DSRTOS_HOOK_KERNEL_IDLE, NULL)

/**
 * @brief Call kernel tick hook
 */
#define DSRTOS_HOOK_TICK() \
    dsrtos_hook_call(DSRTOS_HOOK_KERNEL_TICK, NULL)

/**
 * @brief Call task switch hook
 */
#define DSRTOS_HOOK_TASK_SWITCH(old_task, new_task) \
    do { \
        dsrtos_hook_call(DSRTOS_HOOK_TASK_SWITCH_OUT, (old_task)); \
        dsrtos_hook_call(DSRTOS_HOOK_TASK_SWITCH_IN, (new_task)); \
    } while (0)

/**
 * @brief Call error hook
 */
#define DSRTOS_HOOK_ERROR(error, context) \
    dsrtos_hook_call(DSRTOS_HOOK_ERROR_FATAL, \
                    &(struct { dsrtos_error_t e; void* c; }){(error), (context)})

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_HOOKS_H */
