/*
 * DSRTOS Preemption Management
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_PREEMPTION_H
#define DSRTOS_PREEMPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"

/* Preemption control states */
typedef enum {
    DSRTOS_PREEMPT_STATE_ENABLED = 0,
    DSRTOS_PREEMPT_STATE_DISABLED,
    DSRTOS_PREEMPT_STATE_CRITICAL,
    DSRTOS_PREEMPT_STATE_ERROR
} dsrtos_preempt_state_t;

/* Preemption statistics */
typedef struct {
    uint32_t disable_count;
    uint32_t max_disable_depth;
    uint32_t total_disabled_time;
    uint32_t max_disabled_time;
    uint32_t deferred_switches;
    uint32_t forced_preemptions;
} dsrtos_preempt_stats_t;

/* Preemption control block */
typedef struct {
    volatile uint32_t disable_count;
    volatile uint32_t saved_primask;
    volatile bool preemption_enabled;
    volatile bool deferred_switch;
    dsrtos_preempt_state_t state;
    uint32_t disable_timestamp;
    dsrtos_preempt_stats_t stats;
} dsrtos_preemption_state_t;

/* Function prototypes */
dsrtos_error_t dsrtos_preemption_init(void);
void dsrtos_preemption_disable(void);
void dsrtos_preemption_enable(void);
bool dsrtos_preemption_is_enabled(void);
void dsrtos_preemption_point(void);
dsrtos_error_t dsrtos_preemption_get_stats(dsrtos_preempt_stats_t* stats);
dsrtos_preempt_state_t dsrtos_preemption_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_PREEMPTION_H */
