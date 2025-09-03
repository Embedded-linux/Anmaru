/*
 * @file dsrtos_config.h
 * @brief DSRTOS Project-Wide Configuration for Phases 1-3
 * @date 2024-12-30
 * 
 * Central configuration file for all DSRTOS phases.
 * Modify these values to customize DSRTOS for your application.
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_CONFIG_H
#define DSRTOS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * SYSTEM CONFIGURATION
 *============================================================================*/

/* Target Board Selection */
#define DSRTOS_BOARD_STM32F407         1
#define DSRTOS_BOARD_STM32F429         2
#define DSRTOS_BOARD_CUSTOM            99

#ifndef DSRTOS_BOARD
#define DSRTOS_BOARD                   DSRTOS_BOARD_STM32F407
#endif

/* CPU Configuration */
#define DSRTOS_CPU_FREQUENCY_HZ        168000000UL
#define DSRTOS_SYSTICK_FREQUENCY_HZ   1000UL
#define DSRTOS_USE_FPU                 1
#define DSRTOS_USE_MPU                 1
#define DSRTOS_USE_CACHE               0

/* Memory Configuration */
#define DSRTOS_FLASH_SIZE              (1024U * 1024U)    /* 1MB */
#define DSRTOS_SRAM_SIZE               (192U * 1024U)     /* 192KB */
#define DSRTOS_CCM_SIZE                (64U * 1024U)      /* 64KB */
#define DSRTOS_HEAP_SIZE               (32U * 1024U)      /* 32KB */

/*==============================================================================
 * PHASE 1: BOOT CONFIGURATION
 *============================================================================*/

/* Clock Configuration */
#define DSRTOS_HSE_VALUE               8000000UL          /* 8MHz crystal */
#define DSRTOS_PLL_M                   8U
#define DSRTOS_PLL_N                   336U
#define DSRTOS_PLL_P                   2U
#define DSRTOS_PLL_Q                   7U

/* Boot Options */
#define DSRTOS_ENABLE_WATCHDOG         1
#define DSRTOS_WATCHDOG_TIMEOUT_MS     5000U
#define DSRTOS_ENABLE_BROWNOUT_DETECT  1
#define DSRTOS_BOOT_TIMEOUT_MS         1000U

/* Debug Configuration */
#define DSRTOS_DEBUG_UART              USART1
#define DSRTOS_DEBUG_BAUD_RATE         115200U
#define DSRTOS_ENABLE_SEMIHOSTING      0
#define DSRTOS_ENABLE_ITM_TRACE        1

/*==============================================================================
 * PHASE 2: KERNEL CONFIGURATION
 *============================================================================*/

/* Kernel Features */
#define DSRTOS_USE_HOOKS               1
#define DSRTOS_USE_STATS               1
#define DSRTOS_USE_TRACE               1
#define DSRTOS_USE_ERROR_HANDLER       1

/* Critical Section Method */
#define DSRTOS_CRITICAL_METHOD_BASEPRI 1
#define DSRTOS_CRITICAL_METHOD_PRIMASK 2
#define DSRTOS_CRITICAL_METHOD         DSRTOS_CRITICAL_METHOD_BASEPRI

/* System Call Configuration */
#define DSRTOS_MAX_SYSCALL_INTERRUPT_PRIORITY  5
#define DSRTOS_USE_SYSCALL_VALIDATION          1

/* Hook Configuration */
#define DSRTOS_USE_IDLE_HOOK           1
#define DSRTOS_USE_TICK_HOOK           1
#define DSRTOS_USE_MALLOC_FAILED_HOOK  1
#define DSRTOS_USE_STACK_OVERFLOW_HOOK 1

/*==============================================================================
 * PHASE 3: TASK MANAGEMENT CONFIGURATION
 *============================================================================*/

/* Task Limits */
#define DSRTOS_MAX_TASKS               256U
#define DSRTOS_MAX_TASK_NAME_LEN       32U
#define DSRTOS_MAX_PRIORITY            255U
#define DSRTOS_IDLE_PRIORITY           255U

/* Stack Configuration */
#define DSRTOS_MIN_STACK_SIZE          256U
#define DSRTOS_DEFAULT_STACK_SIZE      1024U
#define DSRTOS_IDLE_STACK_SIZE         512U
#define DSRTOS_ISR_STACK_SIZE          2048U
#define DSRTOS_STACK_GROWTH_DIRECTION  (-1)  /* -1 = down, 1 = up */

/* Stack Protection */
#define DSRTOS_USE_STACK_CANARIES      1
#define DSRTOS_STACK_CANARY_VALUE      0xDEADBEEFUL
#define DSRTOS_STACK_FILL_PATTERN      0xA5A5A5A5UL
#define DSRTOS_STACK_GUARD_SIZE        32U

/* Task Features */
#define DSRTOS_USE_TASK_NOTIFICATIONS  1
#define DSRTOS_USE_TASK_DELETE         1
#define DSRTOS_USE_TASK_SUSPEND        1
#define DSRTOS_USE_TIME_SLICING        1
#define DSRTOS_DEFAULT_TIME_SLICE      10U

/* Task Pools (Static Allocation) */
#define DSRTOS_USE_STATIC_ALLOCATION   1
#define DSRTOS_TASK_POOL_SIZE          32U
#define DSRTOS_STACK_POOL_SIZE         16U

/* Statistics Collection */
#define DSRTOS_COLLECT_RUNTIME_STATS   1
#define DSRTOS_STATS_SAMPLE_PERIOD_MS  100U
#define DSRTOS_STATS_HISTORY_SIZE      60U
#define DSRTOS_CPU_LOAD_EWMA_ALPHA     0.2F

