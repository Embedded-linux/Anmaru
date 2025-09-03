/*
 * @file dsrtos_types.h
 * @brief DSRTOS Common Type Definitions
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_TYPES_H
#define DSRTOS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*==============================================================================
 * ERROR CODES
 *============================================================================*/

typedef enum {
    DSRTOS_SUCCESS                = 0,
    DSRTOS_ERROR_INVALID_PARAM    = -1,
    DSRTOS_ERROR_NO_MEMORY        = -2,
    DSRTOS_ERROR_NO_RESOURCE      = -3,
    DSRTOS_ERROR_INVALID_STATE    = -4,
    DSRTOS_ERROR_TIMEOUT          = -5,
    DSRTOS_ERROR_NOT_PERMITTED    = -6,
    DSRTOS_ERROR_NOT_INITIALIZED  = -7,
    DSRTOS_ERROR_ALREADY_INITIALIZED = -8,
    DSRTOS_ERROR_ALREADY_STARTED  = -9,
    DSRTOS_ERROR_CORRUPTED        = -10,
    DSRTOS_ERROR_STACK_OVERFLOW   = -11,
    DSRTOS_ERROR_LIMIT_EXCEEDED   = -12,
    DSRTOS_ERROR_NOT_ENABLED      = -13,
    DSRTOS_ERROR_LOW_RESOURCE     = -14,
    DSRTOS_ERROR_INTERNAL         = -15
} dsrtos_error_t;

/*==============================================================================
 * SYSTEM TYPES
 *============================================================================*/

/* Time types */
typedef uint64_t dsrtos_time_t;
typedef uint32_t dsrtos_tick_t;

/* ID types */
typedef uint32_t dsrtos_task_id_t;
typedef uint32_t dsrtos_mutex_id_t;
typedef uint32_t dsrtos_sem_id_t;
typedef uint32_t dsrtos_queue_id_t;

/* Handle types */
typedef void* dsrtos_handle_t;

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TYPES_H */
