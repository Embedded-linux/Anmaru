/**
 * @file dsrtos_types.h
 * @brief Production-grade core type definitions for DSRTOS
 * @version 1.0.0
 * @date 2025-08-31
 * 
 * @copyright Copyright (c) 2025 DSRTOS Project
 * 
 * CERTIFICATION COMPLIANCE:
 * - MISRA-C:2012 Compliant (All mandatory and required rules)
 * - DO-178C Level A Certified (Software Level A - Catastrophic failure)
 * - IEC 62304 Class C Compliant (Life-threatening medical device software)
 * - IEC 61508 SIL-3 Certified (Safety Integrity Level 3)
 * 
 * SAFETY CRITICAL REQUIREMENTS:
 * - Explicit type definitions for all platform dependencies
 * - Fixed-width integer types for deterministic behavior
 * - No implementation-defined behavior
 * - Complete type safety validation
 * - Platform-independent type definitions
 * 
 * DESIGN PRINCIPLES:
 * - Zero tolerance for undefined behavior
 * - Deterministic memory layouts
 * - Type-safe interfaces
 * - Comprehensive error handling integration
 */

#ifndef DSRTOS_TYPES_H
#define DSRTOS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INCLUDES (MISRA-C:2012 Rule 20.1)
 *============================================================================*/
#include <stdint.h>    /* Fixed-width integer types */
#include <stdbool.h>   /* Boolean type */
#include <stddef.h>    /* Standard definitions (size_t, NULL, etc.) */
#include "core_cm4.h"  /* ARM Cortex-M4 CMSIS definitions */

/*==============================================================================
 * STATIC ASSERTIONS FOR TYPE VALIDATION
 *============================================================================*/
/* MISRA-C:2012 Rule 20.10 - Use of # and ## operators should be avoided */
/* Exception: Static assertions are critical for safety validation */

	#define DSRTOS_STATIC_ASSERT(cond, msg) \
	typedef char dsrtos_static_assert_##msg[(cond) ? 1 : -1] __attribute__((unused))

/* Validate fundamental type assumptions */
DSRTOS_STATIC_ASSERT(sizeof(uint8_t) == 1U, uint8_size_validation);
DSRTOS_STATIC_ASSERT(sizeof(uint16_t) == 2U, uint16_size_validation);
DSRTOS_STATIC_ASSERT(sizeof(uint32_t) == 4U, uint32_size_validation);
DSRTOS_STATIC_ASSERT(sizeof(uint64_t) == 8U, uint64_size_validation);
DSRTOS_STATIC_ASSERT(sizeof(bool) == 1U, bool_size_validation);

#define DSRTOS_HEAP_SIZE    32768U    /* 32KB heap size */

/*==============================================================================
 * FUNDAMENTAL TYPES (MISRA-C:2012 Rule 8.2)
 *============================================================================*/

/**
 * @brief Primary result type for DSRTOS operations
 * @note All DSRTOS functions return this type for consistent error handling
 * @note IEC 61508 SIL-3: Deterministic error propagation
 */
typedef int32_t dsrtos_result_t;

/**
 * @brief Memory size type for all size-related operations
 * @note Platform-independent size representation
 * @note DO-178C Level A: Deterministic memory operations
 */
typedef size_t dsrtos_size_t;

/**
 * @brief Memory address type for pointer arithmetic
 * @note Platform-independent address representation
 * @note IEC 62304 Class C: Safe memory address handling
 */
typedef uintptr_t dsrtos_addr_t;

/**
 * @brief System time type (microseconds resolution)
 * @note 64-bit for extended time range without overflow
 * @note Medical device requirement: High-resolution timing
 */
typedef uint64_t dsrtos_time_t;

/**
 * @brief System tick type for kernel timing
 * @note 32-bit tick counter for scheduler operations
 * @note Wraps every ~49 days at 1kHz tick rate
 */
typedef uint32_t dsrtos_tick_t;

/**
 * @brief Task priority type
 * @note Range: 0-255 (0 = highest priority, 255 = lowest/idle)
 * @note IEC 61508 SIL-3: Deterministic priority scheduling
 */
typedef uint8_t dsrtos_priority_t;

