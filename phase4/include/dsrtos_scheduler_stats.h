/*
 * DSRTOS Scheduler Statistics Interface
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_SCHEDULER_STATS_H
#define DSRTOS_SCHEDULER_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dsrtos_types.h"

/* Scheduler statistics structure */
typedef struct {
    /* Scheduling operations */
    uint64_t total_schedules;
    uint64_t total_context_switches;
    uint64_t total_preemptions;
    uint64_t total_migrations;
    
    /* Timing statistics */
    uint32_t avg_schedule_time_ns;
    uint32_t max_schedule_time_ns;
    uint32_t min_schedule_time_ns;
    uint32_t avg_switch_time_ns;
    uint32_t max_switch_time_ns;
    
    /* Queue statistics */
    uint32_t queue_operations;
    uint32_t queue_max_depth;
    uint32_t queue_avg_depth;
    
    /* Error statistics */
    uint32_t priority_inversions;
    uint32_t deadline_misses;
    uint32_t stack_overflows;
    uint32_t queue_corruptions;
} dsrtos_scheduler_stats_t;

/* Per-priority statistics */
typedef struct {
    uint32_t tasks_at_priority;
    uint32_t total_runtime_ms;
    uint32_t context_switches;
    uint32_t avg_wait_time_ms;
    uint32_t max_wait_time_ms;
} dsrtos_priority_stats_t;

/* Queue depth statistics */
typedef struct {
    uint32_t current_depth;
    uint32_t max_depth;
    uint32_t min_depth;
    uint32_t avg_depth;
    uint32_t samples;
} dsrtos_queue_depth_stats_t;

/* Function prototypes */
dsrtos_error_t dsrtos_scheduler_stats_init(void);
dsrtos_error_t dsrtos_scheduler_stats_get(dsrtos_scheduler_stats_t* stats);
dsrtos_error_t dsrtos_scheduler_stats_reset(void);
dsrtos_error_t dsrtos_priority_stats_get(uint8_t priority, dsrtos_priority_stats_t* stats);
dsrtos_error_t dsrtos_queue_depth_stats_get(dsrtos_queue_depth_stats_t* stats);

/* Update functions */
void dsrtos_scheduler_stats_update_schedule(uint32_t time_ns);
void dsrtos_scheduler_stats_update_switch(uint32_t time_ns);
void dsrtos_scheduler_stats_update_preemption(void);
void dsrtos_scheduler_stats_update_migration(void);
void dsrtos_scheduler_stats_update_queue_depth(uint32_t depth);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_STATS_H */
