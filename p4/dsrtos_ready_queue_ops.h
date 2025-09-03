/*
 * DSRTOS Ready Queue Operations
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_READY_QUEUE_OPS_H
#define DSRTOS_READY_QUEUE_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task.h"
#include "dsrtos_task_scheduler_interface.h"

/* Queue iterator for traversal */
typedef struct {
    uint16_t current_priority;
    void* current_node;
    uint32_t remaining_tasks;
    uint32_t iteration_count;
} dsrtos_queue_iterator_t;

/* Queue statistics */
typedef struct {
    uint32_t total_iterations;
    uint32_t max_iteration_time;
    uint32_t rebalance_count;
    uint32_t migration_count;
} dsrtos_queue_ops_stats_t;

/* Function prototypes */
dsrtos_error_t dsrtos_queue_iterator_init(
    dsrtos_queue_iterator_t* iter,
    dsrtos_ready_queue_t* queue
);

dsrtos_tcb_t* dsrtos_queue_iterator_next(dsrtos_queue_iterator_t* iter);

dsrtos_error_t dsrtos_ready_queue_rebalance(dsrtos_ready_queue_t* queue);

dsrtos_error_t dsrtos_ready_queue_migrate(
    dsrtos_ready_queue_t* src_queue,
    dsrtos_ready_queue_t* dst_queue,
    dsrtos_tcb_t* task
);

dsrtos_error_t dsrtos_ready_queue_clear(dsrtos_ready_queue_t* queue);

dsrtos_error_t dsrtos_ready_queue_get_stats(
    dsrtos_ready_queue_t* queue,
    dsrtos_queue_ops_stats_t* stats
);

bool dsrtos_ready_queue_contains(
    dsrtos_ready_queue_t* queue,
    dsrtos_tcb_t* task
);

uint32_t dsrtos_ready_queue_count_at_priority(
    dsrtos_ready_queue_t* queue,
    uint8_t priority
);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_READY_QUEUE_OPS_H */
