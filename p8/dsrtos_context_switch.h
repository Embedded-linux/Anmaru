/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 8: ARM Cortex-M4 Context Switch Header
 * 
 * ARM Cortex-M4F specific context switching with FPU support
 * Optimized for < 200 CPU cycles context switch
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_CONTEXT_SWITCH_H
#define DSRTOS_CONTEXT_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"

/* Missing type definition */
typedef int32_t dsrtos_status_t;

/* ============================================================================
 * ARM Cortex-M4 Core Registers
 * ============================================================================ */

/* MISRA-C:2012 Rule 6.1: Bit-field types shall be explicitly signed or unsigned */
/* Hardware stack frame pushed by exception entry */
typedef struct {
    uint32_t r0;                            /* General purpose R0 */
    uint32_t r1;                            /* General purpose R1 */
    uint32_t r2;                            /* General purpose R2 */
    uint32_t r3;                            /* General purpose R3 */
    uint32_t r12;                           /* General purpose R12 */
    uint32_t lr;                            /* Link Register (R14) */
    uint32_t pc;                            /* Program Counter (R15) */
    uint32_t xpsr;                          /* Program Status Register */
} dsrtos_hw_stack_frame_t;

/* Extended stack frame with FPU context (when FPU is used) */
typedef struct {
    uint32_t s0;                            /* FP registers S0-S15 */
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t s12;
    uint32_t s13;
    uint32_t s14;
    uint32_t s15;
    uint32_t fpscr;                         /* FP Status Control Register */
    uint32_t reserved;                      /* Reserved for alignment */
} dsrtos_fpu_stack_frame_t;

/* Software saved context */
typedef struct {
    uint32_t r4;                            /* General purpose R4-R11 */
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t lr_exc;                        /* Exception return value */
} dsrtos_sw_stack_frame_t;

/* Extended FPU context (S16-S31) for lazy saving */
typedef struct {
    uint32_t s16;                           /* FP registers S16-S31 */
    uint32_t s17;
    uint32_t s18;
    uint32_t s19;
    uint32_t s20;
    uint32_t s21;
    uint32_t s22;
    uint32_t s23;
    uint32_t s24;
    uint32_t s25;
    uint32_t s26;
    uint32_t s27;
    uint32_t s28;
    uint32_t s29;
    uint32_t s30;
    uint32_t s31;
} dsrtos_fpu_extended_frame_t;

/* Complete task context */
typedef struct dsrtos_context {
    /* Stack pointer */
    uint32_t* sp;                           /* Current stack pointer */
    
    /* Stack boundaries for overflow detection */
    uint32_t* stack_base;                   /* Stack base address */
    uint32_t stack_size;                    /* Stack size in bytes */
    uint32_t* stack_limit;                  /* Stack limit for overflow check */
    
    /* FPU state */
    struct {
        bool active;                        /* FPU context is active */
        bool lazy_saved;                    /* FPU context lazy saved */
        uint32_t fpccr;                     /* FP Context Control Register */
        dsrtos_fpu_extended_frame_t* extended; /* Extended FPU registers */
    } fpu;
    
    /* MPU configuration */
    struct {
        bool enabled;                       /* MPU enabled for task */
        uint8_t num_regions;                /* Number of configured regions */
        struct {
            uint32_t rbar;                  /* Region Base Address Register */
            uint32_t rasr;                  /* Region Attribute and Size Register */
        } regions[8];
    } mpu;
    
    /* Exception state */
    struct {
        uint32_t primask;                   /* Priority mask */
        uint32_t basepri;                   /* Base priority */
        uint32_t faultmask;                 /* Fault mask */
        uint32_t control;                   /* Control register */
    } exc_state;
    
    /* Performance counters */
    struct {
        uint32_t switch_count;              /* Number of context switches */
        uint32_t switch_cycles;             /* Total cycles in switching */
        uint32_t fpu_saves;                 /* FPU context saves */
        uint32_t stack_usage;               /* Maximum stack usage */
    } stats;
} dsrtos_context_t;

/* ============================================================================
 * System Control Block (SCB) Registers
 * ============================================================================ */

