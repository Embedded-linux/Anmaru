/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Project
 *
 * File: boot.h
 * Description: Boot and initialization interface for STM32F407
 * Phase: 1 - Boot & Board Bring-up
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_BOOT_H
#define DSRTOS_BOOT_H

#include <stdint.h>
#include <stdbool.h>

/* MISRA Rule 8.7: Objects shall be defined at block scope if only accessed from within a single function */

/* Boot stage definitions */
typedef enum {
    BOOT_STAGE_RESET = 0U,
    BOOT_STAGE_EARLY_INIT,
    BOOT_STAGE_CLOCK_INIT,
    BOOT_STAGE_MEMORY_INIT,
    BOOT_STAGE_PERIPHERAL_INIT,
    BOOT_STAGE_KERNEL_INIT,
    BOOT_STAGE_COMPLETE
} dsrtos_boot_stage_t;

/* Boot error codes */
typedef enum {
    BOOT_ERROR_NONE = 0U,
    BOOT_ERROR_CLOCK_INIT,
    BOOT_ERROR_PLL_LOCK,
    BOOT_ERROR_MEMORY_TEST,
    BOOT_ERROR_STACK_OVERFLOW,
    BOOT_ERROR_WATCHDOG,
    BOOT_ERROR_PERIPHERAL,
    BOOT_ERROR_CHECKSUM
} dsrtos_boot_error_t;

/* Boot configuration structure */
typedef struct {
    uint32_t system_clock_hz;
    uint32_t ahb_divider;
    uint32_t apb1_divider;
    uint32_t apb2_divider;
    bool enable_fpu;
    bool enable_cache;
    bool enable_mpu;
    bool enable_watchdog;
    uint32_t watchdog_timeout_ms;
} dsrtos_boot_config_t;

/* Boot status structure */
typedef struct {
    dsrtos_boot_stage_t current_stage;
    dsrtos_boot_error_t last_error;
    uint32_t boot_count;
    uint32_t reset_reason;
    uint32_t boot_time_ms;
    uint32_t checksum_status;
} dsrtos_boot_status_t;

/* Function prototypes - MISRA Rule 8.2: Function types shall be in prototype form */
void dsrtos_boot_init(void);
void dsrtos_early_init(void);
dsrtos_boot_error_t dsrtos_clock_init(void);
dsrtos_boot_error_t dsrtos_memory_init(void);
dsrtos_boot_error_t dsrtos_peripheral_init(void);
void dsrtos_boot_complete(void);

/* Safety functions */
bool dsrtos_boot_self_test(void);
void dsrtos_boot_record_error(dsrtos_boot_error_t error);
dsrtos_boot_status_t* dsrtos_boot_get_status(void);

/* Reset and fault handlers */
void dsrtos_system_reset(void);
uint32_t dsrtos_get_reset_reason(void);
void dsrtos_clear_reset_flags(void);

#endif /* DSRTOS_BOOT_H */
