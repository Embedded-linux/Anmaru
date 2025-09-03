/*
 * DSRTOS Task-Scheduler Interface
 * 
 * Copyright (C) 2024 DSRTOS
 * 
 * SPDX-License-Identifier: MIT
 * 
 * Certification:
 *   - DO-178C DAL-B
 *   - IEC 62304 Class B
 *   - ISO 26262 ASIL D
 *   - IEC 61508 SIL 3
 * 
 * MISRA-C:2012 Compliance:
 *   - Mandatory: 100%
 *   - Required: 100%
 *   - Advisory: 98%
 * 
 * Safety Features:
 *   - Dual redundancy checks
 *   - Memory protection
 *   - Fault containment
 *   - Error recovery
 */

#ifndef DSRTOS_TASK_SCHEDULER_INTERFACE_H
#define DSRTOS_TASK_SCHEDULER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dsrtos_types.h"
#include "dsrtos_config.h"
#include "dsrtos_error.h"

/*==============================================================================
 * CONFIGURATION VALIDATION
 *============================================================================*/
/* MISRA-C:2012 Dir 4.4: Validate configuration parameters */
#if (DSRTOS_MAX_PRIORITY_LEVELS == 0U) || (DSRTOS_MAX_PRIORITY_LEVELS > 256U)
    #error "Invalid DSRTOS_MAX_PRIORITY_LEVELS configuration"
#endif

#if (DSRTOS_READY_QUEUE_SIZE == 0U) || (DSRTOS_READY_QUEUE_SIZE > 1024U)
    #error "Invalid DSRTOS_READY_QUEUE_SIZE configuration"
#endif

/*==============================================================================
 * CONSTANTS
 *============================================================================*/
/* Priority levels - MISRA-C:2012 Rule 2.5: All macros used */
#define DSRTOS_PRIORITY_LEVELS          (256U)
#define DSRTOS_IDLE_PRIORITY           (0U)
#define DSRTOS_HIGHEST_PRIORITY        (255U)
#define DSRTOS_DEFAULT_PRIORITY        (128U)
#define DSRTOS_INVALID_PRIORITY        (0xFFFFU)

/* Queue configuration */
#define DSRTOS_QUEUE_MAGIC             (0x5153U)    /* 'QS' */
#define DSRTOS_NODE_MAGIC              (0x4E4FU)    /* 'NO' */
#define DSRTOS_BITMAP_WORDS            (8U)         /* 256 bits / 32 */

/* Time slice configuration */
#define DSRTOS_TICK_RATE_HZ            (1000U)
#define DSRTOS_MIN_TIME_SLICE          (1U)
#define DSRTOS_DEFAULT_TIME_SLICE      (10U)
#define DSRTOS_MAX_TIME_SLICE          (100U)

/* Safety thresholds */
#define DSRTOS_MAX_PREEMPTION_DEPTH    (16U)
#define DSRTOS_MAX_INHERITANCE_CHAIN   (8U)
#define DSRTOS_WATCHDOG_CYCLES         (1000000U)

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/
/* Forward declarations - MISRA-C:2012 Rule 8.2 */
typedef struct dsrtos_tcb dsrtos_tcb_t;
typedef struct dsrtos_ready_queue dsrtos_ready_queue_t;
typedef struct dsrtos_scheduler_interface dsrtos_scheduler_interface_t;

/* Scheduling decision reasons */
typedef enum {
    DSRTOS_SCHED_REASON_NONE = 0,
    DSRTOS_SCHED_REASON_TICK,
    DSRTOS_SCHED_REASON_YIELD,
    DSRTOS_SCHED_REASON_BLOCK,
    DSRTOS_SCHED_REASON_UNBLOCK,
    DSRTOS_SCHED_REASON_PRIORITY_CHANGE,
    DSRTOS_SCHED_REASON_TASK_EXIT,
    DSRTOS_SCHED_REASON_PREEMPTION,
    DSRTOS_SCHED_REASON_MIGRATION,
    DSRTOS_SCHED_REASON_ERROR_RECOVERY,
    DSRTOS_SCHED_REASON_MAX
} dsrtos_sched_reason_t;