/* SCB_BASE defined in core_cm4.h */
#define SCB_ICSR                (*(volatile uint32_t*)(SCB_BASE + 0x04))
#define SCB_VTOR                (*(volatile uint32_t*)(SCB_BASE + 0x08))
#define SCB_AIRCR               (*(volatile uint32_t*)(SCB_BASE + 0x0C))
#define SCB_SCR                 (*(volatile uint32_t*)(SCB_BASE + 0x10))
#define SCB_CCR                 (*(volatile uint32_t*)(SCB_BASE + 0x14))
#define SCB_SHPR1               (*(volatile uint32_t*)(SCB_BASE + 0x18))
#define SCB_SHPR2               (*(volatile uint32_t*)(SCB_BASE + 0x1C))
#define SCB_SHPR3               (*(volatile uint32_t*)(SCB_BASE + 0x20))
#define SCB_SHCSR               (*(volatile uint32_t*)(SCB_BASE + 0x24))
#define SCB_CFSR                (*(volatile uint32_t*)(SCB_BASE + 0x28))

/* SCB_ICSR bits */
#define SCB_ICSR_PENDSVSET      (1UL << 28)
#define SCB_ICSR_PENDSVCLR      (1UL << 27)

/* ============================================================================
 * FPU Registers
 * ============================================================================ */

/* FPU_BASE defined in core_cm4.h */
#define FPU_FPCCR               (*(volatile uint32_t*)(FPU_BASE + 0x04))
#define FPU_FPCAR               (*(volatile uint32_t*)(FPU_BASE + 0x08))
#define FPU_FPDSCR              (*(volatile uint32_t*)(FPU_BASE + 0x0C))

/* FPU_FPCCR bits */
#define FPU_FPCCR_ASPEN         (1UL << 31)    /* Automatic state preservation */
#define FPU_FPCCR_LSPEN         (1UL << 30)    /* Lazy state preservation */
#define FPU_FPCCR_LSPACT        (1UL << 0)     /* Lazy state active */

/* ============================================================================
 * MPU Registers
 * ============================================================================ */

/* MPU_BASE defined in core_cm4.h */
#define MPU_TYPE                (*(volatile uint32_t*)(MPU_BASE + 0x00))
#define MPU_CTRL                (*(volatile uint32_t*)(MPU_BASE + 0x04))
#define MPU_RNR                 (*(volatile uint32_t*)(MPU_BASE + 0x08))
#define MPU_RBAR                (*(volatile uint32_t*)(MPU_BASE + 0x0C))
#define MPU_RASR                (*(volatile uint32_t*)(MPU_BASE + 0x10))

/* MPU_CTRL bits */
#define MPU_CTRL_ENABLE         (1UL << 0)
#define MPU_CTRL_HFNMIENA       (1UL << 1)
#define MPU_CTRL_PRIVDEFENA     (1UL << 2)

/* ============================================================================
 * Exception Return Values
 * ============================================================================ */

#define EXC_RETURN_HANDLER_MSP  0xFFFFFFF1UL   /* Return to Handler mode, use MSP */
#define EXC_RETURN_THREAD_MSP   0xFFFFFFF9UL   /* Return to Thread mode, use MSP */
#define EXC_RETURN_THREAD_PSP   0xFFFFFFFDUL   /* Return to Thread mode, use PSP */

/* With FPU context */
#define EXC_RETURN_HANDLER_MSP_FPU  0xFFFFFFE1UL
#define EXC_RETURN_THREAD_MSP_FPU   0xFFFFFFE9UL
#define EXC_RETURN_THREAD_PSP_FPU   0xFFFFFFEDUL

/* ============================================================================
 * Configuration Constants
 * ============================================================================ */

#define DSRTOS_STACK_FILL_PATTERN      0xDEADBEEFUL
#define DSRTOS_STACK_GUARD_SIZE         32U
/* DSRTOS_MIN_STACK_SIZE defined in dsrtos_types.h */
#define DSRTOS_STACK_ALIGNMENT          8U
#define DSRTOS_MPU_GUARD_SIZE           32U

/* Context switch timing targets */
#define DSRTOS_TARGET_SWITCH_CYCLES     200U
#define DSRTOS_MAX_SWITCH_CYCLES        250U

/* Priority levels for exceptions */
#define DSRTOS_PENDSV_PRIORITY          0xFFU  /* Lowest priority */
#define DSRTOS_SYSTICK_PRIORITY         0xF0U
#define DSRTOS_SVCALL_PRIORITY          0x00U  /* Highest priority */

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/* Current and next task contexts */
extern dsrtos_context_t* volatile g_current_context;
extern dsrtos_context_t* volatile g_next_context;

