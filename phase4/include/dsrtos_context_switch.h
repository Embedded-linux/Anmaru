/*
 * DSRTOS Context Switch Management
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_CONTEXT_SWITCH_H
#define DSRTOS_CONTEXT_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"

/* Forward declarations */
typedef struct dsrtos_tcb dsrtos_tcb_t;

/* Context switch state enumeration */
typedef enum {
    CONTEXT_STATE_IDLE = 0,
    CONTEXT_STATE_SAVE,
    CONTEXT_STATE_SWITCH,
    CONTEXT_STATE_RESTORE,
    CONTEXT_STATE_ERROR
} context_state_t;

/* Context switch statistics structure */
typedef struct {
    uint32_t switch_count;
    uint32_t switch_cycles_min;
    uint32_t switch_cycles_max;
    uint32_t switch_cycles_avg;
    uint32_t preemption_count;
    uint32_t voluntary_count;
} dsrtos_context_stats_t;

/* Context switch function declarations */
dsrtos_error_t dsrtos_context_switch_init(void);
dsrtos_error_t dsrtos_context_switch_perform(void* from_stack, void* to_stack);
dsrtos_error_t dsrtos_context_switch_get_stats(void);
void dsrtos_context_switch(dsrtos_tcb_t* next_task);
void dsrtos_context_switch_from_isr(void);
void dsrtos_switch_context_handler(uint32_t* current_sp);
dsrtos_error_t dsrtos_context_get_stats(dsrtos_context_stats_t* stats);
dsrtos_error_t dsrtos_context_reset_stats(void);
dsrtos_error_t dsrtos_context_init_task(dsrtos_tcb_t* task, void (*entry)(void*), void* param);
void dsrtos_context_dump(dsrtos_tcb_t* task);
context_state_t dsrtos_context_get_state(void);
bool dsrtos_context_in_switch(void);
void* dsrtos_context_save(dsrtos_tcb_t* task);
void* dsrtos_context_restore(dsrtos_tcb_t* task);
void dsrtos_start_first_task(void);
void dsrtos_panic(const char* reason);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CONTEXT_SWITCH_H */