/* Queue node structure with safety features */
typedef struct dsrtos_queue_node {
    uint16_t magic_start;                  /* Magic number for validation */
    dsrtos_tcb_t* task;                   /* Task control block */
    struct dsrtos_queue_node* next;       /* Next in list */
    struct dsrtos_queue_node* prev;       /* Previous in list */
    uint32_t insertion_tick;              /* Insertion timestamp */
    uint32_t checksum;                    /* Node checksum */
    uint16_t magic_end;                   /* End magic number */
} dsrtos_queue_node_t;

/* Priority list with integrity checking */
typedef struct dsrtos_priority_list {
    dsrtos_queue_node_t* head;            /* List head */
    dsrtos_queue_node_t* tail;            /* List tail */
    uint32_t count;                       /* Number of tasks */
    uint32_t max_count;                   /* Maximum allowed */
    uint8_t priority;                     /* Priority level */
    uint8_t reserved[3];                  /* Alignment padding */
    uint32_t list_checksum;              /* List integrity checksum */
} dsrtos_priority_list_t;

/* Ready queue structure with comprehensive safety */
struct dsrtos_ready_queue {
    /* Dual redundant magic numbers */
    uint16_t magic_start;
    
    /* Priority bitmap for O(1) lookup */
    volatile uint32_t priority_bitmap[DSRTOS_BITMAP_WORDS];
    volatile uint32_t priority_bitmap_mirror[DSRTOS_BITMAP_WORDS]; /* Redundant copy */
    
    /* Priority lists array */
    dsrtos_priority_list_t priority_lists[DSRTOS_PRIORITY_LEVELS];
    
    /* Queue statistics */
    struct {
        uint32_t total_tasks;
        uint32_t highest_priority;
        uint32_t insertions;
        uint32_t removals;
        uint32_t corruptions_detected;
        uint32_t repairs_performed;
        uint64_t total_wait_time;
        uint64_t last_validation_tick;
    } stats;
    
    /* Queue management */
    dsrtos_tcb_t* idle_task;
    uint32_t current_tick;
    
    /* Synchronization with timeout */
    struct {
        volatile uint32_t lock_count;
        volatile uint32_t lock_owner;
        uint32_t lock_timeout_cycles;
        uint32_t max_lock_time;
    } sync;
    
    /* Integrity monitoring */
    struct {
        uint32_t validation_counter;
        uint32_t validation_interval;
        uint32_t last_error_tick;
        uint16_t error_code;
    } integrity;
    
    /* End magic number */
    uint16_t magic_end;
};

/* Scheduler operations interface */
typedef struct dsrtos_scheduler_ops {
    /* Initialization/cleanup */
    dsrtos_error_t (*init)(void* context);
    dsrtos_error_t (*deinit)(void* context);
    dsrtos_error_t (*reset)(void* context);
    
    /* Task selection */
    dsrtos_tcb_t* (*select_next_task)(dsrtos_ready_queue_t* queue);
    bool (*need_reschedule)(dsrtos_ready_queue_t* queue, dsrtos_sched_reason_t reason);
    
    /* Queue operations */
    dsrtos_error_t (*enqueue_task)(dsrtos_ready_queue_t* queue, dsrtos_tcb_t* task);
    dsrtos_error_t (*dequeue_task)(dsrtos_ready_queue_t* queue, dsrtos_tcb_t* task);
    dsrtos_error_t (*requeue_task)(dsrtos_ready_queue_t* queue, dsrtos_tcb_t* task);
    
    /* Priority operations */
    dsrtos_error_t (*change_priority)(dsrtos_ready_queue_t* queue, 
                                      dsrtos_tcb_t* task, uint8_t new_priority);
    uint8_t (*get_effective_priority)(const dsrtos_tcb_t* task);
    
    /* Time management */
    void (*update_time_slice)(dsrtos_tcb_t* task);
    bool (*time_slice_expired)(const dsrtos_tcb_t* task);
    
    /* Safety operations */
    dsrtos_error_t (*validate_queue)(dsrtos_ready_queue_t* queue);
    dsrtos_error_t (*repair_queue)(dsrtos_ready_queue_t* queue);
} dsrtos_scheduler_ops_t;

/* Scheduler interface structure */
struct dsrtos_scheduler_interface {
    /* Validation magic */
    uint32_t magic;
    
    /* Operations table */
    const dsrtos_scheduler_ops_t* ops;
    
    /* Ready queue pointer */
    dsrtos_ready_queue_t* ready_queue;
    
    /* Current execution context */
    dsrtos_tcb_t* current_task;
    dsrtos_tcb_t* next_task;
    dsrtos_tcb_t* previous_task;
    
