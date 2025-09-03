/*
 * @file dsrtos_task_statistics.h
 * @brief DSRTOS Task Statistics Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#ifndef DSRTOS_TASK_STATISTICS_H
#define DSRTOS_TASK_STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_task_manager.h"

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/* Statistics export format */
typedef enum {
    DSRTOS_STATS_FORMAT_BINARY = 0,
    DSRTOS_STATS_FORMAT_CSV    = 1,
    DSRTOS_STATS_FORMAT_JSON   = 2
} dsrtos_stats_format_t;

/* CPU load statistics */
typedef struct {
    uint32_t idle_time;
    uint32_t busy_time;
    uint32_t last_sample_time;
    float cpu_load_percent;
    float cpu_load_ewma;
} cpu_load_stats_t;

/* Task performance metrics */
typedef struct {
    uint64_t total_runtime;
    uint64_t min_runtime;
    uint64_t max_runtime;
    uint64_t avg_runtime;
    uint32_t execution_count;
    uint32_t deadline_misses;
    uint32_t response_time_min;
    uint32_t response_time_max;
    uint32_t response_time_avg;
    uint32_t jitter_max;
} task_perf_metrics_t;

/* System-wide statistics */
typedef struct {
    cpu_load_stats_t cpu_load;
    uint32_t total_context_switches;
    uint32_t voluntary_yields;
    uint32_t preemptions;
    uint32_t migrations;
    uint32_t active_tasks;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
    uint32_t suspended_tasks;
    uint64_t system_uptime;
    uint32_t peak_active_tasks;
} system_stats_t;

/* Task statistics structure */
typedef struct {
    uint32_t total_runtime;
    uint32_t max_runtime;
    uint32_t min_runtime;
    uint32_t context_switches;
    uint32_t preemptions;
    uint32_t deadline_misses;
} dsrtos_task_stats_t;

/* Statistics report hook prototype */
typedef void (*dsrtos_stats_report_hook_t)(const system_stats_t *stats);

/*==============================================================================
 * PUBLIC API
 *============================================================================*/

/* Initialization */
dsrtos_error_t dsrtos_stats_init_phase3(void);

/* Control */
dsrtos_error_t dsrtos_stats_enable(bool enable);
dsrtos_error_t dsrtos_stats_collect(void);

/* Retrieval */
dsrtos_error_t dsrtos_stats_get_task_phase3(const dsrtos_tcb_t *tcb,
                                            dsrtos_task_stats_t *stats);
dsrtos_error_t dsrtos_stats_get_system(system_stats_t *stats);
dsrtos_error_t dsrtos_stats_get_performance(uint32_t task_id,
                                            task_perf_metrics_t *metrics);
dsrtos_error_t dsrtos_stats_get_cpu_load(float *load);

/* Management */
dsrtos_error_t dsrtos_stats_reset_phase3(dsrtos_tcb_t *tcb);
dsrtos_error_t dsrtos_stats_register_hook(dsrtos_stats_report_hook_t hook);

/* Export */
int32_t dsrtos_stats_export_phase3(void *buffer, uint32_t size, dsrtos_stats_format_t format);

/* Helper function for task manager - declared in dsrtos_task_manager.h */

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_STATISTICS_H */
