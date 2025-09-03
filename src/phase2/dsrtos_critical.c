/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Critical Section Management Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_critical.h"
#include "dsrtos_kernel_init.h"
#include "dsrtos_panic.h"
#include "dsrtos_assert.h"
#include "dsrtos_error.h"
#include <string.h>
#include "stm32f4xx.h"
#include "stm32_compat.h"

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define CRITICAL_CYCLES_TO_US(cycles)  ((cycles) / (SystemCoreClock / 1000000U))
#define CRITICAL_US_TO_CYCLES(us)      ((us) * (SystemCoreClock / 1000000U))

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
/* Critical section control block - aligned for cache performance */
static dsrtos_critical_t g_critical_cb __attribute__((aligned(32))) = {
    .nesting_level = 0U,
    .saved_primask = 0U,
    .entry_timestamp = 0U,
    .type = DSRTOS_CRITICAL_TYPE_KERNEL,
    .owner = NULL,
    .stats = {
        .enter_count = 0U,
        .exit_count = 0U,
        .max_nesting = 0U,
        .max_duration_cycles = 0U,
        .total_duration_cycles = 0U,
        .violation_count = 0U,
        .timeout_count = 0U
    },
    .magic = DSRTOS_CRITICAL_MAGIC
};

/* Timeout configuration */
static uint32_t g_critical_timeout_cycles = 0U;

/* Per-CPU critical section tracking for SMP support */
#ifdef DSRTOS_SMP_ENABLED
static dsrtos_critical_t g_critical_per_cpu[DSRTOS_MAX_CPU_COUNT];
#endif

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static uint32_t critical_get_timestamp(void);
static void critical_update_stats(uint32_t duration_cycles);
static void critical_panic_handler(const char* msg);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize critical section management
 */
dsrtos_error_t dsrtos_critical_init(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Reset control block */
    memset(&g_critical_cb, 0, sizeof(g_critical_cb));
    
    /* Initialize control block */
    g_critical_cb.magic = DSRTOS_CRITICAL_MAGIC;
    g_critical_cb.nesting_level = 0U;
    g_critical_cb.saved_primask = 0U;
    g_critical_cb.type = DSRTOS_CRITICAL_TYPE_KERNEL;
    
    /* Set default timeout (100ms) */
    g_critical_timeout_cycles = CRITICAL_US_TO_CYCLES(DSRTOS_CRITICAL_TIMEOUT_MS * 1000U);
    
    /* Initialize per-CPU structures for SMP */
    #ifdef DSRTOS_SMP_ENABLED
    for (uint32_t cpu = 0U; cpu < DSRTOS_MAX_CPU_COUNT; cpu++) {
        memset(&g_critical_per_cpu[cpu], 0, sizeof(dsrtos_critical_t));
        g_critical_per_cpu[cpu].magic = DSRTOS_CRITICAL_MAGIC;
    }
    #endif
    
    /* Register with kernel */
    result = dsrtos_kernel_register_service(
        DSRTOS_SERVICE_CRITICAL_SECTION,
        &g_critical_cb
    );
    
    return result;
}

/**
 * @brief Enter critical section
 */
void dsrtos_critical_enter(void)
{
    /* Disable interrupts first */
    uint32_t primask = dsrtos_arch_disable_interrupts();
    
    /* Validate state */
    DSRTOS_ASSERT(g_critical_cb.magic == DSRTOS_CRITICAL_MAGIC);
    
    /* Check for overflow */
    if (g_critical_cb.nesting_level >= DSRTOS_CRITICAL_MAX_NESTING) {
        critical_panic_handler("Critical section nesting overflow");
        return;
    }
    
    /* First entry - save state and timestamp */
    if (g_critical_cb.nesting_level == 0U) {
        g_critical_cb.saved_primask = primask;
        g_critical_cb.entry_timestamp = critical_get_timestamp();
        
        /* Set owner if scheduler is running */
        dsrtos_kernel_t* kernel = dsrtos_kernel_get_kcb();
        if (kernel != NULL && kernel->state == DSRTOS_KERNEL_STATE_RUNNING) {
            /* Owner would be current task - implemented in Phase 3 */
            g_critical_cb.owner = NULL;
        }
    }
    
    /* Increment nesting level */
    g_critical_cb.nesting_level++;
    
    /* Update statistics */
    g_critical_cb.stats.enter_count++;
    if (g_critical_cb.nesting_level > g_critical_cb.stats.max_nesting) {
        g_critical_cb.stats.max_nesting = g_critical_cb.nesting_level;
    }
    
    /* Memory barrier to ensure all writes complete */
    __DSB();
    __ISB();
}

/**
 * @brief Exit critical section
 */
void dsrtos_critical_exit(void)
{
    /* Validate state */
    DSRTOS_ASSERT(g_critical_cb.magic == DSRTOS_CRITICAL_MAGIC);
    
    /* Check for underflow */
    if (g_critical_cb.nesting_level == 0U) {
        critical_panic_handler("Critical section underflow");
        return;
    }
    
    /* Decrement nesting level */
    g_critical_cb.nesting_level--;
    
    /* Last exit - restore state and update stats */
    if (g_critical_cb.nesting_level == 0U) {
        /* Calculate duration */
        uint32_t exit_timestamp = critical_get_timestamp();
        uint32_t duration_cycles = exit_timestamp - g_critical_cb.entry_timestamp;
        
        /* Update statistics */
        critical_update_stats(duration_cycles);
        
        /* Check for timeout violation */
        if (duration_cycles > g_critical_timeout_cycles) {
            g_critical_cb.stats.timeout_count++;
            g_critical_cb.stats.violation_count++;
        }
        
        /* Clear owner */
        g_critical_cb.owner = NULL;
        
        /* Memory barrier before restoring interrupts */
        __DSB();
        __ISB();
        
        /* Restore interrupt state */
        dsrtos_arch_restore_interrupts(g_critical_cb.saved_primask);
    }
    
    /* Update exit statistics */
    g_critical_cb.stats.exit_count++;
}

