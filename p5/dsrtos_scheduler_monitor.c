/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Monitoring Implementation
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
#include "dsrtos_scheduler_manager.h"
#include "dsrtos_critical.h"
#include "dsrtos_config.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define MONITOR_TRACE(msg)      DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define MONITOR_ASSERT(cond)    DSRTOS_ASSERT(cond)

/* Monitoring thresholds */
#define MONITOR_SAMPLE_PERIOD_MS    (10U)
#define MONITOR_HISTORY_SIZE        (100U)
#define MONITOR_ANOMALY_THRESHOLD   (3U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Performance sample */
typedef struct {
    uint32_t timestamp;
    uint8_t cpu_usage;
    uint32_t context_switches;
    uint32_t deadline_misses;
    uint32_t scheduling_time_ns;
    uint32_t ipc_rate;
} monitor_sample_t;

/* Monitor control block */
typedef struct {
    /* Sample buffer */
    monitor_sample_t samples[MONITOR_HISTORY_SIZE];
    uint32_t sample_index;
    uint32_t sample_count;
    
    /* Anomaly detection */
    struct {
        uint32_t cpu_anomalies;
        uint32_t deadline_anomalies;
        uint32_t latency_anomalies;
        uint32_t last_anomaly_time;
    } anomalies;
    
    /* Trends */
    struct {
        int8_t cpu_trend;      /* -1: decreasing, 0: stable, 1: increasing */
        int8_t ipc_trend;
        int8_t deadline_trend;
        uint32_t trend_duration;
    } trends;
    
    /* Alerts */
    struct {
        bool cpu_overload;
        bool deadline_crisis;
        bool resource_starvation;
        uint32_t alert_mask;
    } alerts;
    
    /* State */
    uint32_t last_sample_time;
    volatile uint32_t monitoring_enabled;
} monitor_control_block_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static monitor_control_block_t g_monitor = {0};

/*==============================================================================
 *                     LOCAL FUNCTION PROTOTYPES
 *============================================================================*/
