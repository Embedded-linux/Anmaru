/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Project
 *
 * File: system_config.h
 * Description: System configuration and hardware definitions
 * Phase: 1 - Boot & Board Bring-up
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_SYSTEM_CONFIG_H
#define DSRTOS_SYSTEM_CONFIG_H

/* Crystal frequency */
#define HSE_VALUE    8000000U   /* 8 MHz external crystal */
#define HSI_VALUE    16000000U  /* 16 MHz internal oscillator */

/* System frequencies */
#define SYSCLK_FREQ  168000000U /* 168 MHz system clock */
#define HCLK_FREQ    168000000U /* 168 MHz AHB bus */
#define PCLK1_FREQ   42000000U  /* 42 MHz APB1 bus */
#define PCLK2_FREQ   84000000U  /* 84 MHz APB2 bus */

/* Memory regions */
#define SRAM_BASE    0x20000000U
#define CCM_BASE     0x10000000U

/* Memory sizes */
#define FLASH_SIZE   0x100000U  /* 1MB */
#define SRAM_SIZE    0x30000U   /* 192KB */
#define CCM_SIZE     0x10000U   /* 64KB */

/* Stack configuration */
#define MAIN_STACK_SIZE     0x2000U  /* 8KB */
#define PROCESS_STACK_SIZE  0x1000U  /* 4KB */

/* Interrupt priorities (0 = highest, 15 = lowest) */
#define SYSTICK_PRIORITY    1U
#define PENDSV_PRIORITY     15U
#define SVC_PRIORITY        0U

/* Watchdog configuration */
#define IWDG_TIMEOUT_MS     1000U
#define WWDG_WINDOW         0x7FU
#define WWDG_COUNTER        0x7FU

/* Debug configuration */
#define DEBUG_UART          USART2
#define DEBUG_BAUD_RATE     115200U

/* Safety features */
#define ENABLE_STACK_CANARY     1
#define ENABLE_MEMORY_PROTECTION 1
#define ENABLE_WATCHDOG         1
#define ENABLE_FAULT_HANDLERS   1
#define ENABLE_DIAGNOSTIC_LOG   1

/* Certification compliance */
#define MISRA_COMPLIANT        1
#define DO178C_LEVEL          "DAL-B"
#define IEC62304_CLASS        "B"
#define ISO26262_ASIL         "D"

#endif /* DSRTOS_SYSTEM_CONFIG_H */

/* APB clock frequencies */
#ifndef PCLK1_FREQ
#define PCLK1_FREQ   42000000U  /* 42 MHz APB1 bus */
#endif

#ifndef PCLK2_FREQ  
#define PCLK2_FREQ   84000000U  /* 84 MHz APB2 bus */
#endif

/* Debug UART configuration */
#ifndef DEBUG_BAUD_RATE
#define DEBUG_BAUD_RATE     115200U
#endif

#ifndef DEBUG_UART
#define DEBUG_UART          USART2
#endif
