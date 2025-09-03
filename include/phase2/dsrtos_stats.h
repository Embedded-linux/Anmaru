/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Statistics Engine Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_STATS_H
#define DSRTOS_STATS_H

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
 * STATISTICS CATEGORIES
 *============================================================================*/
typedef enum {
    DSRTOS_STATS_CAT_KERNEL     = 0x00U,
    DSRTOS_STATS_CAT_TASK       = 0x01U,
    DSRTOS_STATS_CAT_SCHEDULER  = 0x02U,
    DSRTOS_STATS_CAT_MEMORY     = 0x03U,
    DSRTOS_STATS_CAT_IPC        = 0x04U,
    DSRTOS_STATS_CAT_INTERRUPT  = 0x05U,
    DSRTOS_STATS_CAT_TIMER      = 0x06U,
    DSRTOS_STATS_CAT_DRIVER     = 0x07U,
    DSRTOS_STATS_CAT_CUSTOM     = 0x08U,
    DSRTOS_STATS_CAT_MAX        = 0x09U
} dsrtos_stats_category_t;

/*=============================================================================
 * STATISTICS COUNTER TYPES
 *============================================================================*/
typedef enum {
    DSRTOS_STATS_TYPE_COUNTER   = 0x00U,  /* Simple counter */
    DSRTOS_STATS_TYPE_GAUGE     = 0x01U,  /* Current value */
    DSRTOS_STATS_TYPE_HISTOGRAM = 0x02U,  /* Distribution */
    DSRTOS_STATS_TYPE_RATE      = 0x03U,  /* Rate per second */
    DSRTOS_STATS_TYPE_AVERAGE   = 0x04U,  /* Running average */
    DSRTOS_STATS_TYPE_MIN_MAX   = 0x05U,  /* Min/max tracking */
} dsrtos_stats_type_t;

/*=============================================================================
 * KERNEL STATISTICS
 *============================================================================*/
typedef struct {
    /* Time Statistics */
    uint64_t uptime_ticks;              /* System uptime */
    uint64_t idle_ticks;                /* Idle time */
    uint64_t busy_ticks;                /* Busy time */
    
    /* Context Switch Statistics */
    uint32_t context_switches;          /* Total context switches */
    uint32_t voluntary_switches;        /* Voluntary switches */
    uint32_t preemptive_switches;       /* Preemptive switches */
    
    /* Interrupt Statistics */
    uint32_t interrupt_count;           /* Total interrupts */
    uint32_t nested_interrupts;         /* Nested interrupt count */
    uint32_t max_interrupt_latency;     /* Maximum latency */
    
    /* Error Statistics */
    uint32_t error_count;               /* Total errors */
    uint32_t panic_count;               /* Panic count */
    uint32_t assertion_failures;        /* Assertion failures */
    
    /* System Load */
    uint8_t cpu_usage_percent;          /* CPU usage percentage */
    uint8_t peak_cpu_usage;             /* Peak CPU usage */
    uint16_t reserved;                  /* Alignment */
} dsrtos_kernel_stats_t;

/*=============================================================================
 * TASK STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t task_count;                /* Current task count */
    uint32_t peak_task_count;           /* Peak task count */
    uint32_t tasks_created;             /* Total tasks created */
    uint32_t tasks_deleted;             /* Total tasks deleted */
    uint32_t stack_overflows;           /* Stack overflow count */
    uint32_t deadline_misses;           /* Deadline miss count */
    uint64_t total_runtime;             /* Total runtime cycles */
} dsrtos_task_stats_t;

/*=============================================================================
 * MEMORY STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t heap_size;                 /* Total heap size */
    uint32_t heap_used;                 /* Heap bytes used */
    uint32_t heap_free;                 /* Heap bytes free */
    uint32_t heap_peak_usage;           /* Peak heap usage */
    uint32_t allocation_count;          /* Total allocations */
    uint32_t free_count;                /* Total frees */
    uint32_t allocation_failures;       /* Failed allocations */
    uint32_t fragmentation_percent;     /* Fragmentation percentage */
} dsrtos_memory_stats_t;

/*=============================================================================
 * SCHEDULER STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t scheduler_invocations;     /* Scheduler calls */
    uint32_t scheduler_switches;        /* Scheduler type switches */
    uint32_t ready_queue_length;        /* Current ready queue length */
    uint32_t max_ready_queue_length;    /* Maximum ready queue length */
    uint64_t total_scheduling_time;     /* Total scheduling time */
    uint32_t avg_scheduling_time;       /* Average scheduling time */
    uint32_t max_scheduling_time;       /* Maximum scheduling time */
} dsrtos_scheduler_stats_t;

/*=============================================================================
 * STATISTICS ENTRY
 *============================================================================*/
