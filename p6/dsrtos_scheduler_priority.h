/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Development Team
 * 
 * File: dsrtos_scheduler_priority.h
 * Description: Static Priority Scheduler with O(1) operations
 * Phase: 6 - Concrete Scheduler Implementations
 * 
 * Features:
 * - 256 priority levels (0 = highest, 255 = lowest)
 * - O(1) scheduling decisions using priority bitmap
 * - Priority inheritance support
 * - Priority aging for starvation prevention
 * 
 * MISRA-C:2012 Compliance:
 * - Dir 4.9: Function-like macros used for bit operations
 * - Rule 8.13: Const qualification for pointer parameters
 * - Rule 17.7: Return values checked for all functions
 */

#ifndef DSRTOS_SCHEDULER_PRIORITY_H
#define DSRTOS_SCHEDULER_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_scheduler.h"
#include "dsrtos_task_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * PRIORITY SCHEDULER CONFIGURATION
 * ============================================================================ */

/* Priority levels */
#define PRIO_NUM_LEVELS             (256U)
#define PRIO_HIGHEST                (0U)     /* Highest priority */
#define PRIO_LOWEST                 (255U)   /* Lowest priority */
#define PRIO_DEFAULT                (128U)   /* Default priority */

/* Priority bitmap */
#define PRIO_BITMAP_WORDS           (8U)     /* 256 bits / 32 = 8 words */
#define PRIO_BITS_PER_WORD          (32U)

/* Priority inheritance */
#define PRIO_INHERITANCE_ENABLED    (1U)
#define PRIO_MAX_INHERITANCE_DEPTH  (8U)
#define PRIO_INHERITANCE_TABLE_SIZE (32U)

/* Priority aging */
#define PRIO_AGING_ENABLED          (1U)
#define PRIO_AGING_PERIOD_MS        (1000U)
#define PRIO_AGE_THRESHOLD_MS       (5000U)
#define PRIO_AGE_BOOST              (10U)    /* Priority boost amount */

/* Performance targets */
#define PRIO_MAX_SCHEDULE_TIME_US   (3U)
#define PRIO_MAX_ENQUEUE_TIME_US    (2U)
#define PRIO_MAX_PRIORITY_SET_US    (1U)

/* Queue integrity */
#define PRIO_QUEUE_MAGIC            (0x5052494FU)  /* 'PRIO' */

/* ============================================================================
 * PRIORITY SCHEDULER STRUCTURES
 * ============================================================================ */

/* Forward declarations */
typedef struct dsrtos_priority_scheduler dsrtos_priority_scheduler_t;
typedef struct dsrtos_priority_stats dsrtos_priority_stats_t;
typedef struct dsrtos_priority_queue dsrtos_priority_queue_t;

/* Priority queue node */
typedef struct dsrtos_priority_node {
    dsrtos_tcb_t* task;                    /* Task control block */
    struct dsrtos_priority_node* next;     /* Next in priority queue */
    struct dsrtos_priority_node* prev;     /* Previous in priority queue */
    uint8_t effective_priority;             /* Current effective priority */
    uint8_t base_priority;                  /* Original priority */
    uint32_t enqueue_time;                  /* When enqueued */
    uint32_t age_counter;                   /* For priority aging */
} dsrtos_priority_node_t;

/* Single priority level queue */
struct dsrtos_priority_queue {
    dsrtos_priority_node_t* head;          /* Queue head */
    dsrtos_priority_node_t* tail;          /* Queue tail */
    uint32_t count;                        /* Number of tasks */
};

/* Priority inheritance record */
typedef struct dsrtos_pi_record {
    dsrtos_tcb_t* task;                    /* Task with inherited priority */
    uint8_t original_priority;              /* Original priority */
    uint8_t inherited_priority;             /* Inherited priority */
    uint32_t inheritance_count;             /* Number of inheritances */
    uint32_t timestamp;                     /* When inherited */
    void* blocking_resource;                /* Resource causing inheritance */
} dsrtos_pi_record_t;

/* Priority scheduler statistics */
struct dsrtos_priority_stats {
    /* Scheduling statistics */
    uint64_t total_schedules;               /* Total scheduling decisions */
    uint64_t priority_changes;              /* Dynamic priority changes */
    uint64_t inheritance_activations;       /* Priority inheritances */
    uint64_t aging_adjustments;             /* Aging adjustments */
    
    /* Performance metrics */
    uint32_t min_schedule_time_us;          /* Minimum schedule time */
    uint32_t max_schedule_time_us;          /* Maximum schedule time */
    uint32_t avg_schedule_time_us;          /* Average schedule time */
    uint32_t bitmap_scans;                  /* Bitmap scan operations */
    
    /* Priority distribution */
    uint32_t priority_distribution[PRIO_NUM_LEVELS];  /* Tasks per priority */
    uint32_t priority_switches[PRIO_NUM_LEVELS];      /* Switches at each level */
    
