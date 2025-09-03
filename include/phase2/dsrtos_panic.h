/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Panic Handler Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_PANIC_H
#define DSRTOS_PANIC_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"

/*=============================================================================
 * PANIC REASON CODES
 *============================================================================*/
typedef enum {
    DSRTOS_PANIC_NONE                = 0x0000U,
    /* Hardware Faults */
    DSRTOS_PANIC_HARD_FAULT          = 0x0001U,
    DSRTOS_PANIC_MEM_FAULT           = 0x0002U,
    DSRTOS_PANIC_BUS_FAULT           = 0x0003U,
    DSRTOS_PANIC_USAGE_FAULT         = 0x0004U,
    DSRTOS_PANIC_NMI                 = 0x0005U,
    
    /* Kernel Errors */
    DSRTOS_PANIC_KERNEL_ERROR        = 0x0010U,
    DSRTOS_PANIC_STACK_OVERFLOW      = 0x0011U,
    DSRTOS_PANIC_HEAP_CORRUPTION     = 0x0012U,
    DSRTOS_PANIC_NULL_POINTER        = 0x0013U,
    DSRTOS_PANIC_INVALID_STATE       = 0x0014U,
    DSRTOS_PANIC_CRITICAL_SECTION    = 0x0015U,
    
    /* Task Errors */
    DSRTOS_PANIC_TASK_ERROR          = 0x0020U,
    DSRTOS_PANIC_TASK_STACK_OVERFLOW = 0x0021U,
    DSRTOS_PANIC_TASK_DEADLINE_MISS  = 0x0022U,
    DSRTOS_PANIC_TASK_INVALID        = 0x0023U,
    
    /* Resource Errors */
    DSRTOS_PANIC_RESOURCE_ERROR      = 0x0030U,
    DSRTOS_PANIC_DEADLOCK            = 0x0031U,
    DSRTOS_PANIC_PRIORITY_INVERSION  = 0x0032U,
    DSRTOS_PANIC_MUTEX_ERROR         = 0x0033U,
    
    /* System Errors */
    DSRTOS_PANIC_WATCHDOG            = 0x0040U,
    DSRTOS_PANIC_ASSERTION_FAILED    = 0x0041U,
    DSRTOS_PANIC_CORRUPTION          = 0x0042U,
    DSRTOS_PANIC_CONFIGURATION       = 0x0043U,
    
    /* User Defined */
    DSRTOS_PANIC_USER_DEFINED        = 0x0100U,
    
    DSRTOS_PANIC_MAX                 = 0xFFFFU
} dsrtos_panic_reason_t;

/*=============================================================================
 * PANIC CONTEXT STRUCTURE
 *============================================================================*/
typedef struct {
    /* Panic Information */
    dsrtos_panic_reason_t reason;      /* Panic reason code */
    const char* message;                /* Panic message string */
    const char* file;                   /* Source file */
    uint32_t line;                      /* Line number */
    uint32_t timestamp;                 /* Panic timestamp */
    
    /* CPU Context at Panic */
    struct {
        uint32_t r0, r1, r2, r3;       /* General purpose registers */
        uint32_t r4, r5, r6, r7;
        uint32_t r8, r9, r10, r11;
        uint32_t r12;                   /* IP */
        uint32_t sp;                    /* Stack pointer */
        uint32_t lr;                    /* Link register */
        uint32_t pc;                    /* Program counter */
        uint32_t psr;                   /* Program status register */
    } cpu_context;
    
    /* Fault Status Registers */
    struct {
        uint32_t cfsr;                  /* Configurable Fault Status */
        uint32_t hfsr;                  /* Hard Fault Status */
        uint32_t dfsr;                  /* Debug Fault Status */
        uint32_t mmfar;                 /* MemManage Fault Address */
        uint32_t bfar;                  /* BusFault Address */
        uint32_t afsr;                  /* Auxiliary Fault Status */
    } fault_status;
    
    /* System State */
    struct {
        void* current_task;             /* Current task TCB */
        uint32_t kernel_state;          /* Kernel state */
        uint32_t critical_nesting;      /* Critical section nesting */
        uint32_t interrupt_nesting;     /* Interrupt nesting */
    } system_state;
    
    /* Stack Information */
    struct {
        uint32_t* sp_main;              /* Main stack pointer */
        uint32_t* sp_process;           /* Process stack pointer */
        uint32_t stack_size;            /* Stack size */
        uint32_t stack_used;            /* Stack used */
    } stack_info;
    
} dsrtos_panic_context_t;

/*=============================================================================
 * PANIC HANDLER CONFIGURATION
 *============================================================================*/
