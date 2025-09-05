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

/* dsrtos_error_t is defined in common/dsrtos_error.h */

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
 * TASK FLAGS
 *============================================================================*/

#define DSRTOS_TASK_FLAG_REAL_TIME          (0x01U)
#define DSRTOS_TASK_FLAG_PRIORITY_INHERIT   (0x02U)
#define DSRTOS_TASK_FLAG_STACK_CHECK        (0x04U)

/*==============================================================================
 * STACK CONSTANTS
 *============================================================================*/

#define DSRTOS_STACK_PATTERN                (0xA5A5A5A5U)
#define DSRTOS_STACK_CANARY_VALUE           (0xDEADBEEFU)

/* Task limits */
#define DSRTOS_MAX_TASKS                    (32U)
#define DSRTOS_MAX_PRIORITY                 (31U)

/* Error constants */
#define DSRTOS_ERROR_NOT_ENABLED            (-13)
#define DSRTOS_INVALID_PARAM                (-1)
#define DSRTOS_NOT_FOUND                    (-16)

/*==============================================================================
 * TASK STATES
 *============================================================================*/

typedef enum {
    DSRTOS_TASK_STATE_INVALID    = 0U,
    DSRTOS_TASK_STATE_CREATED    = 1U,
    DSRTOS_TASK_STATE_READY      = 2U,
    DSRTOS_TASK_STATE_RUNNING    = 3U,
    DSRTOS_TASK_STATE_BLOCKED    = 4U,
    DSRTOS_TASK_STATE_SUSPENDED  = 5U,
    DSRTOS_TASK_STATE_TERMINATED = 6U,
    DSRTOS_TASK_STATE_DORMANT    = 7U,
    DSRTOS_TASK_STATE_MAX        = 8U
} dsrtos_task_state_t;

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TYPES_H */
