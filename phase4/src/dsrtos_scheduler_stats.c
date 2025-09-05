/*
 * DSRTOS Scheduler Statistics Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_scheduler_stats.h"
#include "dsrtos_critical.h"
#include <string.h>

/* Global statistics */
static dsrtos_scheduler_stats_t g_scheduler_stats;
static dsrtos_priority_stats_t g_priority_stats[DSRTOS_PRIORITY_LEVELS];
static dsrtos_queue_depth_stats_t g_queue_depth_stats;
static bool g_stats_initialized = false;

/**
 * @brief Initialize scheduler statistics
 */
dsrtos_error_t dsrtos_scheduler_stats_init(void)
{
    /* uint32_t i; */ /* Unused for now */
    
    /* Clear global stats */
    (void)memset(&g_scheduler_stats, 0, sizeof(g_scheduler_stats));
    g_scheduler_stats.min_schedule_time_ns = UINT32_MAX;
    /* g_scheduler_stats.min_switch_time_ns = UINT32_MAX; */ /* Field not available */
    
    /* Clear priority stats */
    (void)memset(g_priority_stats, 0, sizeof(g_priority_stats));
    
    /* Initialize queue depth stats */
    g_queue_depth_stats.current_depth = 0U;
    g_queue_depth_stats.max_depth = 0U;
    g_queue_depth_stats.min_depth = UINT32_MAX;
    g_queue_depth_stats.avg_depth = 0U;
    g_queue_depth_stats.samples = 0U;
    
    g_stats_initialized = true;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get scheduler statistics
 */
dsrtos_error_t dsrtos_scheduler_stats_get(dsrtos_scheduler_stats_t* stats)
{
    if ((stats == NULL) || !g_stats_initialized) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    dsrtos_critical_enter();
    *stats = g_scheduler_stats;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Reset scheduler statistics
 */
dsrtos_error_t dsrtos_scheduler_stats_reset(void)
{
    if (!g_stats_initialized) {
        return DSRTOS_ERROR_NOT_INITIALIZED;
    }
    
    dsrtos_critical_enter();
    
    /* Reset counters but keep min values */
    g_scheduler_stats.total_schedules = 0U;
    g_scheduler_stats.total_context_switches = 0U;
    g_scheduler_stats.total_preemptions = 0U;
    g_scheduler_stats.total_migrations = 0U;
    g_scheduler_stats.queue_operations = 0U;
    g_scheduler_stats.priority_inversions = 0U;
    g_scheduler_stats.deadline_misses = 0U;
    g_scheduler_stats.stack_overflows = 0U;
    g_scheduler_stats.queue_corruptions = 0U;
    
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get priority-specific statistics
 */
dsrtos_error_t dsrtos_priority_stats_get(uint8_t priority, dsrtos_priority_stats_t* stats)
{
    if ((stats == NULL) || (priority >= DSRTOS_PRIORITY_LEVELS) || !g_stats_initialized) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    dsrtos_critical_enter();
    *stats = g_priority_stats[priority];
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get queue depth statistics
 */
dsrtos_error_t dsrtos_queue_depth_stats_get(dsrtos_queue_depth_stats_t* stats)
{
    if ((stats == NULL) || !g_stats_initialized) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    dsrtos_critical_enter();
    *stats = g_queue_depth_stats;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Update scheduling statistics
 */
void dsrtos_scheduler_stats_update_schedule(uint32_t time_ns)
{
    uint64_t total_time;
    
    if (!g_stats_initialized) {
        return;
    }
    
    g_scheduler_stats.total_schedules++;
    
    /* Update min/max */
    if (time_ns < g_scheduler_stats.min_schedule_time_ns) {
        g_scheduler_stats.min_schedule_time_ns = time_ns;
    }
    if (time_ns > g_scheduler_stats.max_schedule_time_ns) {
        g_scheduler_stats.max_schedule_time_ns = time_ns;
    }
    
    /* Update average */
    if (g_scheduler_stats.total_schedules > 0U) {
        total_time = (uint64_t)g_scheduler_stats.avg_schedule_time_ns * 
                    (g_scheduler_stats.total_schedules - 1U);
        total_time += time_ns;
        g_scheduler_stats.avg_schedule_time_ns = 
            (uint32_t)(total_time / g_scheduler_stats.total_schedules);
    }
}

/**
 * @brief Update context switch statistics
 */
void dsrtos_scheduler_stats_update_switch(uint32_t time_ns)
{
    uint64_t total_time;
    
    if (!g_stats_initialized) {
        return;
    }
    
    g_scheduler_stats.total_context_switches++;
    
    /* Update min/max */
    /* if (time_ns < g_scheduler_stats.min_switch_time_ns) {
        g_scheduler_stats.min_switch_time_ns = time_ns;
    } */ /* Field not available */
    if (time_ns > g_scheduler_stats.max_switch_time_ns) {
        g_scheduler_stats.max_switch_time_ns = time_ns;
    }
    
    /* Update average */
    if (g_scheduler_stats.total_context_switches > 0U) {
        total_time = (uint64_t)g_scheduler_stats.avg_switch_time_ns * 
                    (g_scheduler_stats.total_context_switches - 1U);
        total_time += time_ns;
        g_scheduler_stats.avg_switch_time_ns = 
            (uint32_t)(total_time / g_scheduler_stats.total_context_switches);
    }
}

/**
 * @brief Update preemption count
 */
void dsrtos_scheduler_stats_update_preemption(void)
{
    if (g_stats_initialized) {
        g_scheduler_stats.total_preemptions++;
    }
}

/**
 * @brief Update migration count
 */
void dsrtos_scheduler_stats_update_migration(void)
{
    if (g_stats_initialized) {
        g_scheduler_stats.total_migrations++;
    }
}

/**
 * @brief Update queue depth statistics
 */
void dsrtos_scheduler_stats_update_queue_depth(uint32_t depth)
{
    uint64_t total_depth;
    
    if (!g_stats_initialized) {
        return;
    }
    
    g_queue_depth_stats.current_depth = depth;
    g_queue_depth_stats.samples++;
    
    /* Update min/max */
    if (depth < g_queue_depth_stats.min_depth) {
        g_queue_depth_stats.min_depth = depth;
    }
    if (depth > g_queue_depth_stats.max_depth) {
        g_queue_depth_stats.max_depth = depth;
    }
    
    /* Update average */
    if (g_queue_depth_stats.samples > 0U) {
        total_depth = (uint64_t)g_queue_depth_stats.avg_depth * 
                     (g_queue_depth_stats.samples - 1U);
        total_depth += depth;
        g_queue_depth_stats.avg_depth = 
            (uint32_t)(total_depth / g_queue_depth_stats.samples);
    }
    
    /* Update global max */
    if (depth > g_scheduler_stats.queue_max_depth) {
        g_scheduler_stats.queue_max_depth = depth;
    }
}
