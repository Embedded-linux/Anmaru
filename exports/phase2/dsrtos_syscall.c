/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: System Call Interface Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_syscall.h"
#include "dsrtos_kernel_init.h"
#include "dsrtos_critical.h"
#include "dsrtos_panic.h"
#include "dsrtos_assert.h"
#include "dsrtos_error.h"
#include <string.h>
#include "core_cm4.h"

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define SYSCALL_TABLE_SIZE      256U
#define SYSCALL_MAGIC           0x53595343U  /* 'SYSC' */
#define SYSCALL_MAX_DURATION_US 1000U

/*=============================================================================
 * PRIVATE TYPES
 *============================================================================*/
typedef struct {
    dsrtos_syscall_entry_t table[SYSCALL_TABLE_SIZE];
    uint32_t count;
    dsrtos_syscall_stats_t stats;
    uint32_t magic;
    bool initialized;
} syscall_manager_t;

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
/* System call manager - aligned for performance */
static syscall_manager_t g_syscall_mgr __attribute__((aligned(32))) = {
    .table = {{0}},
    .count = 0U,
    .stats = {0},
    .magic = 0U,
    .initialized = false
};

/* System call in progress flag for reentrancy check */
static volatile bool g_syscall_active = false;

/* Current system call context */
static dsrtos_syscall_context_t g_current_syscall __attribute__((aligned(16)));

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static dsrtos_error_t syscall_validate_number(dsrtos_syscall_num_t syscall_num);
static dsrtos_syscall_entry_t* syscall_find_entry(dsrtos_syscall_num_t syscall_num);
static dsrtos_error_t syscall_check_privilege(const dsrtos_syscall_entry_t* entry);
static dsrtos_error_t syscall_execute_handler(dsrtos_syscall_context_t* context);
static void syscall_update_stats(uint32_t duration_cycles, bool success);
static uint32_t syscall_get_timestamp(void);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize system call interface
 */
dsrtos_error_t dsrtos_syscall_init(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check if already initialized */
    if (g_syscall_mgr.initialized) {
        result = DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    else {
        /* Clear system call table */
        memset(&g_syscall_mgr, 0, sizeof(g_syscall_mgr));
        
        /* Initialize manager */
        g_syscall_mgr.magic = SYSCALL_MAGIC;
        g_syscall_mgr.count = 0U;
        g_syscall_mgr.initialized = true;
        
        /* Register default system call handlers */
        /* These would be implemented in respective phases */
        
        /* Register with kernel */
        result = dsrtos_kernel_register_service(
            DSRTOS_SERVICE_SYSCALL,
            &g_syscall_mgr
        );
        
        /* Configure SVC priority */
        NVIC_SetPriority(SVCall_IRQn, DSRTOS_SYSCALL_PRIORITY);
    }
    
    return result;
}

/**
 * @brief Register system call handler
 */
dsrtos_error_t dsrtos_syscall_register(const dsrtos_syscall_entry_t* entry)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Validate parameters */
    if (entry == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_syscall_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else if (entry->handler == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (g_syscall_mgr.count >= SYSCALL_TABLE_SIZE) {
        result = DSRTOS_ERROR_NO_MEMORY;
    }
    else {
        /* Check if already registered */
        dsrtos_syscall_entry_t* existing = syscall_find_entry(entry->syscall_num);
        if (existing != NULL) {
            result = DSRTOS_ERROR_ALREADY_EXISTS;
        }
        else {
            /* Add to table */
            uint32_t index = g_syscall_mgr.count;
            g_syscall_mgr.table[index] = *entry;
            g_syscall_mgr.count++;
        }
    }
    
    return result;
}

/**
 * @brief Unregister system call handler
 */
dsrtos_error_t dsrtos_syscall_unregister(dsrtos_syscall_num_t syscall_num)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (!g_syscall_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        /* Find entry */
        bool found = false;
        for (uint32_t i = 0U; i < g_syscall_mgr.count; i++) {
            if (g_syscall_mgr.table[i].syscall_num == syscall_num) {
                /* Remove entry by shifting remaining entries */
                for (uint32_t j = i; j < g_syscall_mgr.count - 1U; j++) {
                    g_syscall_mgr.table[j] = g_syscall_mgr.table[j + 1U];
                }
                g_syscall_mgr.count--;
                found = true;
                break;
            }
        }
        
        if (!found) {
            result = DSRTOS_ERROR_NOT_FOUND;
        }
    }
    
    return result;
}

/**
 * @brief Execute system call
 */
dsrtos_error_t dsrtos_syscall(
    dsrtos_syscall_num_t syscall_num,
    const dsrtos_syscall_params_t* params,
    dsrtos_syscall_result_t* result)
{
    dsrtos_error_t error = DSRTOS_SUCCESS;
    
    /* Validate parameters */
    if ((params == NULL) || (result == NULL)) {
        error = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_syscall_mgr.initialized) {
        error = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        /* Prepare context */
        dsrtos_syscall_context_t context;
        context.syscall_num = syscall_num;
        context.params = *params;
        context.timestamp = syscall_get_timestamp();
        context.flags = 0U;
        
        /* Get current task context (would be implemented in Phase 3) */
        context.caller = NULL;
        
        /* Execute via SVC */
        __asm volatile (
            "mov r0, %0\n"
            "svc #0"
            :
            : "r" (&context)
            : "r0", "memory"
        );
        
        /* Copy result */
        *result = context.result;
        error = context.result.error;
    }
    
    return error;
}

/**
 * @brief Execute system call from ISR
 */
dsrtos_error_t dsrtos_syscall_from_isr(
    dsrtos_syscall_num_t syscall_num,
    const dsrtos_syscall_params_t* params,
    dsrtos_syscall_result_t* result)
{
    dsrtos_error_t error = DSRTOS_SUCCESS;
    
    /* Validate parameters */
    if ((params == NULL) || (result == NULL)) {
        error = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_syscall_mgr.initialized) {
        error = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        /* Find handler */
        dsrtos_syscall_entry_t* entry = syscall_find_entry(syscall_num);
        if (entry == NULL) {
            error = DSRTOS_ERROR_NOT_FOUND;
	}
    }
    return error;
}
