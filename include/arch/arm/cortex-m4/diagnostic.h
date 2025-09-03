/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Project
 *
 * File: diagnostic.h
 * Description: Diagnostic and error handling interface
 * Phase: 1 - Boot & Board Bring-up
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_DIAGNOSTIC_H
#define DSRTOS_DIAGNOSTIC_H

#include <stdint.h>
#include <stdbool.h>

/* Diagnostic error codes */
typedef enum {
    DIAGNOSTIC_ERROR_NONE = 0U,
    DIAGNOSTIC_ERROR_SELF_TEST,
    DIAGNOSTIC_ERROR_MEMORY_FAULT,
    DIAGNOSTIC_ERROR_BUS_FAULT,
    DIAGNOSTIC_ERROR_USAGE_FAULT,
    DIAGNOSTIC_ERROR_HARD_FAULT,
    DIAGNOSTIC_ERROR_STACK_OVERFLOW,
    DIAGNOSTIC_ERROR_DIV_BY_ZERO,
    DIAGNOSTIC_ERROR_UNALIGNED,
    DIAGNOSTIC_ERROR_UNHANDLED_INTERRUPT,
    DIAGNOSTIC_ERROR_WATCHDOG_TIMEOUT
} dsrtos_diagnostic_error_t;

/* Diagnostic log entry */
typedef struct {
    uint32_t timestamp;
    dsrtos_diagnostic_error_t error_code;
    uint32_t pc;
    uint32_t lr;
    uint32_t sp;
    uint32_t additional_data;
} dsrtos_diagnostic_entry_t;

/* Function prototypes */
void dsrtos_diagnostic_init(void);
void dsrtos_diagnostic_error(dsrtos_diagnostic_error_t error);
void dsrtos_diagnostic_log_error(uint32_t error_code);
void dsrtos_diagnostic_log_fault(uint32_t pc, uint32_t lr, uint32_t psr);
void dsrtos_diagnostic_log_memory_fault(uint32_t cfsr, uint32_t mmfar);
void dsrtos_diagnostic_log_bus_fault(uint32_t cfsr, uint32_t bfar);
void dsrtos_diagnostic_log_usage_fault(uint32_t cfsr);
void dsrtos_diagnostic_safe_mode(void);
bool dsrtos_diagnostic_is_recoverable(void);
void dsrtos_diagnostic_dump(void);

/* LED indicators for diagnostic status */
void dsrtos_diagnostic_led_init(void);
void dsrtos_diagnostic_led_error(void);
void dsrtos_diagnostic_led_heartbeat(void);

#endif /* DSRTOS_DIAGNOSTIC_H */