/**
 * @brief Enter critical section from ISR
 */
uint32_t dsrtos_critical_enter_isr(void)
{
    uint32_t basepri;
    
    /* Save and set BASEPRI to mask lower priority interrupts */
    __asm volatile (
        "mrs %0, basepri\n"
        "mov r1, %1\n"
        "msr basepri, r1"
        : "=r" (basepri)
        : "i" (DSRTOS_MAX_SYSCALL_INTERRUPT_PRIORITY)
        : "r1", "memory"
    );
    
    /* Memory barrier */
    __DSB();
    __ISB();
    
    /* Update ISR nesting statistics */
    g_critical_cb.stats.enter_count++;
    
    return basepri;
}

/**
 * @brief Exit critical section from ISR
 */
void dsrtos_critical_exit_isr(uint32_t saved_mask)
{
    /* Memory barrier */
    __DSB();
    __ISB();
    
    /* Restore BASEPRI */
    __asm volatile (
        "msr basepri, %0"
        :
        : "r" (saved_mask)
        : "memory"
    );
    
    /* Update statistics */
    g_critical_cb.stats.exit_count++;
}

/**
 * @brief Check if in critical section
 */
bool dsrtos_critical_is_active(void)
{
    bool active;
    
    /* Check both nesting level and interrupt state */
    active = (g_critical_cb.nesting_level > 0U) || 
             dsrtos_arch_interrupts_disabled();
    
    return active;
}

/**
 * @brief Get critical section nesting level
 */
uint32_t dsrtos_critical_get_nesting(void)
{
    return g_critical_cb.nesting_level;
}

/**
 * @brief Get critical section statistics
 */
dsrtos_error_t dsrtos_critical_get_stats(dsrtos_critical_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else {
        /* Copy statistics atomically */
        uint32_t primask = dsrtos_arch_disable_interrupts();
        *stats = g_critical_cb.stats;
        dsrtos_arch_restore_interrupts(primask);
    }
    
    return result;
}

/**
 * @brief Reset critical section statistics
 */
dsrtos_error_t dsrtos_critical_reset_stats(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check if in critical section */
    if (g_critical_cb.nesting_level > 0U) {
        result = DSRTOS_ERROR_INVALID_STATE;
    }
    else {
        /* Reset statistics */
        uint32_t primask = dsrtos_arch_disable_interrupts();
        memset(&g_critical_cb.stats, 0, sizeof(g_critical_cb.stats));
        dsrtos_arch_restore_interrupts(primask);
    }
    
    return result;
}

/**
 * @brief Validate critical section state
 */
dsrtos_error_t dsrtos_critical_validate(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check magic number */
    if (g_critical_cb.magic != DSRTOS_CRITICAL_MAGIC) {
        result = DSRTOS_ERROR_CORRUPTION;
    }
    /* Check nesting level */
    else if (g_critical_cb.nesting_level > DSRTOS_CRITICAL_MAX_NESTING) {
        result = DSRTOS_ERROR_INVALID_STATE;
    }
    /* Check statistics consistency */
    else if (g_critical_cb.stats.exit_count > g_critical_cb.stats.enter_count) {
        result = DSRTOS_ERROR_INVALID_STATE;
    }
    else {
        /* State is valid */
    }
    
    return result;
}

/**
 * @brief Set critical section timeout
 */
dsrtos_error_t dsrtos_critical_set_timeout(uint32_t timeout_ms)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check if in critical section */
    if (g_critical_cb.nesting_level > 0U) {
        result = DSRTOS_ERROR_INVALID_STATE;
    }
    /* Validate timeout range */
    else if (timeout_ms == 0U || timeout_ms > 10000U) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else {
        /* Set new timeout */
        g_critical_timeout_cycles = CRITICAL_US_TO_CYCLES(timeout_ms * 1000U);
    }
    
    return result;
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Get current timestamp for timing measurements
 */
static uint32_t critical_get_timestamp(void)
{
    /* Use DWT cycle counter if available */
    #ifdef DWT
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) != 0U) {
        return DWT->CYCCNT;
    }
    #endif
    
    /* Fallback to SysTick */
    return (0xFFFFFFU - SysTick->VAL);
}

/* Function removed - not used in current implementation */

/**
 * @brief Update critical section statistics
 */
static void critical_update_stats(uint32_t duration_cycles)
{
    /* Update maximum duration */
    if (duration_cycles > g_critical_cb.stats.max_duration_cycles) {
        g_critical_cb.stats.max_duration_cycles = duration_cycles;
    }
    
    /* Update total duration */
    g_critical_cb.stats.total_duration_cycles += duration_cycles;
    
    /* Check for violations */
    if (duration_cycles > g_critical_timeout_cycles) {
        g_critical_cb.stats.violation_count++;
    }
}

/* Function removed - not used in current implementation */

/**
 * @brief Critical section panic handler
 */
static void critical_panic_handler(const char* msg)
{
    /* Disable all interrupts */
    __disable_irq();
    
    /* Call kernel panic handler */
    dsrtos_panic(DSRTOS_PANIC_CRITICAL_SECTION, msg, __FILE__, __LINE__);
    
    /* Should not return */
    while (1) {
        __NOP();
    }
}
