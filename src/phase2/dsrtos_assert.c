/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Assertion Framework Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_assert.h"
#include "dsrtos_panic.h"
#include "dsrtos_critical.h"
#include <stdio.h>
#include "core_cm4.h"

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
static struct {
    dsrtos_assert_handler_t custom_handler;
    uint32_t failure_count;
    bool initialized;
} g_assert_state = {
    .custom_handler = NULL,
    .failure_count = 0U,
    .initialized = false
};

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static void assert_default_handler(
    const char* expr,
    const char* file,
    uint32_t line,
    const char* func,
    dsrtos_assert_type_t type
);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize assertion framework
 */
dsrtos_error_t dsrtos_assert_init(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (!g_assert_state.initialized) {
        g_assert_state.custom_handler = NULL;
        g_assert_state.failure_count = 0U;
        g_assert_state.initialized = true;
    }
    
    return result;
}

/**
 * @brief Set custom assertion handler
 */
dsrtos_assert_handler_t dsrtos_assert_set_handler(dsrtos_assert_handler_t handler)
{
    dsrtos_assert_handler_t previous = g_assert_state.custom_handler;
    g_assert_state.custom_handler = handler;
    return previous;
}

/**
 * @brief Assert failure handler
 */

#if 0

void dsrtos_assert_failed(
    const char* expr,
    const char* file,
    uint32_t line,
    const char* func,
    dsrtos_assert_type_t type)

#endif

void dsrtos_assert_failed(const char* expr, const char* file, int line, const char* func)
{
    /* Disable interrupts */
    __disable_irq();
    
    /* Increment failure count */
    g_assert_state.failure_count++;
    dsrtos_assert_type_t type = DSRTOS_ASSERT_TYPE_PRECONDITION;
    
    /* Call custom handler if registered */
    if (g_assert_state.custom_handler != NULL) {
       g_assert_state.custom_handler(expr, file, (uint32_t)line, func, type);
    }
    else {
        assert_default_handler(expr, file, (uint32_t)line, func, type);
    }
    
    /* Trigger panic */
    dsrtos_panic(DSRTOS_PANIC_ASSERTION_FAILED, expr, file, (uint32_t)line);
    
    /* Should never reach here */
    while (1) {
        __NOP();
    }
}

/**
 * @brief Get assertion statistics
 */
dsrtos_error_t dsrtos_assert_get_stats(uint32_t* count)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (count == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else {
        *count = g_assert_state.failure_count;
    }
    
    return result;
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Default assertion handler
 */
static void assert_default_handler(
    const char* expr,
    const char* file,
    uint32_t line,
    const char* func,
    dsrtos_assert_type_t type)
{
    /* Format assertion message */
    static char assert_msg[256];
    const char* type_str;
    
    switch (type) {
        case DSRTOS_ASSERT_TYPE_PRECONDITION:
            type_str = "PRECONDITION";
            break;
        case DSRTOS_ASSERT_TYPE_POSTCONDITION:
            type_str = "POSTCONDITION";
            break;
        case DSRTOS_ASSERT_TYPE_INVARIANT:
            type_str = "INVARIANT";
            break;
        case DSRTOS_ASSERT_TYPE_STATE:
            type_str = "STATE";
            break;
        case DSRTOS_ASSERT_TYPE_PARAMETER:
            type_str = "PARAMETER";
            break;
        case DSRTOS_ASSERT_TYPE_BOUNDS:
            type_str = "BOUNDS";
            break;
        case DSRTOS_ASSERT_TYPE_NULL_CHECK:
            type_str = "NULL_CHECK";
            break;
        case DSRTOS_ASSERT_TYPE_ALIGNMENT:
            type_str = "ALIGNMENT";
            break;
        case DSRTOS_ASSERT_TYPE_MAGIC:
            type_str = "MAGIC";
            break;
	case DSRTOS_ASSERT_TYPE_CUSTOM:
        default:
            type_str = "CUSTOM";
            break;
    }
    
    snprintf(assert_msg, sizeof(assert_msg),
             "ASSERTION FAILED [%s]: %s\nFile: %s:%lu\nFunction: %s",
             type_str, expr, file, line, func);
    
    /* Store for debugger inspection */
    (void)assert_msg;
}