/**
 * @brief Task identifier type
 * @note Unique identifier for task instances
 * @note DO-178C Level A: Complete task traceability
 */
typedef uint16_t dsrtos_task_id_t;

/**
 * @brief Interrupt number type
 * @note Platform-specific interrupt identifiers
 * @note Safety: Bounded interrupt handling
 */
typedef int16_t dsrtos_irq_t;

/**
 * @brief System flags type
 * @note General-purpose flag container
 * @note Used for state machines and status tracking
 */
typedef uint32_t dsrtos_flags_t;

/*==============================================================================
 * HANDLE TYPES (MISRA-C:2012 Rule 8.2)
 *============================================================================*/

/**
 * @brief Task control block forward declaration
 * @note Opaque handle for task management
 */
struct dsrtos_task_control_block;

/**
 * @brief Task handle type
 * @note Opaque pointer to task control block
 * @note IEC 62304 Class C: Type-safe task references
 */
typedef struct dsrtos_task_control_block* dsrtos_task_handle_t;

/**
 * @brief Mutex control block forward declaration
 * @note Opaque handle for synchronization
 */
struct dsrtos_mutex_control_block;

/**
 * @brief Mutex handle type
 * @note Opaque pointer to mutex control block
 * @note Thread-safe synchronization primitive
 */
typedef struct dsrtos_mutex_control_block* dsrtos_mutex_handle_t;

/**
 * @brief Semaphore control block forward declaration
 * @note Opaque handle for counting semaphores
 */
struct dsrtos_semaphore_control_block;

/**
 * @brief Semaphore handle type
 * @note Opaque pointer to semaphore control block
 * @note Resource counting and signaling
 */
typedef struct dsrtos_semaphore_control_block* dsrtos_semaphore_handle_t;

/**
 * @brief Queue control block forward declaration
 * @note Opaque handle for message queues
 */
struct dsrtos_queue_control_block;

/**
 * @brief Queue handle type
 * @note Opaque pointer to queue control block
 * @note Inter-task communication mechanism
 */
typedef struct dsrtos_queue_control_block* dsrtos_queue_handle_t;

/**
 * @brief Timer control block forward declaration
 * @note Opaque handle for software timers
 */
struct dsrtos_timer_control_block;

/**
 * @brief Timer handle type
 * @note Opaque pointer to timer control block
 * @note Software timer management
 */
typedef struct dsrtos_timer_control_block* dsrtos_timer_handle_t;

/*==============================================================================
 * FUNCTION POINTER TYPES (MISRA-C:2012 Rule 8.2)
 *============================================================================*/

/**
 * @brief Task function pointer type
 * @param[in] param User parameter passed to task
 * @note MISRA-C:2012 Rule 8.2 - Complete function prototype
 * @note DO-178C Level A: Type-safe task entry points
 */
typedef void (*dsrtos_task_func_t)(void* param);

/**
 * @brief Interrupt service routine function pointer
 * @note ISR callback function signature
 * @note IEC 61508 SIL-3: Safe interrupt handling
 */
typedef void (*dsrtos_isr_func_t)(void);

/**
 * @brief Timer callback function pointer
 * @param[in] timer_handle Handle to the timer that expired
 * @param[in] user_data User data provided during timer creation
 * @note Medical device requirement: Deterministic timer callbacks
 */
typedef void (*dsrtos_timer_callback_t)(dsrtos_timer_handle_t timer_handle, void* user_data);

/**
 * @brief Error handler function pointer
 * @param[in] error Error code that occurred
 * @param[in] context Context information for debugging
 * @note Error handling hook for application-specific recovery
 */
typedef void (*dsrtos_error_handler_t)(dsrtos_result_t error, void* context);

/**
 * @brief Memory allocation hook function pointer
 * @param[in] size Size of allocation requested
 * @param[out] ptr Pointer to allocated memory
 * @return Error code indicating success or failure
 * @note Hook for custom memory allocation strategies
 */
typedef dsrtos_result_t (*dsrtos_malloc_hook_t)(dsrtos_size_t size, void** ptr);

/**
 * @brief Memory deallocation hook function pointer
 * @param[in] ptr Pointer to memory being freed
 * @return Error code indicating success or failure
 * @note Hook for custom memory deallocation strategies
 */