    /* Starvation metrics */
    uint32_t starvation_prevented;          /* Starvation preventions */
    uint32_t max_wait_time_ms;              /* Maximum wait time */
    uint32_t aging_promotions;              /* Tasks promoted by aging */
};

/* Priority scheduler main structure */
struct dsrtos_priority_scheduler {
    /* Base scheduler interface */
    dsrtos_scheduler_base_t base;           /* Must be first member */
    
    /* Priority configuration */
    uint8_t num_priorities;                 /* Number of priority levels */
    uint8_t highest_priority;               /* Highest priority value */
    uint8_t lowest_priority;                /* Lowest priority value */
    
    /* Ready queue array - one per priority */
    dsrtos_priority_queue_t ready_queues[PRIO_NUM_LEVELS];
    
    /* Priority bitmap for O(1) selection */
    struct {
        uint32_t bitmap[PRIO_BITMAP_WORDS]; /* 256 bits */
        uint8_t highest_set;                /* Cached highest priority */
        bool cache_valid;                   /* Cache validity flag */
        uint32_t last_update;               /* Last update timestamp */
    } priority_map;
    
    /* Task node pool */
    dsrtos_priority_node_t node_pool[256]; /* Pool of nodes */
    uint32_t node_bitmap[8];                /* Allocation bitmap */
    
    /* Priority inheritance support */
    struct {
        bool enabled;                       /* PI enabled */
        uint32_t max_depth;                 /* Max inheritance chain */
        uint32_t active_count;              /* Active inheritances */
        dsrtos_pi_record_t table[PRIO_INHERITANCE_TABLE_SIZE];
        uint32_t table_bitmap;              /* Table allocation bitmap */
    } inheritance;
    
    /* Priority aging */
    struct {
        bool enabled;                       /* Aging enabled */
        uint32_t period_ms;                 /* Aging check period */
        uint32_t threshold_ms;              /* Time before aging */
        uint8_t boost_amount;               /* Priority boost */
        uint32_t last_aging_time;           /* Last aging check */
    } aging;
    
    /* Current execution state */
    dsrtos_tcb_t* current_task;            /* Currently running task */
    uint8_t current_priority;               /* Current priority level */
    
    /* Statistics */
    dsrtos_priority_stats_t stats;          /* Performance statistics */
    
    /* Synchronization */
    uint32_t lock;                         /* Scheduler lock */
    uint32_t critical_count;                /* Critical section depth */
};

/* ============================================================================
 * PRIORITY BITMAP OPERATIONS (O(1) complexity)
 * ============================================================================ */

/* Set priority bit */
#define PRIO_BITMAP_SET(scheduler, priority) \
    do { \
        uint32_t _word = (priority) / 32U; \
        uint32_t _bit = (priority) % 32U; \
        (scheduler)->priority_map.bitmap[_word] |= (1U << _bit); \
        (scheduler)->priority_map.cache_valid = false; \
    } while(0)

/* Clear priority bit */
#define PRIO_BITMAP_CLEAR(scheduler, priority) \
    do { \
        uint32_t _word = (priority) / 32U; \
        uint32_t _bit = (priority) % 32U; \
        (scheduler)->priority_map.bitmap[_word] &= ~(1U << _bit); \
        (scheduler)->priority_map.cache_valid = false; \
    } while(0)

/* Test priority bit */
#define PRIO_BITMAP_TEST(scheduler, priority) \
    (((scheduler)->priority_map.bitmap[(priority) / 32U] & \
      (1U << ((priority) % 32U))) != 0U)

/* Find first set bit (highest priority) */
static inline uint8_t prio_bitmap_ffs(const uint32_t* bitmap)
{
    uint32_t word;
    uint32_t bit;
    uint32_t i;
    
    for (i = 0U; i < PRIO_BITMAP_WORDS; i++) {
        word = bitmap[i];
        if (word != 0U) {
            /* Use built-in or implement bit scan */
            #ifdef __GNUC__
                bit = (uint32_t)__builtin_ffs((int)word) - 1U;
            #else
                /* Software bit scan */
                bit = 0U;
                while ((word & (1U << bit)) == 0U) {
                    bit++;
                }
            #endif
            return (uint8_t)((i * 32U) + bit);
        }
    }
    
    return PRIO_LOWEST;  /* No priority set */
}

/* ============================================================================
 * PRIORITY SCHEDULER API
 * ============================================================================ */

/* Initialization and lifecycle */
dsrtos_status_t dsrtos_priority_init(dsrtos_priority_scheduler_t* scheduler);
dsrtos_status_t dsrtos_priority_start(dsrtos_priority_scheduler_t* scheduler);
dsrtos_status_t dsrtos_priority_stop(dsrtos_priority_scheduler_t* scheduler);
dsrtos_status_t dsrtos_priority_reset(dsrtos_priority_scheduler_t* scheduler);

