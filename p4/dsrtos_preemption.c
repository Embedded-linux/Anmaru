/*
 * DSRTOS Preemption Management Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_preemption.h"
#include "dsrtos_critical.h"
#include "dsrtos_kernel.h"
#include "dsrtos_assert.h"
#include "dsrtos_trace.h"
#include "stm32f4xx.h"

/* Maximum allowed preemption disable time (microseconds) */
#define MAX_PREEMPTION_DISABLE_US  (1000U)

/* Global preemption state */
static dsrtos_preemption_state_t g_preemption_state = {
    .disable_count = 0U,
    .saved_primask = 0U,
    .preemption_enabled = true,
    .deferred_switch = false,
    .state = DSRTOS_PREEMPT_STATE_ENABLED,
    .disable_timestamp = 0U,
    .stats = {0}
};

/**
 * @brief Initialize preemption control
 */
dsrtos_error_t dsrtos_preemption_init(void)
{
    /* Initialize state */
    g_preemption_state.disable_count = 0U;
    g_preemption_state.saved_primask = 0U;
    g_preemption_state.preemption_enabled = true;
    g_preemption_state.deferred_switch = false;
    g_preemption_state.state = DSRTOS_PREEMPT_STATE_ENABLED;
    g_preemption_state.disable_timestamp = 0U;
    
    /* Clear statistics */
    g_preemption_state.stats.disable_count = 0U;
    g_preemption_state.stats.max_disable_depth = 0U;
    g_preemption_state.stats.total_disabled_time = 0U;
    g_preemption_state.stats.max_disabled_time = 0U;
    g_preemption_state.stats.deferred_switches = 0U;
    g_preemption_state.stats.forced_preemptions = 0U;
    
    DSRTOS_TRACE_PREEMPTION("Preemption control initialized");
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Disable preemption with nesting support
 */
void dsrtos_preemption_disable(void)
{
    uint32_t primask;
    uint32_t current_time;
    
    /* Save and disable interrupts */
    primask = __get_PRIMASK();
    __disable_irq();
    
    /* First disable - save state */
    if (g_preemption_state.disable_count == 0U) {
        g_preemption_state.saved_primask = primask;
        g_preemption_state.preemption_enabled = false;
        g_preemption_state.state = DSRTOS_PREEMPT_STATE_DISABLED;
        g_preemption_state.disable_timestamp = DWT->CYCCNT;
    }
    
    /* Increment disable count */
    g_preemption_state.disable_count++;
    g_preemption_state.stats.disable_count++;
    
    /* Track maximum nesting */
    if (g_preemption_state.disable_count > g_preemption_state.stats.max_disable_depth) {
        g_preemption_state.stats.max_disable_depth = g_preemption_state.disable_count;
    }
    
    /* Check for excessive nesting */
    if (g_preemption_state.disable_count > DSRTOS_MAX_PREEMPTION_DEPTH) {
        g_preemption_state.state = DSRTOS_PREEMPT_STATE_ERROR;
        DSRTOS_ASSERT(false, "Preemption disable depth exceeded");
    }
    
    /* Restore interrupts if they were enabled */
    if ((primask & 0x1U) == 0U) {
        __enable_irq();
    }
    
    DSRTOS_TRACE_PREEMPTION("Preemption disabled, depth=%u", 
                           g_preemption_state.disable_count);
}

/**
 * @brief Enable preemption with deferred switch handling
 */
void dsrtos_preemption_enable(void)
{
    uint32_t primask;
    uint32_t disabled_time;
    
    /* Save and disable interrupts */
    primask = __get_PRIMASK();
    __disable_irq();
    
    /* Check for underflow */
    if (g_preemption_state.disable_count == 0U) {
        g_preemption_state.state = DSRTOS_PREEMPT_STATE_ERROR;
        DSRTOS_ASSERT(false, "Preemption enable underflow");
        goto exit;
    }
    
    /* Decrement disable count */
    g_preemption_state.disable_count--;
    
    /* Last enable - restore preemption */
    if (g_preemption_state.disable_count == 0U) {
        /* Calculate disabled time */
        disabled_time = DWT->CYCCNT - g_preemption_state.disable_timestamp;
        g_preemption_state.stats.total_disabled_time += disabled_time;
        
        if (disabled_time > g_preemption_state.stats.max_disabled_time) {
            g_preemption_state.stats.max_disabled_time = disabled_time;
        }
        
        /* Check for excessive disable time */
        if (disabled_time > (SystemCoreClock / 1000000U * MAX_PREEMPTION_DISABLE_US)) {
            DSRTOS_TRACE_WARNING("Preemption disabled for %u cycles", disabled_time);
        }
        
        /* Re-enable preemption */
        g_preemption_state.preemption_enabled = true;
        g_preemption_state.state = DSRTOS_PREEMPT_STATE_ENABLED;
        
        /* Check for deferred switch */
        if (g_preemption_state.deferred_switch) {
            g_preemption_state.deferred_switch = false;
            g_preemption_state.stats.deferred_switches++;
            
            /* Trigger deferred context switch */
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
    
    DSRTOS_TRACE_PREEMPTION("Preemption enabled, depth=%u",
                           g_preemption_state.disable_count);

exit:
    /* Restore interrupts */
    if ((primask & 0x1U) == 0U) {
        __enable_irq();
    }
}

/**
 * @brief Check if preemption is enabled
 */
bool dsrtos_preemption_is_enabled(void)
{
    return g_preemption_state.preemption_enabled;
}

/**
 * @brief Force preemption point
 */
void dsrtos_preemption_point(void)
{
    if (g_preemption_state.preemption_enabled && 
        g_preemption_state.deferred_switch) {
        
        g_preemption_state.deferred_switch = false;
        g_preemption_state.stats.forced_preemptions++;
        
        /* Trigger context switch */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        
        DSRTOS_TRACE_PREEMPTION("Forced preemption point");
    }
}

/**
 * @brief Get preemption statistics
 */
dsrtos_error_t dsrtos_preemption_get_stats(dsrtos_preempt_stats_t* stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    *stats = g_preemption_state.stats;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get current preemption state
 */
dsrtos_preempt_state_t dsrtos_preemption_get_state(void)
{
    return g_preemption_state.state;
}
