/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Development Team
 * 
 * File: dsrtos_scheduler_rr.h
 * Description: Round Robin Scheduler Implementation
 * Phase: 6 - Concrete Scheduler Implementations
 * 
 * MISRA-C:2012 Compliance:
 * - Dir 4.9: Function-like macros used for performance-critical operations
 * - Rule 8.13: Const qualification for pointer parameters
 * - Rule 17.7: Return values checked for all functions
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, IEC 61508 SIL 3, ISO 26262 ASIL D
 */

#ifndef DSRTOS_SCHEDULER_RR_H
#define DSRTOS_SCHEDULER_RR_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_scheduler.h"
#include "dsrtos_task_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ROUND ROBIN SCHEDULER CONFIGURATION
 * ============================================================================ */

/* Time slice configuration */
#define RR_DEFAULT_TIMESLICE_MS     (10U)
#define RR_MIN_TIMESLICE_MS         (1U)
#define RR_MAX_TIMESLICE_MS         (100U)
#define RR_TIMESLICE_EXTENSION_MAX  (3U)

/* Starvation prevention */
#define RR_STARVATION_THRESHOLD_MS  (5000U)
#define RR_STARVATION_BOOST_FACTOR  (2U)

/* Queue management */
#define RR_MAX_READY_TASKS          (256U)
#define RR_QUEUE_INTEGRITY_CHECK    (0x52524151U)  /* 'RRAQ' */

/* Performance targets */
#define RR_MAX_SCHEDULE_TIME_US     (5U)
#define RR_MAX_ENQUEUE_TIME_US      (2U)
#define RR_MAX_DEQUEUE_TIME_US      (2U)

/* ============================================================================
 * ROUND ROBIN SCHEDULER STRUCTURES
 * ============================================================================ */

/* Forward declarations */
typedef struct dsrtos_rr_scheduler dsrtos_rr_scheduler_t;
typedef struct dsrtos_rr_stats dsrtos_rr_stats_t;
typedef struct dsrtos_rr_queue dsrtos_rr_queue_t;

/* Round Robin task node */
typedef struct dsrtos_rr_node {
    dsrtos_tcb_t* task;                    /* Task control block */
    struct dsrtos_rr_node* next;           /* Next in queue */
    struct dsrtos_rr_node* prev;           /* Previous in queue */
    uint32_t enqueue_time;                 /* Time when enqueued */
    uint32_t accumulated_wait;             /* Total wait time */
    uint8_t boost_level;                   /* Starvation boost level */
} dsrtos_rr_node_t;

/* Round Robin ready queue */
struct dsrtos_rr_queue {
    dsrtos_rr_node_t* head;                /* Queue head */
    dsrtos_rr_node_t* tail;                /* Queue tail */
    uint32_t count;                        /* Number of tasks */
    uint32_t integrity_check;              /* Integrity marker */
    
    /* Queue operations count */
    uint64_t enqueue_count;
    uint64_t dequeue_count;
};

/* Round Robin statistics */
struct dsrtos_rr_stats {
    /* Scheduling statistics */
    uint64_t total_slices;                 /* Total time slices */
    uint64_t total_preemptions;            /* Preemptions */
    uint64_t total_yields;                 /* Voluntary yields */
    uint64_t total_extensions;             /* Slice extensions */
    
    /* Performance metrics */
    uint32_t avg_slice_usage;              /* Average slice usage % */
    uint32_t max_queue_length;             /* Maximum queue length */
    uint32_t min_schedule_time_us;         /* Minimum schedule time */
    uint32_t max_schedule_time_us;         /* Maximum schedule time */
    uint32_t avg_schedule_time_us;         /* Average schedule time */
    
    /* Fairness metrics */
    uint32_t starvation_count;             /* Starvation detections */
    uint32_t boost_count;                  /* Priority boosts */
    uint32_t max_wait_time_ms;             /* Maximum wait time */
    uint32_t fairness_index;               /* Fairness index (0-100) */
};

/* Round Robin scheduler main structure */
struct dsrtos_rr_scheduler {
    /* Base scheduler interface */
    dsrtos_scheduler_base_t base;          /* Must be first member */
    
    /* Configuration */
    uint32_t time_slice_ms;                /* Current time slice */
    uint32_t min_slice_ms;                 /* Minimum time slice */
    uint32_t max_slice_ms;                 /* Maximum time slice */
    bool dynamic_slice;                    /* Dynamic slice adjustment */
    
    /* Ready queue */
    dsrtos_rr_queue_t ready_queue;         /* Task ready queue */
    
    /* Current execution state */
    dsrtos_tcb_t* current_task;            /* Currently running */
    uint32_t slice_remaining;              /* Ticks remaining */
    uint32_t slice_start_time;             /* Slice start time */
    uint32_t slice_extensions;             /* Extensions granted */
    