typedef struct {
    bool auto_restart;                  /* Auto restart after panic */
    uint32_t restart_delay_ms;          /* Restart delay in milliseconds */
    bool dump_to_console;               /* Dump panic info to console */
    bool save_to_flash;                 /* Save panic info to flash */
    uint32_t flash_address;             /* Flash address for panic log */
    uint32_t max_panics;                /* Maximum panics before halt */
} dsrtos_panic_config_t;

/*=============================================================================
 * PANIC HANDLER CALLBACK
 *============================================================================*/
typedef void (*dsrtos_panic_handler_t)(const dsrtos_panic_context_t* context);

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize panic handler
 * 
 * @param[in] config Panic handler configuration
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-PANIC-001: Initialization
 * @safety Must be called during kernel initialization
 */
dsrtos_error_t dsrtos_panic_init(const dsrtos_panic_config_t* config);

/**
 * @brief Trigger system panic
 * 
 * @param[in] reason Panic reason code
 * @param[in] message Panic message (can be NULL)
 * @param[in] file Source file name
 * @param[in] line Source line number
 * 
 * @requirements REQ-PANIC-002: Panic trigger
 * @safety This function does not return
 */
void dsrtos_panic(dsrtos_panic_reason_t reason, 
                  const char* message,
                  const char* file,
                  uint32_t line) __attribute__((noreturn));

/**
 * @brief Register custom panic handler
 * 
 * @param[in] handler Custom panic handler function
 * @return Previous handler or NULL
 * 
 * @requirements REQ-PANIC-003: Custom handler
 * @safety Handler must not return
 */
dsrtos_panic_handler_t dsrtos_panic_register_handler(dsrtos_panic_handler_t handler);

/**
 * @brief Get last panic context
 * 
 * @param[out] context Pointer to context structure
 * @return DSRTOS_SUCCESS if panic occurred, DSRTOS_ERROR_NOT_FOUND otherwise
 * 
 * @requirements REQ-PANIC-004: Panic history
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_panic_get_last(dsrtos_panic_context_t* context);

/**
 * @brief Clear panic history
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-PANIC-005: History clear
 * @safety Should be called after analyzing panic
 */
dsrtos_error_t dsrtos_panic_clear_history(void);

/**
 * @brief Get panic count
 * 
 * @return Number of panics since boot
 * 
 * @requirements REQ-PANIC-006: Panic statistics
 * @safety Thread-safe
 */
uint32_t dsrtos_panic_get_count(void);

/**
 * @brief Dump panic information
 * 
 * @param[in] context Panic context to dump
 * 
 * @requirements REQ-PANIC-007: Debug output
 * @safety May be called from panic handler
 */
void dsrtos_panic_dump(const dsrtos_panic_context_t* context);

/**
 * @brief Save panic context to persistent storage
 * 
 * @param[in] context Panic context to save
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-PANIC-008: Persistent storage
 * @safety Must complete quickly
 */
dsrtos_error_t dsrtos_panic_save(const dsrtos_panic_context_t* context);

/**
 * @brief Load panic context from persistent storage
 * 
 * @param[out] context Loaded panic context
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-PANIC-009: Recovery
 * @safety Call during initialization
 */
dsrtos_error_t dsrtos_panic_load(dsrtos_panic_context_t* context);

/*=============================================================================
 * FAULT HANDLER DECLARATIONS
 *============================================================================*/

/**
 * @brief Hard fault handler
 */
void HardFault_Handler(void) __attribute__((naked));

/**
 * @brief Hard fault handler C implementation
 * @param sp Stack pointer at fault
 */
void hard_fault_handler_c(uint32_t* sp);

/**
 * @brief Memory management fault handler
 */
void MemManage_Handler(void) __attribute__((naked));

/**
 * @brief Bus fault handler
 */
void BusFault_Handler(void) __attribute__((naked));

/**
 * @brief Usage fault handler
 */
void UsageFault_Handler(void) __attribute__((naked));

/*=============================================================================
 * PANIC MACROS
 *============================================================================*/

/**
 * @brief Panic with message
 */
#define DSRTOS_PANIC(reason, msg) \
    dsrtos_panic((reason), (msg), __FILE__, __LINE__)

/**
 * @brief Panic without message
 */
#define DSRTOS_PANIC_NO_MSG(reason) \
    dsrtos_panic((reason), NULL, __FILE__, __LINE__)

/**
 * @brief Conditional panic
 */
#define DSRTOS_PANIC_IF(condition, reason, msg) \
    do { \
        if (condition) { \
            dsrtos_panic((reason), (msg), __FILE__, __LINE__); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_PANIC_H */
