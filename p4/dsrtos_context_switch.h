/*
 * DSRTOS Context Switch Interface
 * 
 * Copyright (C) 2024 DSRTOS
 * 
 * Provides architecture-specific context switching for ARM Cortex-M4.
 * 
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_CONTEXT_SWITCH_H
#define DSRTOS_CONTEXT_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dsrtos_types.h"
#include "dsrtos_task.h"

/*==============================================================================
 * ARM CORTEX-M4 CONTEXT DEFINITIONS
 *============================================================================*/

/* CPU context saved on stack during context switch */
typedef struct {
    /* Hardware stacked registers (automatically saved by CPU) */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;     /* Link register */
    uint32_t pc;     /* Program counter */
    uint32_t xpsr;   /* Program status register */
    
    /* Software stacked registers (manually saved) */
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    
    /* FPU registers if enabled */
#ifdef DSRTOS_USE_FPU
    uint32_t s16_s31[16];  /* FPU registers S16-S31 */
#endif
} dsrtos_cpu_context_t;

/* Context switch statistics */
typedef struct {
    uint32_t switch_count;
    uint32_t switch_cycles_min;
    uint32_t switch_cycles_max;
    uint32_t switch_cycles_avg;
    uint32_t preemption_count;
    uint32_t voluntary_count;
} dsrtos_context_stats_t;

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize context switching subsystem
 * 
 * @return DSRTOS_SUCCESS or error code
 * 
 * @req REQ-CTX-001: Context switch initialization
 * @safety Sets up PendSV for lowest priority
 */
dsrtos_error_t dsrtos_context_switch_init(void);

/**
 * @brief Trigger context switch to next task
 * 
 * @param next_task Task to switch to
 * 
 * @req REQ-CTX-002: Context switch trigger
 * @safety Uses PendSV for safe switching
 */
void dsrtos_context_switch(dsrtos_tcb_t* next_task);

/**
 * @brief Perform immediate context switch (from ISR)
 * 
 * @req REQ-CTX-003: ISR context switch
 * @safety Only callable from ISR context
 */
void dsrtos_context_switch_from_isr(void);

/**
 * @brief Save current task context
 * 
 * @param task Task whose context to save
 * @return Stack pointer after save
 * 
 * @req REQ-CTX-004: Context save
 * @safety Preserves all registers
 */
void* dsrtos_context_save(dsrtos_tcb_t* task);

/**
 * @brief Restore task context
 * 
 * @param task Task whose context to restore
 * @return Stack pointer for restoration
 * 
 * @req REQ-CTX-005: Context restore
 * @safety Validates stack before restore
 */
void* dsrtos_context_restore(dsrtos_tcb_t* task);

/**
 * @brief Get context switch statistics
 * 
 * @param stats Statistics buffer
 * @return DSRTOS_SUCCESS or error code
 * 
 * @req REQ-STATS-003: Context statistics
 */
dsrtos_error_t dsrtos_context_get_stats(dsrtos_context_stats_t* stats);

/**
 * @brief Assembly functions (implemented in context_switch.S)
 */
extern void PendSV_Handler(void);
extern void dsrtos_start_first_task(void);
extern void dsrtos_switch_context(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CONTEXT_SWITCH_H */
