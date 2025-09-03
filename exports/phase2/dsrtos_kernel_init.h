/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Kernel Core Initialization Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_KERNEL_INIT_H
#define DSRTOS_KERNEL_INIT_H

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
 * MISRA-C:2012 COMPLIANCE DEVIATIONS
 *============================================================================*/
/* 
 * Deviation: Dir 4.9 - Function-like macros used for compile-time configuration
 * Justification: Required for efficient kernel configuration validation
 */

/*=============================================================================
 * KERNEL VERSION INFORMATION
 *============================================================================*/
#ifndef DSRTOS_VERSION_MAJOR
#endif
#ifndef DSRTOS_VERSION_MAJOR
#define DSRTOS_VERSION_MAJOR        1U
#endif
#endif
#ifndef DSRTOS_VERSION_MINOR
#define DSRTOS_VERSION_MINOR        0U
#endif
#ifndef DSRTOS_VERSION_PATCH
#define DSRTOS_VERSION_PATCH        0U
#endif
#define DSRTOS_VERSION_BUILD        1U

/* Version string format: "MAJOR.MINOR.PATCH-BUILD" */
#define DSRTOS_VERSION_STRING       "1.0.0-1"

/* Kernel magic numbers for validation */
#define DSRTOS_KERNEL_MAGIC         0x44535254U  /* 'DSRT' */
#define DSRTOS_KERNEL_INIT_MAGIC    0x494E4954U  /* 'INIT' */

/*=============================================================================
 * KERNEL STATE DEFINITIONS
 *============================================================================*/
typedef enum {
    DSRTOS_KERNEL_STATE_UNINITIALIZED = 0x00U,
    DSRTOS_KERNEL_STATE_INITIALIZING  = 0x01U,
    DSRTOS_KERNEL_STATE_READY         = 0x02U,
    DSRTOS_KERNEL_STATE_RUNNING       = 0x03U,
    DSRTOS_KERNEL_STATE_SUSPENDED     = 0x04U,
    DSRTOS_KERNEL_STATE_ERROR         = 0x05U,
    DSRTOS_KERNEL_STATE_SHUTDOWN      = 0x06U,
    DSRTOS_KERNEL_STATE_MAX           = 0x07U
} dsrtos_kernel_state_t;

/*=============================================================================
 * KERNEL INITIALIZATION PHASES
 *============================================================================*/
typedef enum {
    DSRTOS_INIT_PHASE_HARDWARE    = 0x00U,
    DSRTOS_INIT_PHASE_MEMORY      = 0x01U,
    DSRTOS_INIT_PHASE_INTERRUPTS  = 0x02U,
    DSRTOS_INIT_PHASE_SCHEDULER   = 0x03U,
    DSRTOS_INIT_PHASE_SERVICES    = 0x04U,
    DSRTOS_INIT_PHASE_DRIVERS     = 0x05U,
    DSRTOS_INIT_PHASE_APPLICATION = 0x06U,
    DSRTOS_INIT_PHASE_MAX         = 0x07U
} dsrtos_init_phase_t;

/*=============================================================================
 * KERNEL SERVICES IDENTIFIERS
 *============================================================================*/
typedef enum {
    DSRTOS_SERVICE_CRITICAL_SECTION = 0x00U,
    DSRTOS_SERVICE_SYSCALL         = 0x01U,
    DSRTOS_SERVICE_ERROR_HANDLER   = 0x02U,
    DSRTOS_SERVICE_STATISTICS      = 0x03U,
    DSRTOS_SERVICE_HOOKS           = 0x04U,
    DSRTOS_SERVICE_MEMORY          = 0x05U,
    DSRTOS_SERVICE_TIME            = 0x06U,
    DSRTOS_SERVICE_DEBUG           = 0x07U,
    DSRTOS_SERVICE_MAX             = 0x08U
} dsrtos_service_id_t;

/*=============================================================================
 * KERNEL CONFIGURATION STRUCTURE
 *============================================================================*/