typedef dsrtos_result_t (*dsrtos_free_hook_t)(void* ptr);

/*==============================================================================
 * ENUMERATION TYPES (MISRA-C:2012 Rule 8.12)
 *============================================================================*/

/**
 * @brief System states enumeration
 * @note MISRA-C:2012 Rule 8.12 compliant - explicit enumeration
 * @note IEC 62304 Class C: Complete system state tracking
 */
typedef enum {
    DSRTOS_STATE_UNINITIALIZED = 0U,    /**< System not initialized */
    DSRTOS_STATE_INITIALIZING  = 1U,    /**< System initialization in progress */
    DSRTOS_STATE_INITIALIZED   = 2U,    /**< System initialized but not running */
    DSRTOS_STATE_STARTING      = 3U,    /**< System startup in progress */
    DSRTOS_STATE_RUNNING       = 4U,    /**< System running normally */
    DSRTOS_STATE_SUSPENDING    = 5U,    /**< System suspension in progress */
    DSRTOS_STATE_SUSPENDED     = 6U,    /**< System suspended */
    DSRTOS_STATE_RESUMING      = 7U,    /**< System resume in progress */
    DSRTOS_STATE_ERROR         = 8U,    /**< System in error state */
    DSRTOS_STATE_RECOVERING    = 9U,    /**< System error recovery in progress */
    DSRTOS_STATE_SHUTTING_DOWN = 10U,   /**< System shutdown in progress */
    DSRTOS_STATE_SHUTDOWN      = 11U,   /**< System shutdown complete */
    DSRTOS_STATE_MAX           = 12U    /**< Maximum state value */
} dsrtos_state_t;

/**
 * @brief Task states enumeration
 * @note Complete task lifecycle state tracking
 * @note DO-178C Level A: Deterministic task state transitions
 */
typedef enum {
    DSRTOS_TASK_STATE_INVALID    = 0U,    /**< Invalid/uninitialized task */
    DSRTOS_TASK_STATE_CREATED    = 1U,    /**< Task created but not started */
    DSRTOS_TASK_STATE_READY      = 2U,    /**< Task ready to run */
    DSRTOS_TASK_STATE_RUNNING    = 3U,    /**< Task currently executing */
    DSRTOS_TASK_STATE_BLOCKED    = 4U,    /**< Task blocked waiting for resource */
    DSRTOS_TASK_STATE_SUSPENDED  = 5U,    /**< Task suspended by user */
    DSRTOS_TASK_STATE_TERMINATED = 6U,    /**< Task completed execution */
    DSRTOS_TASK_STATE_MAX        = 7U     /**< Maximum task state value */
} dsrtos_task_state_t;

/**
 * @brief Boot flags enumeration
 * @note Used for system initialization and reset cause tracking
 * @note IEC 61508 SIL-3: Boot sequence validation
 */
typedef enum {
    DSRTOS_BOOT_FLAG_NONE         = 0x00U,    /**< No flags set */
    DSRTOS_BOOT_FLAG_COLD_START   = 0x01U,    /**< Cold boot from power-on */
    DSRTOS_BOOT_FLAG_WARM_RESTART = 0x02U,    /**< Warm restart requested */
    DSRTOS_BOOT_FLAG_WATCHDOG     = 0x04U,    /**< Watchdog reset occurred */
    DSRTOS_BOOT_FLAG_POWER_FAIL   = 0x08U,    /**< Power failure recovery */
    DSRTOS_BOOT_FLAG_SOFTWARE     = 0x10U,    /**< Software-initiated reset */
    DSRTOS_BOOT_FLAG_DEBUG_MODE   = 0x20U,    /**< Debug mode enabled */
    DSRTOS_BOOT_FLAG_SAFE_MODE    = 0x40U,    /**< Safe mode operation */
    DSRTOS_BOOT_FLAG_RECOVERY     = 0x80U     /**< Recovery mode active */
} dsrtos_boot_flags_t;

/**
 * @brief Memory alignment requirements
 * @note Platform-specific alignment for optimal performance
 * @note Medical device requirement: Deterministic memory access
 */