/* Task scheduling operations */
dsrtos_tcb_t* dsrtos_priority_select_next(dsrtos_priority_scheduler_t* scheduler);
dsrtos_status_t dsrtos_priority_enqueue(dsrtos_priority_scheduler_t* scheduler,
                                        dsrtos_tcb_t* task,
                                        uint8_t priority);
dsrtos_status_t dsrtos_priority_dequeue(dsrtos_priority_scheduler_t* scheduler,
                                        dsrtos_tcb_t* task);
dsrtos_status_t dsrtos_priority_remove(dsrtos_priority_scheduler_t* scheduler,
                                       dsrtos_tcb_t* task);

/* Priority management */
dsrtos_status_t dsrtos_priority_set(dsrtos_priority_scheduler_t* scheduler,
                                    dsrtos_tcb_t* task,
                                    uint8_t new_priority);
uint8_t dsrtos_priority_get(const dsrtos_priority_scheduler_t* scheduler,
                            const dsrtos_tcb_t* task);
dsrtos_status_t dsrtos_priority_boost(dsrtos_priority_scheduler_t* scheduler,
                                      dsrtos_tcb_t* task,
                                      uint8_t boost_amount);

/* Priority inheritance */
dsrtos_status_t dsrtos_priority_inherit(dsrtos_priority_scheduler_t* scheduler,
                                        dsrtos_tcb_t* task,
                                        uint8_t inherited_priority,
                                        void* resource);
dsrtos_status_t dsrtos_priority_uninherit(dsrtos_priority_scheduler_t* scheduler,
                                          dsrtos_tcb_t* task,
                                          void* resource);
bool dsrtos_priority_has_inheritance(const dsrtos_priority_scheduler_t* scheduler,
                                     const dsrtos_tcb_t* task);

/* Priority aging */
dsrtos_status_t dsrtos_priority_aging_enable(dsrtos_priority_scheduler_t* scheduler,
                                             bool enable);
dsrtos_status_t dsrtos_priority_aging_configure(dsrtos_priority_scheduler_t* scheduler,
                                                uint32_t period_ms,
                                                uint32_t threshold_ms,
                                                uint8_t boost);
void dsrtos_priority_aging_check(dsrtos_priority_scheduler_t* scheduler);

/* Statistics and monitoring */
dsrtos_status_t dsrtos_priority_get_stats(const dsrtos_priority_scheduler_t* scheduler,
                                          dsrtos_priority_stats_t* stats);
dsrtos_status_t dsrtos_priority_reset_stats(dsrtos_priority_scheduler_t* scheduler);
uint32_t dsrtos_priority_get_task_count(const dsrtos_priority_scheduler_t* scheduler,
                                        uint8_t priority);

/* Plugin interface implementation */
dsrtos_status_t dsrtos_priority_register_plugin(dsrtos_priority_scheduler_t* scheduler);
dsrtos_status_t dsrtos_priority_unregister_plugin(dsrtos_priority_scheduler_t* scheduler);

/* Node pool management */
dsrtos_priority_node_t* dsrtos_priority_alloc_node(dsrtos_priority_scheduler_t* scheduler);
void dsrtos_priority_free_node(dsrtos_priority_scheduler_t* scheduler,
                              dsrtos_priority_node_t* node);

/* Queue operations */
void dsrtos_priority_queue_init(dsrtos_priority_queue_t* queue);
void dsrtos_priority_queue_push(dsrtos_priority_queue_t* queue,
                                dsrtos_priority_node_t* node);
dsrtos_priority_node_t* dsrtos_priority_queue_pop(dsrtos_priority_queue_t* queue);
void dsrtos_priority_queue_remove(dsrtos_priority_queue_t* queue,
                                  dsrtos_priority_node_t* node);

/* Performance profiling */
uint32_t dsrtos_priority_measure_schedule_time(dsrtos_priority_scheduler_t* scheduler);
void dsrtos_priority_update_metrics(dsrtos_priority_scheduler_t* scheduler,
                                    uint32_t schedule_time_us);

/* MISRA-C:2012 compliance helper macros */
#define PRIO_ASSERT_VALID_SCHEDULER(s) \
    do { \
        if ((s) == NULL || (s)->base.magic != SCHEDULER_MAGIC) { \
            return DSRTOS_INVALID_PARAM; \
        } \
    } while(0)

#define PRIO_ASSERT_VALID_PRIORITY(p) \
    do { \
        if ((p) >= PRIO_NUM_LEVELS) { \
            return DSRTOS_INVALID_PARAM; \
        } \
    } while(0)

#define PRIO_ENTER_CRITICAL(s) \
    do { \
        dsrtos_enter_critical(); \
        (s)->critical_count++; \
    } while(0)

#define PRIO_EXIT_CRITICAL(s) \
    do { \
        (s)->critical_count--; \
        dsrtos_exit_critical(); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_PRIORITY_H */