typedef struct {
    /* System Configuration */
    uint32_t cpu_frequency_hz;         /* CPU clock frequency */
    uint32_t systick_frequency_hz;     /* System tick frequency */
    uint32_t heap_size_bytes;          /* Heap size in bytes */
    uint32_t stack_size_words;         /* Main stack size in words */
    
    /* Task Configuration */
    uint16_t max_tasks;                /* Maximum number of tasks */
    uint16_t max_priorities;           /* Number of priority levels */
    
    /* Resource Configuration */
    uint16_t max_mutexes;              /* Maximum number of mutexes */
    uint16_t max_semaphores;           /* Maximum number of semaphores */
    uint16_t max_queues;               /* Maximum number of queues */
    uint16_t max_timers;               /* Maximum number of timers */
    
    /* Feature Flags */
    uint32_t features_enabled;         /* Enabled kernel features bitmap */
    
    /* Safety Configuration */
    bool stack_overflow_check;         /* Enable stack overflow checking */
    bool memory_protection;            /* Enable memory protection unit */
    bool deadline_monitoring;          /* Enable deadline monitoring */
    bool error_logging;                /* Enable error logging */
    
    /* Validation */
    uint32_t config_magic;             /* Configuration magic number */
    uint32_t config_checksum;          /* Configuration checksum */
} dsrtos_kernel_config_t;

/*=============================================================================
 * KERNEL CONTROL BLOCK
 *============================================================================*/
typedef struct {
    /* Kernel State */
    dsrtos_kernel_state_t state;       /* Current kernel state */
    dsrtos_init_phase_t init_phase;    /* Current initialization phase */
    
    /* Configuration */
    const dsrtos_kernel_config_t* config; /* Kernel configuration */
    
    /* Service Registry */
    void* services[DSRTOS_SERVICE_MAX]; /* Service pointers */
    uint32_t services_active;           /* Active services bitmap */
    
    /* System Information */
    uint64_t uptime_ticks;              /* System uptime in ticks */
    uint64_t idle_ticks;                /* Idle time in ticks */
    uint32_t context_switches;          /* Total context switches */
    
    /* Error Information */
    uint32_t error_count;               /* Total error count */
    dsrtos_error_t last_error;         /* Last error code */
    
    /* Critical Section */
    uint32_t critical_nesting;         /* Critical section nesting level */
    uint32_t interrupt_nesting;        /* Interrupt nesting level */
    
    /* Validation */
    uint32_t magic_number;              /* Kernel magic number */
    uint32_t checksum;                  /* Kernel checksum */
} dsrtos_kernel_t;

/*=============================================================================
 * KERNEL INITIALIZATION CALLBACK TYPES
 *============================================================================*/
typedef dsrtos_error_t (*dsrtos_init_callback_t)(void);
typedef void (*dsrtos_service_callback_t)(void* params);
typedef void (*dsrtos_error_callback_t)(dsrtos_error_t error, void* context);

/*=============================================================================
 * KERNEL INITIALIZATION TABLE ENTRY
 *============================================================================*/
typedef struct {
    dsrtos_init_phase_t phase;         /* Initialization phase */
    dsrtos_init_callback_t callback;   /* Initialization callback */
    const char* name;                  /* Component name */
    uint32_t timeout_ms;                /* Initialization timeout */
    bool critical;                      /* Critical component flag */
} dsrtos_init_entry_t;

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize the DSRTOS kernel
 * 
 * @param[in] config Pointer to kernel configuration
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-KERNEL-001: Kernel initialization
 * @safety This function shall be called exactly once at system startup
 */
dsrtos_error_t dsrtos_kernel_init(const dsrtos_kernel_config_t* config);

/**
 * @brief Start the DSRTOS kernel
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * @note This function does not return on success
 * 
 * @requirements REQ-KERNEL-002: Kernel startup
 * @safety Critical section protection required
 */
dsrtos_error_t dsrtos_kernel_start(void);

/**
 * @brief Shutdown the DSRTOS kernel
 * 
 * @param[in] reason Shutdown reason code
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-KERNEL-003: Kernel shutdown
 * @safety Safe shutdown sequence must be followed
 */