typedef struct {
    const char* name;                   /* Statistic name */
    dsrtos_stats_category_t category;   /* Category */
    dsrtos_stats_type_t type;          /* Type */
    union {
        uint32_t counter;               /* Counter value */
        int32_t gauge;                  /* Gauge value */
        struct {
            uint32_t min;
            uint32_t max;
            uint32_t avg;
            uint32_t current;
        } range;                        /* Range values */
    } value;
    uint32_t update_count;              /* Update count */
    uint64_t timestamp;                 /* Last update timestamp */
} dsrtos_stats_entry_t;

/*=============================================================================
 * STATISTICS CONFIGURATION
 *============================================================================*/
typedef struct {
    bool enable_kernel_stats;           /* Enable kernel statistics */
    bool enable_task_stats;             /* Enable task statistics */
    bool enable_memory_stats;           /* Enable memory statistics */
    bool enable_scheduler_stats;        /* Enable scheduler statistics */
    uint32_t sample_rate_ms;            /* Sampling rate in ms */
    uint32_t history_size;              /* History buffer size */
} dsrtos_stats_config_t;

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize statistics engine
 * 
 * @param[in] config Statistics configuration
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-001: Initialization
 * @safety Must be called during kernel initialization
 */
dsrtos_error_t dsrtos_stats_init(const dsrtos_stats_config_t* config);

/**
 * @brief Update a statistic counter
 * 
 * @param[in] category Statistics category
 * @param[in] name Statistic name
 * @param[in] value Value to add
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-002: Counter update
 * @safety Thread-safe, may be called from ISR
 */
dsrtos_error_t dsrtos_stats_update(dsrtos_stats_category_t category,
                                   const char* name,
                                   uint32_t value);

/**
 * @brief Set a gauge value
 * 
 * @param[in] category Statistics category
 * @param[in] name Statistic name
 * @param[in] value Gauge value
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-003: Gauge update
 * @safety Thread-safe, may be called from ISR
 */
dsrtos_error_t dsrtos_stats_set_gauge(dsrtos_stats_category_t category,
                                      const char* name,
                                      int32_t value);

/**
 * @brief Get kernel statistics
 * 
 * @param[out] stats Kernel statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-004: Kernel stats retrieval
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_get_kernel(dsrtos_kernel_stats_t* stats);

/**
 * @brief Get task statistics
 * 
 * @param[out] stats Task statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-005: Task stats retrieval
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_get_task(dsrtos_task_stats_t* stats);

/**
 * @brief Get memory statistics
 * 
 * @param[out] stats Memory statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-006: Memory stats retrieval
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_get_memory(dsrtos_memory_stats_t* stats);

/**
 * @brief Get scheduler statistics
 * 
 * @param[out] stats Scheduler statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-007: Scheduler stats retrieval
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_get_scheduler(dsrtos_scheduler_stats_t* stats);

/**
 * @brief Reset statistics
 * 
 * @param[in] category Category to reset (DSRTOS_STATS_CAT_MAX for all)
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-008: Statistics reset
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_reset(dsrtos_stats_category_t category);

/**
 * @brief Calculate CPU usage percentage
 * 
 * @return CPU usage percentage (0-100)
 * 
 * @requirements REQ-STATS-009: CPU usage calculation
 * @safety Thread-safe, may be called from ISR
 */
uint8_t dsrtos_stats_cpu_usage(void);

/**
 * @brief Get statistics entry by name
 * 
 * @param[in] category Statistics category
 * @param[in] name Statistic name
 * @param[out] entry Statistics entry
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-010: Entry retrieval
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_get_entry(dsrtos_stats_category_t category,
                                      const char* name,
                                      dsrtos_stats_entry_t* entry);

/**
 * @brief Register custom statistic
 * 
 * @param[in] entry Statistics entry to register
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-011: Custom statistics
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_register(const dsrtos_stats_entry_t* entry);

/**
 * @brief Export statistics snapshot
 * 
 * @param[out] buffer Buffer for statistics data
 * @param[in,out] size Buffer size (in: available, out: used)
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-STATS-012: Statistics export
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_stats_export(void* buffer, size_t* size);

/*=============================================================================
 * STATISTICS MACROS
 *============================================================================*/

/**
 * @brief Increment a counter
 */
#define DSRTOS_STATS_INC(category, name) \
    dsrtos_stats_update((category), (name), 1U)

/**
 * @brief Add to a counter
 */
#define DSRTOS_STATS_ADD(category, name, val) \
    dsrtos_stats_update((category), (name), (val))

/**
 * @brief Set a gauge value
 */
#define DSRTOS_STATS_GAUGE(category, name, val) \
    dsrtos_stats_set_gauge((category), (name), (val))

/**
 * @brief Mark timestamp
 */
#define DSRTOS_STATS_TIMESTAMP(category, name) \
    dsrtos_stats_update((category), (name), (uint32_t)dsrtos_get_ticks())

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_STATS_H */
