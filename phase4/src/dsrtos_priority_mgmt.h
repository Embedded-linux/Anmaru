/*
 * DSRTOS Priority Management
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_PRIORITY_MGMT_H
#define DSRTOS_PRIORITY_MGMT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_task_manager.h"

/* Priority protocols */
typedef enum {
    DSRTOS_PRIORITY_PROTOCOL_NONE = 0,
    DSRTOS_PRIORITY_PROTOCOL_INHERIT,
    DSRTOS_PRIORITY_PROTOCOL_CEILING,
    DSRTOS_PRIORITY_PROTOCOL_PROTECT
} dsrtos_priority_protocol_t;

/* Priority inheritance chain node */
typedef struct dsrtos_pi_chain_node {
    dsrtos_tcb_t* task;
    uint8_t inherited_priority;
    struct dsrtos_pi_chain_node* next;
    uint32_t timestamp;
} dsrtos_pi_chain_node_t;

/* Priority information structure */
typedef struct {
    uint8_t base_priority;
    uint8_t current_priority;
    uint8_t ceiling_priority;
    uint8_t inherited_priority;
    dsrtos_priority_protocol_t protocol;
    uint32_t inheritance_count;
    uint32_t inheritance_depth;
    dsrtos_pi_chain_node_t* chain_head;
    void* blocking_mutex;
} dsrtos_priority_info_t;

/* Resource ceiling information */
typedef struct {
    uint32_t resource_id;
    uint8_t ceiling_priority;
    uint32_t owner_count;
    bool in_use;
} dsrtos_resource_ceiling_t;

/* Function prototypes */
dsrtos_error_t dsrtos_priority_mgmt_init(void);
dsrtos_error_t dsrtos_priority_set(dsrtos_tcb_t* task, uint8_t priority);
dsrtos_error_t dsrtos_priority_get(const dsrtos_tcb_t* task, uint8_t* priority);
dsrtos_error_t dsrtos_priority_boost(dsrtos_tcb_t* task, uint8_t boost_priority);
dsrtos_error_t dsrtos_priority_restore(dsrtos_tcb_t* task);

/* Priority inheritance functions */
dsrtos_error_t dsrtos_priority_inherit_apply(
    dsrtos_tcb_t* owner,
    dsrtos_tcb_t* requester
);
dsrtos_error_t dsrtos_priority_inherit_release(dsrtos_tcb_t* task);
dsrtos_error_t dsrtos_priority_inherit_chain_walk(
    dsrtos_tcb_t* start_task,
    uint8_t priority
);

/* Priority ceiling functions */
dsrtos_error_t dsrtos_priority_ceiling_set(void* resource, uint8_t ceiling);
dsrtos_error_t dsrtos_priority_ceiling_get(void* resource, uint8_t* ceiling);
dsrtos_error_t dsrtos_priority_ceiling_enter(dsrtos_tcb_t* task, void* resource);
dsrtos_error_t dsrtos_priority_ceiling_exit(dsrtos_tcb_t* task, void* resource);

/* Utility functions */
bool dsrtos_priority_is_valid(uint8_t priority);
uint8_t dsrtos_priority_normalize(uint8_t priority);
bool dsrtos_priority_inversion_detected(const dsrtos_tcb_t* task);
uint32_t dsrtos_priority_get_inheritance_depth(const dsrtos_tcb_t* task);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_PRIORITY_MGMT_H */
