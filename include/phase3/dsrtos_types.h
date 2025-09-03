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

/* Include common types first to get DSRTOS_STATIC_ASSERT macro */
#include "../common/dsrtos_types.h"

/*==============================================================================
 * ERROR CODES
 *============================================================================*/

/* Use common error definitions from dsrtos_error.h */
#include "../common/dsrtos_error.h"

/* Additional error codes needed by Phase3 */
#ifndef DSRTOS_ERROR_LOW_RESOURCE
#define DSRTOS_ERROR_LOW_RESOURCE            DSRTOS_ERROR_NO_RESOURCES
#endif

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

/*==============================================================================
 * PHASE3 SPECIFIC CONSTANTS
 *============================================================================*/

/* Task management constants */
#define DSRTOS_MAX_TASKS                    (32U)
#define DSRTOS_MIN_STACK_SIZE               (128U)
#define DSRTOS_MAX_STACK_SIZE               (4096U)
#define DSRTOS_STACK_ALIGNMENT              (8U)
#define DSRTOS_MAX_PRIORITY                 (31U)
#define DSRTOS_DEFAULT_TIME_SLICE           (10U)

/* Task flags */
#define DSRTOS_TASK_FLAG_REAL_TIME          (0x01U)
#define DSRTOS_TASK_FLAG_NO_DELETE          (0x02U)

/* Stack patterns */
#define DSRTOS_STACK_PATTERN                (0xA5A5A5A5U)
#define DSRTOS_STACK_CANARY_VALUE           (0xDEADBEEFU)

/* Error codes */
#define DSRTOS_ERROR_NOT_PERMITTED          (-6)
#define DSRTOS_ERROR_LIMIT_EXCEEDED         (-12)

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TYPES_H */