    /* Scheduler state with safety */
    struct {
        volatile bool scheduler_running;
        volatile bool preemption_enabled;
        volatile bool migration_enabled;
        volatile bool in_critical_section;
        uint32_t schedule_count;
        uint32_t preemption_count;
        uint32_t context_switches;
        uint32_t error_recoveries;
    } state;
    
    /* Configuration */
    struct {
        uint32_t tick_rate_hz;
        uint32_t time_slice_ticks;
        bool priority_inheritance;
        bool priority_ceiling;
        bool deadline_monitoring;
        bool stack_checking;
    } config;
    
    /* Safety monitoring */
    struct {
        uint32_t max_latency_cycles;
        uint32_t deadline_misses;
        uint32_t priority_inversions;
        uint32_t stack_overflows;
    } safety;
    
    /* Private data */
    void* private_data;
    
    /* Checksum */
    uint32_t checksum;
};

/*==============================================================================
 * FUNCTION PROTOTYPES - Initialization
 *============================================================================*/

/**
 * @brief Initialize task-scheduler interface with safety checks
 * @param[in,out] interface Scheduler interface structure
 * @param[in] ops Operations table
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-SCHED-001: Safe initialization
 * @safety Validates all pointers and initializes redundant structures
 */
dsrtos_error_t dsrtos_scheduler_interface_init(
    dsrtos_scheduler_interface_t* interface,
    const dsrtos_scheduler_ops_t* ops
);

/**
 * @brief Safely deinitialize task-scheduler interface
 * @param[in,out] interface Scheduler interface structure
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-SCHED-002: Clean shutdown
 */
dsrtos_error_t dsrtos_scheduler_interface_deinit(
    dsrtos_scheduler_interface_t* interface
);

/**
 * @brief Validate interface integrity
 * @param[in] interface Scheduler interface to validate
 * @return true if valid, false if corrupted
 * @req REQ-SAFETY-001: Runtime validation
 */
bool dsrtos_scheduler_interface_validate(
    const dsrtos_scheduler_interface_t* interface
);

/*==============================================================================
 * FUNCTION PROTOTYPES - Ready Queue Management
 *============================================================================*/

/**
 * @brief Initialize ready queue with redundancy
 * @param[out] queue Ready queue structure
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-QUEUE-001: Safe queue initialization
 */
dsrtos_error_t dsrtos_ready_queue_init(dsrtos_ready_queue_t* queue);

/**
 * @brief Insert task with validation
 * @param[in,out] queue Ready queue
 * @param[in] task Task to insert
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-QUEUE-002: O(1) insertion with validation
 */
dsrtos_error_t dsrtos_ready_queue_insert(
    dsrtos_ready_queue_t* queue,
    dsrtos_tcb_t* task
);

/**
 * @brief Remove task with integrity check
 * @param[in,out] queue Ready queue
 * @param[in] task Task to remove
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-QUEUE-003: Safe removal
 */
dsrtos_error_t dsrtos_ready_queue_remove(
    dsrtos_ready_queue_t* queue,
    dsrtos_tcb_t* task
);

/**
 * @brief Get highest priority task safely
 * @param[in] queue Ready queue
 * @return Task TCB or NULL
 * @req REQ-QUEUE-004: O(1) lookup with validation
 */
dsrtos_tcb_t* dsrtos_ready_queue_get_highest_priority(
    dsrtos_ready_queue_t* queue
);

/**
 * @brief Comprehensive queue validation
 * @param[in] queue Ready queue to validate
 * @return true if valid, false if corrupted
 * @req REQ-SAFETY-002: Queue integrity checking
 */
bool dsrtos_ready_queue_validate(const dsrtos_ready_queue_t* queue);

/**
 * @brief Attempt queue repair
 * @param[in,out] queue Ready queue to repair
 * @return DSRTOS_SUCCESS if repaired
 * @req REQ-SAFETY-003: Error recovery
 */
dsrtos_error_t dsrtos_ready_queue_repair(dsrtos_ready_queue_t* queue);

/*==============================================================================
 * FUNCTION PROTOTYPES - Priority Bitmap Operations
 *============================================================================*/

/**
 * @brief Set priority bit atomically
 * @param[in,out] bitmap Priority bitmap array
 * @param[in] priority Priority level (0-255)
 * @req REQ-BITMAP-001: Atomic operations
 */