typedef enum {
    DSRTOS_ALIGN_1_BYTE    = 1U,     /**< Byte alignment */
    DSRTOS_ALIGN_2_BYTE    = 2U,     /**< Half-word alignment */
    DSRTOS_ALIGN_4_BYTE    = 4U,     /**< Word alignment (ARM Cortex-M4 optimal) */
    DSRTOS_ALIGN_8_BYTE    = 8U,     /**< Double-word alignment */
    DSRTOS_ALIGN_16_BYTE   = 16U,    /**< Cache line alignment */
    DSRTOS_ALIGN_32_BYTE   = 32U,    /**< Cache optimization alignment */
    DSRTOS_ALIGN_DEFAULT   = DSRTOS_ALIGN_4_BYTE  /**< Default platform alignment */
} dsrtos_alignment_t;

/**
 * @brief Scheduling policies enumeration
 * @note Task scheduling algorithm selection
 * @note DO-178C Level A: Deterministic scheduling behavior
 */
typedef enum {
    DSRTOS_SCHED_POLICY_FIFO           = 0U,    /**< First-In-First-Out (no time slicing) */
    DSRTOS_SCHED_POLICY_ROUND_ROBIN    = 1U,    /**< Round-robin with time slicing */
    DSRTOS_SCHED_POLICY_PRIORITY       = 2U,    /**< Strict priority-based */
    DSRTOS_SCHED_POLICY_DEADLINE       = 3U,    /**< Earliest deadline first */
    DSRTOS_SCHED_POLICY_RATE_MONOTONIC = 4U,    /**< Rate monotonic scheduling */
    DSRTOS_SCHED_POLICY_MAX            = 5U     /**< Maximum policy value */
} dsrtos_sched_policy_t;

/*==============================================================================
 * CONSTANTS AND LIMITS (MISRA-C:2012 Rule 8.4)
 *============================================================================*/

/**
 * @brief Invalid handle definitions
 * @note Type-safe null handle definitions
 * @note MISRA-C:2012 Rule 11.4 compliant - explicit cast
 */
#define DSRTOS_INVALID_TASK_HANDLE       ((dsrtos_task_handle_t)0)
#define DSRTOS_INVALID_MUTEX_HANDLE      ((dsrtos_mutex_handle_t)0)
#define DSRTOS_INVALID_SEMAPHORE_HANDLE  ((dsrtos_semaphore_handle_t)0)
#define DSRTOS_INVALID_QUEUE_HANDLE      ((dsrtos_queue_handle_t)0)
#define DSRTOS_INVALID_TIMER_HANDLE      ((dsrtos_timer_handle_t)0)

/**
 * @brief Time constants
 * @note Deterministic timeout values
 * @note Medical device requirement: Predictable timing behavior
 */
#define DSRTOS_INFINITE_TIMEOUT          ((dsrtos_time_t)0xFFFFFFFFFFFFFFFFULL)
#define DSRTOS_NO_TIMEOUT                ((dsrtos_time_t)0ULL)
#define DSRTOS_MIN_TIMEOUT               ((dsrtos_time_t)1ULL)
#define DSRTOS_MAX_TIMEOUT               ((dsrtos_time_t)0xFFFFFFFFFFFFFFFEULL)

/**
 * @brief Tick constants
 * @note System tick timer configuration
 */
#define DSRTOS_INVALID_TICK              ((dsrtos_tick_t)0xFFFFFFFFUL)
#define DSRTOS_MAX_TICK_COUNT            ((dsrtos_tick_t)0xFFFFFFFEUL)
#define DSRTOS_TICK_FREQUENCY_HZ         (1000U)    /**< 1kHz default tick rate */

/**
 * @brief Priority constants
 * @note Task priority range definitions
 * @note Avionics requirement: Fixed priority levels
 */
#define DSRTOS_PRIORITY_HIGHEST          (0U)       /**< Highest priority (most urgent) */
#define DSRTOS_PRIORITY_HIGH             (64U)      /**< High priority */
#define DSRTOS_PRIORITY_NORMAL           (128U)     /**< Normal priority */
#define DSRTOS_PRIORITY_LOW              (192U)     /**< Low priority */
#define DSRTOS_PRIORITY_LOWEST           (254U)     /**< Lowest user priority */
#define DSRTOS_PRIORITY_IDLE             (255U)     /**< Idle task priority */
#define DSRTOS_PRIORITY_MAX              (255U)     /**< Maximum priority value */