    /* Task node pool */
    dsrtos_rr_node_t node_pool[RR_MAX_READY_TASKS];
    uint32_t node_pool_bitmap[8];          /* 256 bits for allocation */
    
    /* Fairness tracking */
    uint32_t starvation_threshold;         /* Starvation threshold */
    uint32_t boost_priority;                /* Boost priority level */
    dsrtos_rr_node_t* starving_list;       /* Starving tasks */
    
    /* Statistics */
    dsrtos_rr_stats_t stats;               /* Performance statistics */
    
    /* Synchronization */
    uint32_t lock;                         /* Scheduler lock */
    uint32_t critical_section_count;       /* Critical section depth */
};

/* ============================================================================
 * ROUND ROBIN SCHEDULER API
 * ============================================================================ */

/* Initialization and lifecycle */
dsrtos_status_t dsrtos_rr_init(dsrtos_rr_scheduler_t* scheduler,
                               uint32_t time_slice_ms);
dsrtos_status_t dsrtos_rr_start(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_stop(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_reset(dsrtos_rr_scheduler_t* scheduler);

/* Task scheduling operations */
dsrtos_tcb_t* dsrtos_rr_select_next(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_enqueue(dsrtos_rr_scheduler_t* scheduler,
                                  dsrtos_tcb_t* task);
dsrtos_status_t dsrtos_rr_dequeue(dsrtos_rr_scheduler_t* scheduler,
                                  dsrtos_tcb_t* task);
dsrtos_status_t dsrtos_rr_remove(dsrtos_rr_scheduler_t* scheduler,
                                 dsrtos_tcb_t* task);

/* Time management */
void dsrtos_rr_tick_update(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_yield(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_extend_slice(dsrtos_rr_scheduler_t* scheduler,
                                       uint32_t extension_ms);

/* Configuration */
dsrtos_status_t dsrtos_rr_set_timeslice(dsrtos_rr_scheduler_t* scheduler,
                                        uint32_t time_slice_ms);
dsrtos_status_t dsrtos_rr_enable_dynamic_slice(dsrtos_rr_scheduler_t* scheduler,
                                               bool enable);
dsrtos_status_t dsrtos_rr_set_starvation_threshold(dsrtos_rr_scheduler_t* scheduler,
                                                   uint32_t threshold_ms);

/* Fairness and starvation prevention */
dsrtos_status_t dsrtos_rr_check_starvation(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_boost_starving_task(dsrtos_rr_scheduler_t* scheduler,
                                              dsrtos_rr_node_t* node);
uint32_t dsrtos_rr_calculate_fairness_index(dsrtos_rr_scheduler_t* scheduler);

/* Statistics and monitoring */
dsrtos_status_t dsrtos_rr_get_stats(const dsrtos_rr_scheduler_t* scheduler,
                                    dsrtos_rr_stats_t* stats);
dsrtos_status_t dsrtos_rr_reset_stats(dsrtos_rr_scheduler_t* scheduler);
uint32_t dsrtos_rr_get_queue_length(const dsrtos_rr_scheduler_t* scheduler);

/* Plugin interface implementation */
dsrtos_status_t dsrtos_rr_register_plugin(dsrtos_rr_scheduler_t* scheduler);
dsrtos_status_t dsrtos_rr_unregister_plugin(dsrtos_rr_scheduler_t* scheduler);

/* Queue management utilities */
dsrtos_rr_node_t* dsrtos_rr_alloc_node(dsrtos_rr_scheduler_t* scheduler);
void dsrtos_rr_free_node(dsrtos_rr_scheduler_t* scheduler,
                        dsrtos_rr_node_t* node);
bool dsrtos_rr_queue_verify_integrity(const dsrtos_rr_queue_t* queue);

/* Performance profiling */
uint32_t dsrtos_rr_measure_schedule_time(dsrtos_rr_scheduler_t* scheduler);
void dsrtos_rr_update_performance_metrics(dsrtos_rr_scheduler_t* scheduler,
                                          uint32_t schedule_time_us);

/* MISRA-C:2012 compliance helper macros */
#define RR_ASSERT_VALID_SCHEDULER(s) \
    do { \
        if ((s) == NULL || (s)->base.magic != SCHEDULER_MAGIC) { \
            return DSRTOS_INVALID_PARAM; \
        } \
    } while(0)

#define RR_ASSERT_VALID_TASK(t) \
    do { \
        if ((t) == NULL || (t)->magic != TCB_MAGIC) { \
            return DSRTOS_INVALID_PARAM; \
        } \
    } while(0)

#define RR_ENTER_CRITICAL(s) \
    do { \
        dsrtos_enter_critical(); \
        (s)->critical_section_count++; \
    } while(0)

#define RR_EXIT_CRITICAL(s) \
    do { \
        (s)->critical_section_count--; \
        dsrtos_exit_critical(); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_RR_H */
