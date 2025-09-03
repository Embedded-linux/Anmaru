/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Critical Section Management Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_CRITICAL_H
#define DSRTOS_CRITICAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_config.h"
#include "dsrtos_error.h"

/*=============================================================================
 * CRITICAL SECTION CONFIGURATION
 *============================================================================*/
#define DSRTOS_CRITICAL_MAX_NESTING    255U
#define DSRTOS_CRITICAL_TIMEOUT_MS     100U
#define DSRTOS_CRITICAL_MAGIC          0x43524954U  /* 'CRIT' */

/*=============================================================================
 * CRITICAL SECTION TYPES
 *============================================================================*/
typedef enum {
    DSRTOS_CRITICAL_TYPE_KERNEL = 0x00U,
    DSRTOS_CRITICAL_TYPE_ISR    = 0x01U,
    DSRTOS_CRITICAL_TYPE_TASK   = 0x02U,
    DSRTOS_CRITICAL_TYPE_DRIVER = 0x03U,
    DSRTOS_CRITICAL_TYPE_MAX    = 0x04U
} dsrtos_critical_type_t;

/*=============================================================================
 * CRITICAL SECTION STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t enter_count;              /* Total entries */
    uint32_t exit_count;               /* Total exits */
    uint32_t max_nesting;              /* Maximum nesting level */
    uint32_t max_duration_cycles;      /* Maximum duration in CPU cycles */
    uint32_t total_duration_cycles;    /* Total time in critical sections */
    uint32_t violation_count;          /* Timing violations */
    uint32_t timeout_count;            /* Timeout occurrences */
} dsrtos_critical_stats_t;

/*=============================================================================
 * CRITICAL SECTION CONTROL BLOCK
 *============================================================================*/
typedef struct {
    uint32_t nesting_level;            /* Current nesting level */
    uint32_t saved_primask;            /* Saved interrupt state */
    uint32_t entry_timestamp;          /* Entry timestamp */
    dsrtos_critical_type_t type;      /* Critical section type */
    void* owner;                       /* Owner (task/ISR) */
    dsrtos_critical_stats_t stats;    /* Statistics */
    uint32_t magic;                    /* Magic number for validation */
} dsrtos_critical_t;

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize critical section management
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-CRITICAL-001: Initialization
 * @safety Must be called during kernel initialization
 */
dsrtos_error_t dsrtos_critical_init(void);

/**
 * @brief Enter critical section
 * 
 * @requirements REQ-CRITICAL-002: Critical section entry
 * @safety Disables interrupts, must be paired with exit
 */
void dsrtos_critical_enter(void);

/**
 * @brief Exit critical section
 * 
 * @requirements REQ-CRITICAL-003: Critical section exit
 * @safety Restores interrupt state, must be paired with enter
 */
void dsrtos_critical_exit(void);

/**
 * @brief Enter critical section from ISR
 * 
 * @return Saved interrupt priority mask
 * 
 * @requirements REQ-CRITICAL-004: ISR critical section
 * @safety For use in ISR context only
 */
uint32_t dsrtos_critical_enter_isr(void);

/**
 * @brief Exit critical section from ISR
 * 
 * @param[in] saved_mask Saved interrupt priority mask
 * 
 * @requirements REQ-CRITICAL-005: ISR critical exit
 * @safety For use in ISR context only
 */
void dsrtos_critical_exit_isr(uint32_t saved_mask);

/**
 * @brief Check if in critical section
 * 
 * @return true if in critical section, false otherwise
 * 
 * @requirements REQ-CRITICAL-006: State query
 * @safety Thread-safe, may be called from ISR
 */
bool dsrtos_critical_is_active(void);

/**
 * @brief Get critical section nesting level
 * 
 * @return Current nesting level
 * 
 * @requirements REQ-CRITICAL-007: Nesting query
 * @safety Thread-safe, may be called from ISR
 */
uint32_t dsrtos_critical_get_nesting(void);

/**
 * @brief Get critical section statistics
 * 
 * @param[out] stats Pointer to statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-CRITICAL-008: Statistics
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_critical_get_stats(dsrtos_critical_stats_t* stats);

/**
 * @brief Reset critical section statistics
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-CRITICAL-009: Statistics reset
 * @safety Should be called from non-critical context
 */
dsrtos_error_t dsrtos_critical_reset_stats(void);

/**
 * @brief Validate critical section state
 * 
 * @return DSRTOS_SUCCESS if valid, error code otherwise
 * 
 * @requirements REQ-CRITICAL-010: Validation
 * @safety For diagnostic use
 */
dsrtos_error_t dsrtos_critical_validate(void);

/**
 * @brief Set critical section timeout
 * 
 * @param[in] timeout_ms Timeout in milliseconds
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-CRITICAL-011: Timeout configuration
 * @safety Must be called before entering critical section
 */
dsrtos_error_t dsrtos_critical_set_timeout(uint32_t timeout_ms);

/*=============================================================================
 * INLINE FUNCTIONS FOR PERFORMANCE
 *============================================================================*/

/**
 * @brief Disable interrupts (architecture-specific)
 */
static inline uint32_t dsrtos_arch_disable_interrupts(void)
{
    uint32_t primask;
    __asm volatile (
        "mrs %0, primask\n"
        "cpsid i"
        : "=r" (primask)
        :
        : "memory"
    );
    return primask;
}

/**
 * @brief Enable interrupts (architecture-specific)
 */
static inline void dsrtos_arch_enable_interrupts(void)
{
    __asm volatile (
        "cpsie i"
        :
        :
        : "memory"
    );
}

/**
 * @brief Restore interrupt state (architecture-specific)
 */
static inline void dsrtos_arch_restore_interrupts(uint32_t primask)
{
    __asm volatile (
        "msr primask, %0"
        :
        : "r" (primask)
        : "memory"
    );
}

/**
 * @brief Get current interrupt state
 */
static inline uint32_t dsrtos_arch_get_interrupt_state(void)
{
    uint32_t primask;
    __asm volatile (
        "mrs %0, primask"
        : "=r" (primask)
        :
        :
    );
    return primask;
}

/**
 * @brief Check if interrupts are disabled
 */
static inline bool dsrtos_arch_interrupts_disabled(void)
{
    return (dsrtos_arch_get_interrupt_state() & 0x1U) != 0U;
}

/*=============================================================================
 * CRITICAL SECTION MACROS
 *============================================================================*/

/* MISRA-C:2012 Dir 4.9 deviation: Function-like macros for efficiency */

/**
 * @brief Critical section block macro
 * Usage: DSRTOS_CRITICAL_SECTION { ... protected code ... }
 */
#define DSRTOS_CRITICAL_SECTION \
    for (uint32_t _cs_lock = (dsrtos_critical_enter(), 1U); \
         _cs_lock; \
         _cs_lock = 0U, dsrtos_critical_exit())

/**
 * @brief ISR critical section block macro
 */
#define DSRTOS_CRITICAL_SECTION_ISR(saved) \
    for (uint32_t _cs_lock = 1U, saved = dsrtos_critical_enter_isr(); \
         _cs_lock; \
         _cs_lock = 0U, dsrtos_critical_exit_isr(saved))

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CRITICAL_H */