static void collect_sample(monitor_sample_t* sample);
static void analyze_trends(void);
static void detect_anomalies(const monitor_sample_t* sample);
static void generate_alerts(void);

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize scheduler monitoring
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_monitor_init(void)
{
    (void)memset(&g_monitor, 0, sizeof(g_monitor));
    g_monitor.monitoring_enabled = 1U;
    g_monitor.last_sample_time = dsrtos_get_tick_count();
    
    MONITOR_TRACE("Scheduler monitoring initialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Update scheduler monitoring
 */
void dsrtos_scheduler_monitor_update(void)
{
    monitor_sample_t sample;
    uint32_t current_time;
    
    if (g_monitor.monitoring_enabled == 0U) {
        return;
    }
    
    /* Check sample period */
    current_time = dsrtos_get_tick_count();
    if ((current_time - g_monitor.last_sample_time) < MONITOR_SAMPLE_PERIOD_MS) {
        return;
    }
    
    /* Collect sample */
    collect_sample(&sample);
    
    /* Store sample */
    g_monitor.samples[g_monitor.sample_index] = sample;
    g_monitor.sample_index = (g_monitor.sample_index + 1U) % MONITOR_HISTORY_SIZE;
    if (g_monitor.sample_count < MONITOR_HISTORY_SIZE) {
        g_monitor.sample_count++;
    }
    
    /* Analyze */
    if (g_monitor.sample_count >= 10U) {
        analyze_trends();
        detect_anomalies(&sample);
        generate_alerts();
    }
    
    g_monitor.last_sample_time = current_time;
}

/**
 * @brief Get monitoring statistics
 * @param stats Buffer for statistics
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_monitor_get_stats(void* stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy relevant statistics */
    (void)memcpy(stats, &g_monitor.trends, sizeof(g_monitor.trends));
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Check for scheduler health
 * @return true if healthy, false if issues detected
 */
bool dsrtos_scheduler_monitor_is_healthy(void)
{
    return (g_monitor.alerts.alert_mask == 0U) &&
           (g_monitor.anomalies.cpu_anomalies < MONITOR_ANOMALY_THRESHOLD) &&
           (g_monitor.anomalies.deadline_anomalies < MONITOR_ANOMALY_THRESHOLD);
}

/*==============================================================================
 *                          LOCAL FUNCTIONS
 *============================================================================*/

/**
 * @brief Collect performance sample
 * @param sample Sample buffer
 */
static void collect_sample(monitor_sample_t* sample)
{
    dsrtos_scheduler_context_t* ctx = dsrtos_scheduler_get_context();
    
    sample->timestamp = dsrtos_get_tick_count();
    sample->cpu_usage = ctx->resources.cpu_usage_percent;
    sample->context_switches = ctx->metrics.context_switches;
    sample->deadline_misses = ctx->metrics.deadline_misses;
    sample->scheduling_time_ns = ctx->metrics.average_decision_time_ns;
    sample->ipc_rate = ctx->ipc_stats.ipc_rate;
}

/**
 * @brief Analyze performance trends
 */
static void analyze_trends(void)
{
    uint32_t recent_avg_cpu = 0U;
    uint32_t older_avg_cpu = 0U;
    uint32_t samples_per_half = g_monitor.sample_count / 2U;
    
    /* Calculate CPU trend */
    for (uint32_t i = 0U; i < samples_per_half; i++) {
        uint32_t older_idx = (g_monitor.sample_index + MONITOR_HISTORY_SIZE - 
                             g_monitor.sample_count + i) % MONITOR_HISTORY_SIZE;
        uint32_t recent_idx = (g_monitor.sample_index + MONITOR_HISTORY_SIZE - 
                              samples_per_half + i) % MONITOR_HISTORY_SIZE;
        
        older_avg_cpu += g_monitor.samples[older_idx].cpu_usage;
        recent_avg_cpu += g_monitor.samples[recent_idx].cpu_usage;
    }
    
    older_avg_cpu /= samples_per_half;
    recent_avg_cpu /= samples_per_half;
    
    /* Determine trend */
    if (recent_avg_cpu > older_avg_cpu + 10U) {
        g_monitor.trends.cpu_trend = 1;  /* Increasing */
    } else if (recent_avg_cpu < older_avg_cpu - 10U) {
        g_monitor.trends.cpu_trend = -1; /* Decreasing */
    } else {
        g_monitor.trends.cpu_trend = 0;  /* Stable */
    }
}

/**
 * @brief Detect anomalies in sample
 * @param sample Current sample
 */
static void detect_anomalies(const monitor_sample_t* sample)
{
    /* CPU anomaly */
    if (sample->cpu_usage > 95U) {
        g_monitor.anomalies.cpu_anomalies++;
        g_monitor.anomalies.last_anomaly_time = sample->timestamp;
    } else if (g_monitor.anomalies.cpu_anomalies > 0U) {
        g_monitor.anomalies.cpu_anomalies--;
    }
    
    /* Deadline anomaly */
    if (sample->deadline_misses > 0U) {
        g_monitor.anomalies.deadline_anomalies++;
        g_monitor.anomalies.last_anomaly_time = sample->timestamp;
    } else if (g_monitor.anomalies.deadline_anomalies > 0U) {
        g_monitor.anomalies.deadline_anomalies--;
    }
    
    /* Latency anomaly */
    if (sample->scheduling_time_ns > 10000U) { /* >10μs */
        g_monitor.anomalies.latency_anomalies++;
        g_monitor.anomalies.last_anomaly_time = sample->timestamp;
    } else if (g_monitor.anomalies.latency_anomalies > 0U) {
        g_monitor.anomalies.latency_anomalies--;
    }
}

/**
 * @brief Generate alerts based on monitoring
 */
static void generate_alerts(void)
{
    uint32_t alert_mask = 0U;
    
    /* CPU overload alert */
    if (g_monitor.anomalies.cpu_anomalies >= MONITOR_ANOMALY_THRESHOLD) {
        g_monitor.alerts.cpu_overload = true;
        alert_mask |= (1U << 0);
    } else {
        g_monitor.alerts.cpu_overload = false;
    }
    
    /* Deadline crisis alert */
    if (g_monitor.anomalies.deadline_anomalies >= MONITOR_ANOMALY_THRESHOLD) {
        g_monitor.alerts.deadline_crisis = true;
        alert_mask |= (1U << 1);
    } else {
        g_monitor.alerts.deadline_crisis = false;
    }
    
    /* Resource starvation alert */
    dsrtos_scheduler_context_t* ctx = dsrtos_scheduler_get_context();
    if (ctx->resources.resource_contention > 75U) {
        g_monitor.alerts.resource_starvation = true;
        alert_mask |= (1U << 2);
    } else {
        g_monitor.alerts.resource_starvation = false;
    }
    
    g_monitor.alerts.alert_mask = alert_mask;
    
    if (alert_mask != 0U) {
        MONITOR_TRACE("Scheduler alerts generated");
    }
}