dsrtos_error_t dsrtos_kernel_shutdown(uint32_t reason);

/**
 * @brief Get current kernel state
 * 
 * @return Current kernel state
 * 
 * @requirements REQ-KERNEL-004: State query
 * @safety Thread-safe, may be called from ISR
 */
dsrtos_kernel_state_t dsrtos_kernel_get_state(void);

/**
 * @brief Register a kernel service
 * 
 * @param[in] service_id Service identifier
 * @param[in] service_ptr Service structure pointer
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-KERNEL-005: Service registration
 * @safety Must be called during initialization phase
 */
dsrtos_error_t dsrtos_kernel_register_service(
    dsrtos_service_id_t service_id,
    void* service_ptr
);

/**
 * @brief Get a registered kernel service
 * 
 * @param[in] service_id Service identifier
 * @return Service pointer or NULL if not registered
 * 
 * @requirements REQ-KERNEL-006: Service access
 * @safety Thread-safe after initialization
 */
void* dsrtos_kernel_get_service(dsrtos_service_id_t service_id);

/**
 * @brief Register initialization callback
 * 
 * @param[in] entry Initialization entry
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-KERNEL-007: Init callback registration
 * @safety Must be called before kernel initialization
 */
dsrtos_error_t dsrtos_kernel_register_init(const dsrtos_init_entry_t* entry);

/**
 * @brief Get kernel version information
 * 
 * @param[out] major Major version number
 * @param[out] minor Minor version number
 * @param[out] patch Patch version number
 * @param[out] build Build number
 * 
 * @requirements REQ-KERNEL-008: Version information
 * @safety Thread-safe, may be called from ISR
 */
void dsrtos_kernel_get_version(
    uint32_t* major,
    uint32_t* minor,
    uint32_t* patch,
    uint32_t* build
);

/**
 * @brief Get kernel statistics
 * 
 * @param[out] uptime System uptime in ticks
 * @param[out] idle_time Idle time in ticks
 * @param[out] context_switches Number of context switches
 * 
 * @requirements REQ-KERNEL-009: Statistics
 * @safety Thread-safe, may be called from ISR
 */
void dsrtos_kernel_get_stats(
    uint64_t* uptime,
    uint64_t* idle_time,
    uint32_t* context_switches
);

/**
 * @brief Validate kernel configuration
 * 
 * @param[in] config Configuration to validate
 * @return DSRTOS_SUCCESS if valid, error code otherwise
 * 
 * @requirements REQ-KERNEL-010: Configuration validation
 * @safety Pure function, no side effects
 */
dsrtos_error_t dsrtos_kernel_validate_config(
    const dsrtos_kernel_config_t* config
);

/**
 * @brief Perform kernel self-test
 * 
 * @return DSRTOS_SUCCESS if all tests pass, error code otherwise
 * 
 * @requirements REQ-KERNEL-011: Self-test
 * @safety Must be called during initialization
 */
dsrtos_error_t dsrtos_kernel_self_test(void);

/**
 * @brief Get kernel control block
 * 
 * @return Pointer to kernel control block
 * 
 * @requirements REQ-KERNEL-012: Kernel access
 * @safety Internal use only, not for application use
 */
dsrtos_kernel_t* dsrtos_kernel_get_kcb(void);

/*=============================================================================
 * INLINE FUNCTIONS
 *============================================================================*/

/**
 * @brief Check if kernel is running
 * 
 * @return true if kernel is running, false otherwise
 */
static inline bool dsrtos_kernel_is_running(void)
{
    return (dsrtos_kernel_get_state() == DSRTOS_KERNEL_STATE_RUNNING);
}

/**
 * @brief Check if kernel is initialized
 * 
 * @return true if kernel is initialized, false otherwise
 */
static inline bool dsrtos_kernel_is_initialized(void)
{
    dsrtos_kernel_state_t state = dsrtos_kernel_get_state();
    return (state >= DSRTOS_KERNEL_STATE_READY) && 
           (state < DSRTOS_KERNEL_STATE_SHUTDOWN);
}

#ifdef __cplusplus
}
#endif