/*==============================================================================
 * MEMORY MANAGEMENT (Future Phases)
 *============================================================================*/

/* Heap Configuration */
#define DSRTOS_HEAP_ALGORITHM_TLSF     1
#define DSRTOS_HEAP_ALGORITHM_FIRSTFIT 2
#define DSRTOS_HEAP_ALGORITHM_BESTFIT  3
#define DSRTOS_HEAP_ALGORITHM          DSRTOS_HEAP_ALGORITHM_TLSF

/* Memory Protection */
#define DSRTOS_USE_MEMORY_PROTECTION   1
#define DSRTOS_MPU_REGIONS             8U

/*==============================================================================
 * SCHEDULER CONFIGURATION (Future Phases)
 *============================================================================*/

/* Scheduler Types */
#define DSRTOS_SCHEDULER_ROUND_ROBIN   1
#define DSRTOS_SCHEDULER_PRIORITY      2
#define DSRTOS_SCHEDULER_EDF           3
#define DSRTOS_SCHEDULER_CFS           4
#define DSRTOS_SCHEDULER_ADAPTIVE      5

/* Default Scheduler */
#define DSRTOS_DEFAULT_SCHEDULER       DSRTOS_SCHEDULER_PRIORITY

/* Scheduler Features */
#define DSRTOS_USE_PRIORITY_INHERITANCE 1
#define DSRTOS_USE_PRIORITY_CEILING    0
#define DSRTOS_USE_DEADLINE_MONOTONIC  0

/*==============================================================================
 * SAFETY AND CERTIFICATION
 *============================================================================*/

/* Safety Features */
#define DSRTOS_ENABLE_ASSERTIONS       1
#define DSRTOS_ENABLE_RUNTIME_CHECKS   1
#define DSRTOS_ENABLE_STACK_CHECKING   1
#define DSRTOS_ENABLE_MEMORY_CHECKING  1

/* Certification Standards */
#define DSRTOS_DO178C_LEVEL            2  /* DAL-B */
#define DSRTOS_IEC62304_CLASS          2  /* Class B */
#define DSRTOS_ISO26262_ASIL           4  /* ASIL D */
#define DSRTOS_IEC61508_SIL            3  /* SIL 3 */

/* MISRA Compliance */
#define DSRTOS_MISRA_C_2012            1
#define DSRTOS_MISRA_MANDATORY_ONLY    0
#define DSRTOS_MISRA_REQUIRED_RULES    1
#define DSRTOS_MISRA_ADVISORY_RULES    1

/*==============================================================================
 * DEBUG AND DEVELOPMENT
 *============================================================================*/

/* Debug Levels */
#define DSRTOS_DEBUG_NONE              0
#define DSRTOS_DEBUG_ERROR             1
#define DSRTOS_DEBUG_WARNING           2
#define DSRTOS_DEBUG_INFO              3
#define DSRTOS_DEBUG_VERBOSE           4

#ifndef DSRTOS_DEBUG_LEVEL
#define DSRTOS_DEBUG_LEVEL             DSRTOS_DEBUG_INFO
#endif

/* Debug Features */
#define DSRTOS_ENABLE_TRACE_BUFFER     1
#define DSRTOS_TRACE_BUFFER_SIZE       4096U
#define DSRTOS_ENABLE_PROFILING        1
#define DSRTOS_ENABLE_COVERAGE         0

/*==============================================================================
 * OPTIMIZATION SETTINGS
 *============================================================================*/

/* Code Optimization */
#define DSRTOS_OPTIMIZE_FOR_SPEED      0
#define DSRTOS_OPTIMIZE_FOR_SIZE       1
#define DSRTOS_USE_INLINE_FUNCTIONS    1
#define DSRTOS_USE_FAST_INTERRUPTS     1

/* Data Alignment */
#define DSRTOS_CACHE_LINE_SIZE         32U
#define DSRTOS_TCB_ALIGNMENT           32U
#define DSRTOS_STACK_ALIGNMENT         8U

/*==============================================================================
 * FEATURE GATES
 *============================================================================*/

/* Phase 1 Features */
#define DSRTOS_PHASE1_COMPLETE         1

/* Phase 2 Features */
#define DSRTOS_PHASE2_COMPLETE         1

/* Phase 3 Features */
#define DSRTOS_PHASE3_COMPLETE         1

/* Future Phase Features (disabled) */
#define DSRTOS_PHASE4_COMPLETE         0  /* Task-Scheduler Interface */
#define DSRTOS_PHASE5_COMPLETE         0  /* Pluggable Scheduler */
#define DSRTOS_PHASE6_COMPLETE         0  /* Schedulers */
#define DSRTOS_PHASE7_COMPLETE         0  /* Dynamic Switching */

/*==============================================================================
 * COMPILE-TIME CHECKS
 *============================================================================*/

/* Validate configuration */
#if DSRTOS_MAX_PRIORITY > 255
    #error "Maximum priority cannot exceed 255"
#endif

#if DSRTOS_MIN_STACK_SIZE < 256
    #error "Minimum stack size must be at least 256 bytes"
#endif

#if DSRTOS_MAX_TASKS > 256
    #error "Maximum tasks cannot exceed 256 in current implementation"
#endif

#if (DSRTOS_USE_FPU == 1) && !defined(__VFP_FP__)
    #error "FPU enabled but compiler not configured for hardware floating point"
#endif

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CONFIG_H */