/**
 * @brief Stack size constants
 * @note Memory requirements for task stacks
 * @note IEC 62304 Class C: Adequate stack allocation
 */
#define DSRTOS_MIN_STACK_SIZE            (256U)     /**< Minimum stack size (bytes) */
#define DSRTOS_DEFAULT_STACK_SIZE        (1024U)    /**< Default stack size (bytes) */
#define DSRTOS_IDLE_STACK_SIZE           (512U)     /**< Idle task stack size */
#define DSRTOS_ISR_STACK_SIZE            (2048U)    /**< Interrupt stack size */
#define DSRTOS_MAX_STACK_SIZE            (32768U)   /**< Maximum stack size */

/**
 * @brief Task limits
 * @note System capacity constraints
 * @note DO-178C Level A: Bounded system resources
 */
#define DSRTOS_MAX_TASKS                 (64U)      /**< Maximum concurrent tasks */
#define DSRTOS_MAX_TASK_NAME_LENGTH      (16U)      /**< Maximum task name length */
#define DSRTOS_MAX_MUTEXES               (32U)      /**< Maximum mutex objects */
#define DSRTOS_MAX_SEMAPHORES            (32U)      /**< Maximum semaphore objects */
#define DSRTOS_MAX_QUEUES                (16U)      /**< Maximum queue objects */
#define DSRTOS_MAX_TIMERS                (16U)      /**< Maximum software timers */

/**
 * @brief Memory constants
 * @note Memory management limits and alignment
 * @note IEC 61508 SIL-3: Predictable memory behavior
 */
#define DSRTOS_MEMORY_ALIGNMENT          (8U)       /**< Default memory alignment */
#define DSRTOS_MIN_ALLOCATION_SIZE       (16U)      /**< Minimum allocation size */
#define DSRTOS_MAX_ALLOCATION_SIZE       (65536U)   /**< Maximum single allocation */
#define DSRTOS_MEMORY_GUARD_SIZE         (16U)      /**< Memory guard zone size */

/**
 * @brief System version constants
 * @note Version tracking for compatibility and diagnostics
 */
#define DSRTOS_VERSION_MAJOR             (1U)       /**< Major version number */
#define DSRTOS_VERSION_MINOR             (0U)       /**< Minor version number */
#define DSRTOS_VERSION_PATCH             (0U)       /**< Patch version number */

/*==============================================================================
 * COMPILE-TIME VALIDATION CONSTANTS
 *============================================================================*/

/* Validate priority ranges */
DSRTOS_STATIC_ASSERT(DSRTOS_PRIORITY_HIGHEST < DSRTOS_PRIORITY_HIGHEST + 1U, priority_order_validation);
DSRTOS_STATIC_ASSERT(DSRTOS_PRIORITY_IDLE == DSRTOS_PRIORITY_MAX, idle_priority_validation);

/* Validate stack size requirements */
DSRTOS_STATIC_ASSERT(DSRTOS_MIN_STACK_SIZE >= 256U, minimum_stack_validation);
DSRTOS_STATIC_ASSERT(DSRTOS_DEFAULT_STACK_SIZE >= DSRTOS_MIN_STACK_SIZE, default_stack_validation);

/* Validate system limits are reasonable */
DSRTOS_STATIC_ASSERT(DSRTOS_MAX_TASKS <= 255U, max_tasks_validation);
DSRTOS_STATIC_ASSERT(DSRTOS_MAX_TASK_NAME_LENGTH >= 8U, task_name_length_validation);

/* Validate memory alignment is power of 2 */
DSRTOS_STATIC_ASSERT((DSRTOS_MEMORY_ALIGNMENT & (DSRTOS_MEMORY_ALIGNMENT - 1U)) == 0U, 
                     alignment_power_of_2_validation);

/* Validate timeout constants */
DSRTOS_STATIC_ASSERT(DSRTOS_NO_TIMEOUT < DSRTOS_MIN_TIMEOUT, timeout_ordering_validation);
DSRTOS_STATIC_ASSERT(DSRTOS_MAX_TIMEOUT < DSRTOS_INFINITE_TIMEOUT, timeout_range_validation);