/* FPU state */
extern volatile bool g_fpu_context_active;

/* Performance counters */
extern volatile uint32_t g_context_switch_count;
extern volatile uint32_t g_context_switch_cycles_total;
extern volatile uint32_t g_context_switch_cycles_max;
extern volatile uint32_t g_context_switch_cycles_min;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Initialization */
dsrtos_status_t dsrtos_context_switch_init(void);
dsrtos_status_t dsrtos_fpu_init(void);
dsrtos_status_t dsrtos_mpu_init(void);

/* Stack operations */
void* dsrtos_stack_init(void* stack_top, 
                       void (*task_entry)(void*),
                       void* param,
                       void (*exit_handler)(void));
dsrtos_status_t dsrtos_stack_check(dsrtos_context_t* context);
uint32_t dsrtos_stack_get_free(dsrtos_context_t* context);
void dsrtos_stack_fill_pattern(void* stack_base, uint32_t size);

/* Context switching - Core functions */
void dsrtos_context_switch(void);
void dsrtos_context_switch_request(void);
void dsrtos_context_save(dsrtos_context_t* context);
void dsrtos_context_restore(dsrtos_context_t* context);

/* FPU context management */
void dsrtos_fpu_context_save(dsrtos_context_t* context);
void dsrtos_fpu_context_restore(dsrtos_context_t* context);
void dsrtos_fpu_lazy_save_enable(void);
void dsrtos_fpu_lazy_save_disable(void);
bool dsrtos_fpu_is_context_active(void);

/* MPU management */
dsrtos_status_t dsrtos_mpu_configure_region(uint8_t region,
                                           uint32_t base_addr,
                                           uint32_t size,
                                           uint32_t attributes);
dsrtos_status_t dsrtos_mpu_switch_context(dsrtos_context_t* context);
dsrtos_status_t dsrtos_mpu_enable(void);
dsrtos_status_t dsrtos_mpu_disable(void);

/* Exception handlers (implemented in assembly) */
void PendSV_Handler(void) __attribute__((naked));
void SVC_Handler(void) __attribute__((naked));
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);

/* Performance monitoring */
uint32_t dsrtos_context_get_switch_cycles(void);
void dsrtos_context_reset_stats(void);
void dsrtos_context_dump_stats(void);

/* Hooks */
typedef void (*dsrtos_context_switch_hook_t)(dsrtos_context_t* from, 
                                            dsrtos_context_t* to);
void dsrtos_context_set_pre_switch_hook(dsrtos_context_switch_hook_t hook);
void dsrtos_context_set_post_switch_hook(dsrtos_context_switch_hook_t hook);

/* Inline functions for critical operations */

/* MISRA-C:2012 Dir 4.9: Function-like macros avoided where possible */
static inline void dsrtos_trigger_pendsv(void)
{
    SCB_ICSR = SCB_ICSR_PENDSVSET;
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

static inline uint32_t dsrtos_get_psp(void)
{
    uint32_t psp;
    __asm volatile ("mrs %0, psp" : "=r" (psp));
    return psp;
}

static inline void dsrtos_set_psp(uint32_t psp)
{
    __asm volatile ("msr psp, %0" : : "r" (psp));
}

static inline uint32_t dsrtos_get_msp(void)
{
    uint32_t msp;
    __asm volatile ("mrs %0, msp" : "=r" (msp));
    return msp;
}

static inline void dsrtos_set_msp(uint32_t msp)
{
    __asm volatile ("msr msp, %0" : : "r" (msp));
}

static inline uint32_t dsrtos_get_primask(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    return primask;
}

static inline void dsrtos_set_primask(uint32_t primask)
{
    __asm volatile ("msr primask, %0" : : "r" (primask));
}

static inline uint32_t dsrtos_get_control(void)
{
    uint32_t control;
    __asm volatile ("mrs %0, control" : "=r" (control));
    return control;
}

static inline void dsrtos_set_control(uint32_t control)
{
    __asm volatile ("msr control, %0" : : "r" (control));
    __asm volatile ("isb");
}

/* Critical section management */
static inline uint32_t dsrtos_enter_critical(void)
{
    uint32_t primask = dsrtos_get_primask();
    __asm volatile ("cpsid i" : : : "memory");
    return primask;
}

static inline void dsrtos_exit_critical(uint32_t primask)
{
    dsrtos_set_primask(primask);
}

#endif /* DSRTOS_CONTEXT_SWITCH_H */
