/*
 * DSRTOS Scheduler Decision Engine
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_SCHEDULER_DECISION_H
#define DSRTOS_SCHEDULER_DECISION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_task_scheduler_interface.h"

/* System metrics for decision making */
typedef struct {
    uint32_t cpu_load_percent;
    uint32_t ready_task_count;
    uint32_t blocked_task_count;
    uint32_t interrupt_rate;
    uint32_t context_switch_rate;
    uint32_t memory_free_bytes;
    uint32_t stack_usage_percent;
    bool idle_active;
} dsrtos_system_metrics_t;

/* Scheduling decision result */
typedef struct {
    dsrtos_tcb_t* selected_task;
    dsrtos_sched_reason_t reason;
    uint32_t decision_time_cycles;
    uint32_t confidence_level;
    bool preemption_required;
    bool migration_required;
    uint8_t suggested_priority;
} dsrtos_sched_decision_t;

/* Decision criteria weights */
typedef struct {
    uint16_t priority_weight;
    uint16_t deadline_weight;
    uint16_t cpu_affinity_weight;
    uint16_t cache_locality_weight;
    uint16_t energy_weight;
} dsrtos_decision_weights_t;

/* Function prototypes */
dsrtos_error_t dsrtos_scheduler_decision_init(void);
dsrtos_error_t dsrtos_scheduler_decision_deinit(void);

dsrtos_sched_decision_t* dsrtos_scheduler_make_decision(
    dsrtos_system_metrics_t* metrics
);

bool dsrtos_scheduler_should_preempt(
    const dsrtos_tcb_t* current,
    const dsrtos_tcb_t* candidate
);

uint32_t dsrtos_scheduler_calculate_quantum(
    const dsrtos_tcb_t* task
);

void dsrtos_scheduler_update_metrics(
    dsrtos_system_metrics_t* metrics
);

dsrtos_error_t dsrtos_scheduler_set_weights(
    const dsrtos_decision_weights_t* weights
);

uint32_t dsrtos_scheduler_evaluate_task_score(
    const dsrtos_tcb_t* task,
    const dsrtos_system_metrics_t* metrics
);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_DECISION_H */