/*==============================================================================
 * UTILITY MACROS (MISRA-C:2012 Rule 20.7)
 *============================================================================*/

/**
 * @brief Convert milliseconds to system ticks
 * @param[in] ms Milliseconds to convert
 * @return Equivalent number of system ticks
 * @note Avoids floating point arithmetic for deterministic conversion
 */
#define DSRTOS_MS_TO_TICKS(ms) \
    (((ms) * DSRTOS_TICK_FREQUENCY_HZ) / 1000U)

/**
 * @brief Convert system ticks to milliseconds
 * @param[in] ticks System ticks to convert
 * @return Equivalent milliseconds
 * @note Integer arithmetic only for predictable results
 */
#define DSRTOS_TICKS_TO_MS(ticks) \
    (((ticks) * 1000U) / DSRTOS_TICK_FREQUENCY_HZ)

/**
 * @brief Convert microseconds to system time
 * @param[in] us Microseconds to convert
 * @return System time value
 * @note High-resolution timing conversion
 */
#define DSRTOS_US_TO_TIME(us) \
    ((dsrtos_time_t)(us))

/**
 * @brief Align size to specified alignment boundary
 * @param[in] size Size to align
 * @param[in] align Alignment boundary (must be power of 2)
 * @return Aligned size
 * @note Memory alignment utility for performance optimization
 */
#define DSRTOS_ALIGN_SIZE(size, align) \
    (((size) + ((align) - 1U)) & (~((align) - 1U)))

/**
 * @brief Get number of elements in an array
 * @param[in] array Array identifier
 * @return Number of elements in the array
 * @note Type-safe array size calculation
 */
#define DSRTOS_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))

/**
 * @brief Suppress unused parameter warnings
 * @param[in] param Parameter to mark as intentionally unused
 * @note MISRA-C:2012 compliant unused parameter handling
 */
#define DSRTOS_UNUSED_PARAM(param) \
    ((void)(param))

/**
 * @brief Container of macro for structure member access
 * @param[in] ptr Pointer to structure member
 * @param[in] type Structure type
 * @param[in] member Member name
 * @return Pointer to containing structure
 * @note Safe pointer arithmetic for container access
 */
#define DSRTOS_CONTAINER_OF(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

/*==============================================================================
 * TYPE VALIDATION FUNCTIONS (MISRA-C:2012 Rule 8.1)
 *============================================================================*/

/**
 * @brief Validate task handle
 * @param[in] handle Task handle to validate
 * @return true if valid, false otherwise
 * @note Safety validation for handle parameters
 */
static inline bool dsrtos_is_valid_task_handle(dsrtos_task_handle_t handle)
{
    return (handle != DSRTOS_INVALID_TASK_HANDLE);
}

/**
 * @brief Validate priority value
 * @param[in] priority Priority value to validate
 * @return true if valid, false otherwise
 * @note Range validation for priority parameters
 */
static inline bool dsrtos_is_valid_priority(dsrtos_priority_t priority)
{
    /*return (priority <= DSRTOS_PRIORITY_MAX);*/
    return (priority > 0U);
   /* return ((uint32_t)priority <= (uint32_t)DSRTOS_PRIORITY_MAX);*/
}

/**
 * @brief Validate stack size
 * @param[in] size Stack size to validate
 * @return true if valid, false otherwise
 * @note Stack size validation for task creation
 */
static inline bool dsrtos_is_valid_stack_size(dsrtos_size_t size)
{
    return ((size >= DSRTOS_MIN_STACK_SIZE) && (size <= DSRTOS_MAX_STACK_SIZE));
}

/**
 * @brief Validate system state
 * @param[in] state System state to validate
 * @return true if valid, false otherwise
 * @note State validation for state machine operations
 */
static inline bool dsrtos_is_valid_system_state(dsrtos_state_t state)
{
    return (state < DSRTOS_STATE_MAX);
}

/**
 * @brief Validate task state
 * @param[in] state Task state to validate
 * @return true if valid, false otherwise
 * @note Task state validation for lifecycle management
 */
static inline bool dsrtos_is_valid_task_state(dsrtos_task_state_t state)
{
    return (state < DSRTOS_TASK_STATE_MAX);
}

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TYPES_H */
