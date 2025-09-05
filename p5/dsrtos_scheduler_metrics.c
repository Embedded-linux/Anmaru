/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Metrics Collection Implementation
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_scheduler_core.h"
#include "dsrtos_critical.h"
#include "dsrtos_config.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define METRICS_TRACE(msg)      DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define METRICS_ASSERT(cond)    DSRTOS_ASSERT(cond)

/* Metric collection periods */
#define METRICS_UPDATE_PERIOD_MS    (100U)
#define METRICS_WINDOW_SIZE         (32U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Metrics collector */
typedef struct {
    /* Current metrics */
    dsrtos_scheduler_metrics_t current;
    
    /* Historical data */
    struct {
        uint32_t decision_times[METRICS_WINDOW_SIZE];
        uint32_t context_switch_times[METRICS_WINDOW_SIZE];
        uint32_t window_index;
    } history;
    
    /* Aggregated stats */
    struct {
        uint64_t total_decisions;
        uint64_t total_switches;
        uint64_t total_decision_time_ns;
        uint64_t total_switch_time_ns;
    } aggregate;
    
    /* Last update time */
    uint32_t last_update_time;
} metrics_collector_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static metrics_collector_t g_metrics = {0};

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize metrics collection
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_metrics_init(void)
{
    (void)memset(&g_metrics, 0, sizeof(g_metrics));
    g_metrics.last_update_time = dsrtos_get_tick_count();
    
    METRICS_TRACE("Metrics collection initialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Update scheduler metrics
 */
void dsrtos_scheduler_metrics_update(void)
{
    uint32_t current_time;
    dsrtos_scheduler_context_t* ctx;
    
    current_time = dsrtos_get_tick_count();
    
    /* Check update period */
    if ((current_time - g_metrics.last_update_time) < METRICS_UPDATE_PERIOD_MS) {
        return;
    }
    
    /* Get scheduler context */
    ctx = dsrtos_scheduler_get_context();
    if (ctx == NULL) {
        return;
    }
    
    /* Update from context */
    g_metrics.current = ctx->metrics;
    
    /* Calculate averages */
    if (g_metrics.current.scheduling_decisions > 0U) {
        g_metrics.current.average_decision_time_ns = 
            (uint32_t)(g_metrics.current.total_scheduling_time_ns / 
                      g_metrics.current.scheduling_decisions);
    }
    
    /* Update history */
    g_metrics.history.decision_times[g_metrics.history.window_index] = 
        g_metrics.current.average_decision_time_ns;
    g_metrics.history.context_switch_times[g_metrics.history.window_index] = 
        g_metrics.current.context_switches;
    g_metrics.history.window_index = 
        (g_metrics.history.window_index + 1U) % METRICS_WINDOW_SIZE;
    
    /* Update aggregates */
    g_metrics.aggregate.total_decisions += g_metrics.current.scheduling_decisions;
    g_metrics.aggregate.total_switches += g_metrics.current.context_switches;
    g_metrics.aggregate.total_decision_time_ns += 
        g_metrics.current.total_scheduling_time_ns;
    
    g_metrics.last_update_time = current_time;
}

/**
 * @brief Record scheduling decision
 * @param decision_time_ns Decision time in nanoseconds
 */
void dsrtos_scheduler_metrics_record_decision(uint32_t decision_time_ns)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    g_metrics.current.scheduling_decisions++;
    g_metrics.current.total_scheduling_time_ns += decision_time_ns;
    
    /* Update min/max */
    if ((decision_time_ns < g_metrics.current.min_decision_time_ns) ||
        (g_metrics.current.min_decision_time_ns == 0U)) {
        g_metrics.current.min_decision_time_ns = decision_time_ns;
    }
    if (decision_time_ns > g_metrics.current.max_decision_time_ns) {
        g_metrics.current.max_decision_time_ns = decision_time_ns;
    }
    
    dsrtos_critical_exit(sr);
}

/**
 * @brief Record context switch
 */
void dsrtos_scheduler_metrics_record_switch(void)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    g_metrics.current.context_switches++;
    dsrtos_critical_exit(sr);
}

/**
 * @brief Record task event
 * @param event_type Event type
 */
void dsrtos_scheduler_metrics_record_event(uint32_t event_type)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    switch (event_type) {
        case 0: /* Task scheduled */
            g_metrics.current.tasks_scheduled++;
            break;
        case 1: /* Task preempted */
            g_metrics.current.tasks_preempted++;
            break;
        case 2: /* Task yielded */
            g_metrics.current.tasks_yielded++;
            break;
        case 3: /* Task blocked */
            g_metrics.current.tasks_blocked++;
            break;
        case 4: /* Priority inversion */
            g_metrics.current.priority_inversions++;
            break;
        case 5: /* Deadline miss */
            g_metrics.current.deadline_misses++;
            break;
        case 6: /* Deadline met */
            g_metrics.current.deadline_met++;
            break;
        default:
            break;
    }
    
    dsrtos_critical_exit(sr);
}

/**
 * @brief Get current metrics
 * @param metrics Buffer for metrics
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_metrics_get(
    dsrtos_scheduler_metrics_t* metrics)
{
    uint32_t sr;
    
    if (metrics == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    sr = dsrtos_critical_enter();
    *metrics = g_metrics.current;
    dsrtos_critical_exit(sr);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Reset metrics
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_metrics_reset(void)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    /* Clear current metrics */
    (void)memset(&g_metrics.current, 0, sizeof(g_metrics.current));
    
    /* Clear history */
    (void)memset(&g_metrics.history, 0, sizeof(g_metrics.history));
    
    g_metrics.last_update_time = dsrtos_get_tick_count();
    
    dsrtos_critical_exit(sr);
    
    METRICS_TRACE("Metrics reset");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Calculate performance score
 * @return Performance score (0-100)
 */
uint32_t dsrtos_scheduler_metrics_performance_score(void)
{
    uint32_t score = 100U;
    
    /* Penalize for deadline misses */
    if (g_metrics.current.deadline_misses > 0U) {
        uint32_t miss_rate = 0U;
        uint32_t total_deadlines = g_metrics.current.deadline_misses + 
                                  g_metrics.current.deadline_met;
        if (total_deadlines > 0U) {
            miss_rate = (g_metrics.current.deadline_misses * 100U) / total_deadlines;
            if (miss_rate > 50U) {
                score = 0U;
            } else {
                score -= (miss_rate * 2U);
            }
        }
    }
    
    /* Penalize for high decision time */
    if (g_metrics.current.average_decision_time_ns > 10000U) { /* >10μs */
        score -= 20U;
    } else if (g_metrics.current.average_decision_time_ns > 5000U) { /* >5μs */
        score -= 10U;
    }
    
    /* Penalize for priority inversions */
    if (g_metrics.current.priority_inversions > 10U) {
        score -= 15U;
    } else if (g_metrics.current.priority_inversions > 0U) {
        score -= 5U;
    }
    
    /* Ensure score doesn't underflow */
    if (score > 100U) {
        score = 0U;
    }
    
    return score;
}
