/*
 * DSRTOS Queue Integrity Checking
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_QUEUE_INTEGRITY_H
#define DSRTOS_QUEUE_INTEGRITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_scheduler_interface.h"

/* Integrity check results */
typedef enum {
    DSRTOS_INTEGRITY_OK = 0,
    DSRTOS_INTEGRITY_MAGIC_FAIL,
    DSRTOS_INTEGRITY_COUNT_MISMATCH,
    DSRTOS_INTEGRITY_BITMAP_MISMATCH,
    DSRTOS_INTEGRITY_LIST_CORRUPTED,
    DSRTOS_INTEGRITY_NODE_CORRUPTED,
    DSRTOS_INTEGRITY_CHECKSUM_FAIL,
    DSRTOS_INTEGRITY_CYCLE_DETECTED,
    DSRTOS_INTEGRITY_MEMORY_CORRUPTION
} dsrtos_integrity_status_t;

/* Integrity check report */
typedef struct {
    dsrtos_integrity_status_t status;
    uint32_t error_count;
    uint32_t last_check_tick;
    uint32_t check_count;
    uint32_t repairs_attempted;
    uint32_t repairs_successful;
    uint32_t cycles_detected;
    uint32_t nodes_corrupted;
} dsrtos_integrity_report_t;

/* Repair options */
typedef enum {
    DSRTOS_REPAIR_NONE = 0,
    DSRTOS_REPAIR_MINIMAL,
    DSRTOS_REPAIR_MODERATE,
    DSRTOS_REPAIR_AGGRESSIVE,
    DSRTOS_REPAIR_REBUILD
} dsrtos_repair_level_t;

/* Function prototypes */
dsrtos_error_t dsrtos_queue_integrity_init(void);

dsrtos_integrity_status_t dsrtos_queue_check_integrity(
    dsrtos_ready_queue_t* queue
);

dsrtos_error_t dsrtos_queue_repair_integrity(
    dsrtos_ready_queue_t* queue,
    dsrtos_repair_level_t level
);

dsrtos_error_t dsrtos_queue_get_integrity_report(
    dsrtos_integrity_report_t* report
);

uint32_t dsrtos_queue_calculate_checksum(
    const dsrtos_ready_queue_t* queue
);

bool dsrtos_node_validate(
    const dsrtos_queue_node_t* node
);

bool dsrtos_queue_detect_cycle(
    const dsrtos_priority_list_t* list
);

dsrtos_error_t dsrtos_queue_verify_consistency(
    const dsrtos_ready_queue_t* queue
);

void dsrtos_queue_integrity_periodic_check(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_QUEUE_INTEGRITY_H */