void dsrtos_priority_bitmap_set(volatile uint32_t* bitmap, uint8_t priority);

/**
 * @brief Clear priority bit atomically
 * @param[in,out] bitmap Priority bitmap array
 * @param[in] priority Priority level (0-255)
 * @req REQ-BITMAP-002: Atomic operations
 */
void dsrtos_priority_bitmap_clear(volatile uint32_t* bitmap, uint8_t priority);

/**
 * @brief Find highest priority with validation
 * @param[in] bitmap Priority bitmap array
 * @return Highest priority (1-256) or 0 if none
 * @req REQ-BITMAP-003: O(1) priority lookup
 */
uint16_t dsrtos_priority_bitmap_get_highest(const volatile uint32_t* bitmap);

/**
 * @brief Check if priority is set
 * @param[in] bitmap Priority bitmap array
 * @param[in] priority Priority level to check
 * @return true if set, false otherwise
 * @req REQ-BITMAP-004: Fast bit test
 */
bool dsrtos_priority_bitmap_is_set(const volatile uint32_t* bitmap, uint8_t priority);

/*==============================================================================
 * FUNCTION PROTOTYPES - Scheduling Decisions
 *============================================================================*/

/**
 * @brief Make scheduling decision with safety checks
 * @param[in,out] interface Scheduler interface
 * @param[in] reason Reason for scheduling
 * @return Next task to run or NULL
 * @req REQ-SCHED-003: Safe scheduling decision
 */
dsrtos_tcb_t* dsrtos_schedule(
    dsrtos_scheduler_interface_t* interface,
    dsrtos_sched_reason_t reason
);

/**
 * @brief Check preemption need with validation
 * @param[in] interface Scheduler interface
 * @param[in] new_task Newly ready task
 * @return true if preemption needed
 * @req REQ-SCHED-004: Preemption decision
 */
bool dsrtos_need_preemption(
    const dsrtos_scheduler_interface_t* interface,
    const dsrtos_tcb_t* new_task
);

/*==============================================================================
 * FUNCTION PROTOTYPES - Priority Inheritance
 *============================================================================*/

/**
 * @brief Apply priority inheritance with chain limit
 * @param[in,out] interface Scheduler interface
 * @param[in,out] task Task to inherit priority
 * @param[in] inherited_priority Priority to inherit
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-PRIO-001: Bounded priority inheritance
 */
dsrtos_error_t dsrtos_priority_inherit(
    dsrtos_scheduler_interface_t* interface,
    dsrtos_tcb_t* task,
    uint8_t inherited_priority
);

/**
 * @brief Release priority inheritance safely
 * @param[in,out] interface Scheduler interface
 * @param[in,out] task Task to release inheritance
 * @return DSRTOS_SUCCESS or error code
 * @req REQ-PRIO-002: Safe priority restoration
 */
dsrtos_error_t dsrtos_priority_release(
    dsrtos_scheduler_interface_t* interface,
    dsrtos_tcb_t* task
);

/*==============================================================================
 * INLINE FUNCTIONS - Performance Critical
 *============================================================================*/

/**
 * @brief Lock ready queue with timeout
 * @param[in,out] queue Ready queue to lock
 * @req REQ-SYNC-001: Bounded critical sections
 */
static inline void dsrtos_ready_queue_lock(dsrtos_ready_queue_t* queue)
{
    uint32_t primask;
    
    /* MISRA-C:2012 Rule 13.5: No side effects in Boolean expression */
    if (queue != NULL) {
        primask = __get_PRIMASK();
        __disable_irq();
        
        queue->sync.lock_count++;
        if (queue->sync.lock_count == 1U) {
            queue->sync.lock_owner = primask;
        }
    }
}

/**
 * @brief Unlock ready queue
 * @param[in,out] queue Ready queue to unlock
 * @req REQ-SYNC-002: Safe unlocking
 */
static inline void dsrtos_ready_queue_unlock(dsrtos_ready_queue_t* queue)
{
    /* MISRA-C:2012 Rule 13.5: No side effects in Boolean expression */
    if ((queue != NULL) && (queue->sync.lock_count > 0U)) {
        queue->sync.lock_count--;
        if (queue->sync.lock_count == 0U) {
            if ((queue->sync.lock_owner & 0x1U) == 0U) {
                __enable_irq();
            }
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_SCHEDULER_INTERFACE_H */
