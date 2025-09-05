/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 8: ARM Cortex-M4 Context Switch Implementation
 * 
 * Implements context switching, FPU management, and MPU configuration
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * MISRA-C:2012 Compliant with documented deviations
 */

#include "dsrtos_context_switch.h"
#include "dsrtos_kernel.h"
#include "dsrtos_task_manager.h"
#include <string.h>

/* Missing constants */
#ifndef DSRTOS_OK
#define DSRTOS_OK                          (0)
#endif
#ifndef DSRTOS_ERROR
#define DSRTOS_ERROR                       (-1)
#endif

/* Missing function stubs */
static dsrtos_tcb_t* dsrtos_scheduler_select_next_task(void) {
    return NULL; /* TODO: Implement scheduler selection */
}

static dsrtos_tcb_t* dsrtos_scheduler_get_current_task(void) {
    return NULL; /* TODO: Implement current task getter */
}

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/* MISRA-C:2012 Rule 8.7: Could be static but needed by assembly */
dsrtos_context_t* volatile g_current_context = NULL;
dsrtos_context_t* volatile g_next_context = NULL;

/* FPU state tracking */
volatile bool g_fpu_context_active = false;

/* Performance counters */
volatile uint32_t g_context_switch_count = 0U;
volatile uint32_t g_context_switch_cycles_total = 0U;
volatile uint32_t g_context_switch_cycles_max = 0U;
volatile uint32_t g_context_switch_cycles_min = 0xFFFFFFFFU;

/* Context switch hooks */
static dsrtos_context_switch_hook_t g_pre_switch_hook = NULL;
static dsrtos_context_switch_hook_t g_post_switch_hook = NULL;

/* DWT for cycle counting */
#define DWT_CTRL    (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t*)0xE0001004)
#define DWT_LAR     (*(volatile uint32_t*)0xE0001FB0)

/* ============================================================================
 * Forward Declarations (Assembly functions)
 * ============================================================================ */

extern void dsrtos_context_switch_asm(void);
extern uint32_t dsrtos_get_cpu_cycles(void);
extern void dsrtos_enable_cycle_counter(void);
extern void* dsrtos_stack_init_asm(void* stack_top, 
                                  void (*task_entry)(void*),
                                  void* param,
                                  void (*exit_handler)(void));

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

/**
 * @brief Initialize context switching subsystem
 * 
 * MISRA-C:2012 Compliance:
 * - Rule 11.4: Casting between pointer and integer (hardware addresses)
 * - Rule 11.6: Cast from integer to pointer (register access)
 */
dsrtos_status_t dsrtos_context_switch_init(void)
{
    uint32_t reg_value;
    
    /* Enable DWT cycle counter for performance monitoring */
    dsrtos_enable_cycle_counter();
    
    /* Configure PendSV priority to lowest */
    /* MISRA-C:2012 Dev 11.4: Hardware register access requires casting */
    reg_value = SCB_SHPR3;
    reg_value &= ~(0xFFU << 16);
    reg_value |= (DSRTOS_PENDSV_PRIORITY << 16);
    SCB_SHPR3 = reg_value;
    
    /* Configure SysTick priority */
    reg_value = SCB_SHPR3;
    reg_value &= ~(0xFFU << 24);
    reg_value |= (DSRTOS_SYSTICK_PRIORITY << 24);
    SCB_SHPR3 = reg_value;
    
    /* Initialize FPU if present */
    if (dsrtos_fpu_init() != DSRTOS_OK) {
        return DSRTOS_ERROR;
    }
    
    /* Initialize MPU if present */
    if (dsrtos_mpu_init() != DSRTOS_OK) {
        return DSRTOS_ERROR;
    }
    
    /* Reset performance counters */
    dsrtos_context_reset_stats();
    
    return DSRTOS_OK;
}

/**
 * @brief Initialize FPU for lazy stacking
 */
dsrtos_status_t dsrtos_fpu_init(void)
{
    /* Enable FPU access */
    /* MISRA-C:2012 Dev 11.4: Hardware register access */
    uint32_t* cpacr = (uint32_t*)0xE000ED88;
    *cpacr |= (0xFU << 20);  /* CP10 and CP11 Full Access */
    
    /* Memory barrier */
    __asm volatile ("dsb");
    __asm volatile ("isb");
    
    /* Enable automatic lazy stacking */
    FPU_FPCCR |= FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN;
    
    return DSRTOS_OK;
}

/**
 * @brief Initialize MPU with default configuration
 */
dsrtos_status_t dsrtos_mpu_init(void)
{
    /* Check if MPU is present */
    if ((MPU_TYPE & 0xFFU) == 0U) {
        /* No MPU regions available */
        return DSRTOS_ERROR;
    }
    
    /* Disable MPU during configuration */
    MPU_CTRL = 0U;
    
    /* Configure default background region */
    /* Allow privileged access, deny unprivileged access */
    MPU_CTRL = MPU_CTRL_PRIVDEFENA;
    
    return DSRTOS_OK;
}

/* ============================================================================
 * Stack Operations
 * ============================================================================ */

/**
 * @brief Initialize task stack with initial context
 * 
 * @param stack_top Pointer to top of stack (highest address)
 * @param task_entry Task entry point function
 * @param param Parameter to pass to task
 * @param exit_handler Function to call on task exit
 * @return Updated stack pointer
 */
void* dsrtos_stack_init(void* stack_top, 
                       void (*task_entry)(void*),
                       void* param,
                       void (*exit_handler)(void))
{
    uint32_t* sp = (uint32_t*)stack_top;
    
    /* MISRA-C:2012 Rule 11.3: Casting between pointer types for stack */
    /* Align stack to 8-byte boundary */
    sp = (uint32_t*)((uint32_t)sp & ~7U);
    
    /* Build exception stack frame (hardware saved registers) */
    *(--sp) = 0x01000000U;                 /* xPSR - Thumb mode */
    *(--sp) = (uint32_t)task_entry;        /* PC - Entry point */
    *(--sp) = (uint32_t)exit_handler;      /* LR - Exit handler */
    *(--sp) = 0x12121212U;                  /* R12 */
    *(--sp) = 0x03030303U;                  /* R3 */
    *(--sp) = 0x02020202U;                  /* R2 */
    *(--sp) = 0x01010101U;                  /* R1 */
    *(--sp) = (uint32_t)param;             /* R0 - Task parameter */
    
    /* Build software saved context */
    *(--sp) = EXC_RETURN_THREAD_PSP;       /* LR - Exception return */
    *(--sp) = 0x11111111U;                  /* R11 */
    *(--sp) = 0x10101010U;                  /* R10 */
    *(--sp) = 0x09090909U;                  /* R9 */
    *(--sp) = 0x08080808U;                  /* R8 */
    *(--sp) = 0x07070707U;                  /* R7 */
    *(--sp) = 0x06060606U;                  /* R6 */
    *(--sp) = 0x05050505U;                  /* R5 */
    *(--sp) = 0x04040404U;                  /* R4 */
    
    return sp;
}

/**
 * @brief Check for stack overflow
 * 
 * @param context Task context to check
 * @return DSRTOS_OK if stack is safe, DSRTOS_ERROR if overflow detected
 */
dsrtos_status_t dsrtos_stack_check(dsrtos_context_t* context)
{
    uint32_t* current_sp;
    uint32_t* stack_limit;
    uint32_t i;
    
    if (context == NULL) {
        return DSRTOS_ERROR;
    }
    
    current_sp = (uint32_t*)context->sp;
    stack_limit = context->stack_limit;
    
    /* Check if stack pointer is within bounds */
    if (current_sp < stack_limit) {
        /* Stack overflow detected */
        return DSRTOS_ERROR;
    }
    
    /* Check guard pattern at stack limit */
    for (i = 0U; i < (DSRTOS_STACK_GUARD_SIZE / 4U); i++) {
        if (stack_limit[i] != DSRTOS_STACK_FILL_PATTERN) {
            /* Guard area corrupted */
            return DSRTOS_ERROR;
        }
    }
    
    /* Update maximum stack usage */
    uint32_t usage = (uint32_t)context->stack_base - (uint32_t)current_sp;
    if (usage > context->stats.stack_usage) {
        context->stats.stack_usage = usage;
    }
    
    return DSRTOS_OK;
}

/**
 * @brief Get free stack space
 * 
 * @param context Task context
 * @return Number of free bytes on stack
 */
uint32_t dsrtos_stack_get_free(dsrtos_context_t* context)
{
    uint32_t* ptr;
    uint32_t count = 0U;
    
    if (context == NULL) {
        return 0U;
    }
    
    /* Count from stack limit up until we find non-pattern */
    ptr = context->stack_limit;
    while ((ptr < (uint32_t*)context->sp) && 
           (*ptr == DSRTOS_STACK_FILL_PATTERN)) {
        ptr++;
        count += 4U;
    }
    
    return count;
}

/**
 * @brief Fill stack with pattern for overflow detection
 * 
 * @param stack_base Base address of stack
 * @param size Size of stack in bytes
 */
void dsrtos_stack_fill_pattern(void* stack_base, uint32_t size)
{
    uint32_t* ptr = (uint32_t*)stack_base;
    uint32_t words = size / 4U;
    uint32_t i;
    
    for (i = 0U; i < words; i++) {
        ptr[i] = DSRTOS_STACK_FILL_PATTERN;
    }
}

/* ============================================================================
 * Context Switching Core Functions
 * ============================================================================ */

/**
 * @brief Request context switch
 * Sets PendSV pending bit to trigger context switch
 */
void dsrtos_context_switch_request(void)
{
    /* Set PendSV pending bit */
    dsrtos_trigger_pendsv();
}

/**
 * @brief Perform context switch (called from scheduler)
 * 
 * This function prepares the context switch and triggers PendSV
 */
void dsrtos_context_switch(void)
{
    dsrtos_tcb_t* current_task;
    dsrtos_tcb_t* next_task;
    uint32_t critical_state;
    
    /* Enter critical section */
    critical_state = dsrtos_enter_critical();
    
    /* Get current and next tasks from scheduler */
    current_task = dsrtos_scheduler_get_current_task();
    next_task = dsrtos_scheduler_select_next_task();
    
    if ((current_task != next_task) && (next_task != NULL)) {
        /* Set up context pointers for PendSV handler */
        if (current_task != NULL) {
            g_current_context = (dsrtos_context_t*)&current_task->cpu_context;
        }
        g_next_context = (dsrtos_context_t*)&next_task->cpu_context;
        
        /* Call pre-switch hook if registered */
        if (g_pre_switch_hook != NULL) {
            g_pre_switch_hook(g_current_context, g_next_context);
        }
        
        /* Trigger PendSV to perform actual context switch */
        dsrtos_context_switch_request();
    }
    
    /* Exit critical section */
    dsrtos_exit_critical(critical_state);
}

/* ============================================================================
 * FPU Context Management
 * ============================================================================ */

/**
 * @brief Save FPU context (called from assembly if needed)
 * 
 * @param context Context to save FPU state to
 */
void dsrtos_fpu_context_save(dsrtos_context_t* context)
{
    if (context == NULL) {
        return;
    }
    
    /* Check if FPU was used */
    if (FPU_FPCCR & FPU_FPCCR_LSPACT) {
        context->fpu.active = true;
        context->fpu.fpccr = FPU_FPCCR;
        
        /* S0-S15 and FPSCR are automatically saved by hardware */
        /* S16-S31 are saved by PendSV handler if needed */
        
        context->stats.fpu_saves++;
    } else {
        context->fpu.active = false;
    }
}

/**
 * @brief Restore FPU context
 * 
 * @param context Context to restore FPU state from
 */
void dsrtos_fpu_context_restore(dsrtos_context_t* context)
{
    if ((context == NULL) || (!context->fpu.active)) {
        return;
    }
    
    /* FPU context will be restored lazily on first use */
    /* Hardware handles S0-S15 and FPSCR */
    /* PendSV handler handles S16-S31 */
    
    g_fpu_context_active = true;
}

/**
 * @brief Enable FPU lazy stacking
 */
void dsrtos_fpu_lazy_save_enable(void)
{
    FPU_FPCCR |= FPU_FPCCR_LSPEN | FPU_FPCCR_ASPEN;
}

/**
 * @brief Disable FPU lazy stacking
 */
void dsrtos_fpu_lazy_save_disable(void)
{
    FPU_FPCCR &= ~(FPU_FPCCR_LSPEN | FPU_FPCCR_ASPEN);
}

/**
 * @brief Check if FPU context is active
 * 
 * @return true if FPU context is active
 */
bool dsrtos_fpu_is_context_active(void)
{
    return (FPU_FPCCR & FPU_FPCCR_LSPACT) != 0U;
}

/* ============================================================================
 * MPU Management
 * ============================================================================ */

/**
 * @brief Configure MPU region
 * 
 * @param region Region number (0-7)
 * @param base_addr Base address (must be aligned to size)
 * @param size Region size (must be power of 2, minimum 32 bytes)
 * @param attributes Region attributes
 * @return DSRTOS_OK on success
 */
dsrtos_status_t dsrtos_mpu_configure_region(uint8_t region,
                                           uint32_t base_addr,
                                           uint32_t size,
                                           uint32_t attributes)
{
    uint32_t region_size;
    uint32_t size_bits = 0U;
    
    /* Validate region number */
    if (region >= 8U) {
        return DSRTOS_ERROR;
    }
    
    /* Calculate size encoding (size must be power of 2) */
    region_size = 32U;
    size_bits = 4U;  /* Minimum size is 32 bytes (2^5) */
    
    while ((region_size < size) && (size_bits < 31U)) {
        region_size <<= 1;
        size_bits++;
    }
    
    /* Check alignment */
    if ((base_addr & (region_size - 1U)) != 0U) {
        return DSRTOS_ERROR;
    }
    
    /* Select region */
    MPU_RNR = region;
    
    /* Configure region base address */
    MPU_RBAR = base_addr | (1U << 4) | region;  /* Valid bit and region number */
    
    /* Configure region attributes and size */
    MPU_RASR = attributes | (size_bits << 1) | 1U;  /* Enable bit */
    
    /* Memory barrier */
    __asm volatile ("dsb");
    __asm volatile ("isb");
    
    return DSRTOS_OK;
}

/**
 * @brief Switch MPU context for task
 * 
 * @param context Task context with MPU configuration
 * @return DSRTOS_OK on success
 */
dsrtos_status_t dsrtos_mpu_switch_context(dsrtos_context_t* context)
{
    uint8_t i;
    
    if ((context == NULL) || (!context->mpu.enabled)) {
        /* Disable MPU if not used by task */
        MPU_CTRL = 0U;
        return DSRTOS_OK;
    }
    
    /* Disable MPU during reconfiguration */
    MPU_CTRL = 0U;
    
    /* Configure task's MPU regions */
    for (i = 0U; i < context->mpu.num_regions; i++) {
        MPU_RNR = i;
        MPU_RBAR = context->mpu.regions[i].rbar;
        MPU_RASR = context->mpu.regions[i].rasr;
    }
    
    /* Enable MPU with default privileged access */
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
    
    /* Memory barrier */
    __asm volatile ("dsb");
    __asm volatile ("isb");
    
    return DSRTOS_OK;
}

/**
 * @brief Enable MPU
 * 
 * @return DSRTOS_OK on success
 */
dsrtos_status_t dsrtos_mpu_enable(void)
{
    MPU_CTRL |= MPU_CTRL_ENABLE;
    
    /* Memory barrier */
    __asm volatile ("dsb");
    __asm volatile ("isb");
    
    return DSRTOS_OK;
}

/**
 * @brief Disable MPU
 * 
 * @return DSRTOS_OK on success
 */
dsrtos_status_t dsrtos_mpu_disable(void)
{
    MPU_CTRL &= ~MPU_CTRL_ENABLE;
    
    /* Memory barrier */
    __asm volatile ("dsb");
    __asm volatile ("isb");
    
    return DSRTOS_OK;
}

/* ============================================================================
 * Exception Handlers (C portions)
 * ============================================================================ */

/**
 * @brief Hard fault handler (C portion)
 * Called from assembly handler with stack pointer
 * 
 * @param stack Stack pointer at time of fault
 */
void dsrtos_hardfault_handler_c(uint32_t* stack)
{
    /* Extract fault information from stack */
    uint32_t r0 = stack[0];
    uint32_t r1 = stack[1];
    uint32_t r2 = stack[2];
    uint32_t r3 = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr = stack[5];
    uint32_t pc = stack[6];
    uint32_t psr = stack[7];
    
    /* Read fault status registers */
    uint32_t cfsr = SCB_CFSR;
    uint32_t hfsr = SCB_SHCSR;
    uint32_t mmfar = *(volatile uint32_t*)0xE000ED34;
    uint32_t bfar = *(volatile uint32_t*)0xE000ED38;
    
    /* Log fault information */
    /* This would typically be sent to a fault handler or logger */
    (void)r0; (void)r1; (void)r2; (void)r3;
    (void)r12; (void)lr; (void)pc; (void)psr;
    (void)cfsr; (void)hfsr; (void)mmfar; (void)bfar;
    
    /* System reset or halt for debugging */
    while (1) {
        /* Infinite loop - system halted */
    }
}

/**
 * @brief SVC handler (C portion) for system calls
 * 
 * @param svc_number System call number
 * @param params Parameters from registers
 */
void dsrtos_svc_handler_c(uint8_t svc_number, uint32_t* params)
{
    switch (svc_number) {
        case 0:
            /* Start scheduler - handled in assembly */
            break;
            
        case 1:
            /* Yield */
            dsrtos_context_switch_request();
            break;
            
        case 2:
            /* Sleep */
            /* Implementation depends on scheduler */
            break;
            
        default:
            /* Unknown SVC */
            break;
    }
    
    (void)params;  /* Avoid unused parameter warning */
}

/* ============================================================================
 * Performance Monitoring
 * ============================================================================ */

/**
 * @brief Get average context switch time in CPU cycles
 * 
 * @return Average cycles per context switch
 */
uint32_t dsrtos_context_get_switch_cycles(void)
{
    if (g_context_switch_count == 0U) {
        return 0U;
    }
    
    return g_context_switch_cycles_total / g_context_switch_count;
}

/**
 * @brief Reset context switch statistics
 */
void dsrtos_context_reset_stats(void)
{
    uint32_t critical_state = dsrtos_enter_critical();
    
    g_context_switch_count = 0U;
    g_context_switch_cycles_total = 0U;
    g_context_switch_cycles_max = 0U;
    g_context_switch_cycles_min = 0xFFFFFFFFU;
    
    dsrtos_exit_critical(critical_state);
}

/**
 * @brief Dump context switch statistics
 */
void dsrtos_context_dump_stats(void)
{
    uint32_t avg_cycles;
    
    if (g_context_switch_count == 0U) {
        return;
    }
    
    avg_cycles = g_context_switch_cycles_total / g_context_switch_count;
    
    /* Statistics would typically be logged or displayed */
    /* For now, just calculate them */
    (void)avg_cycles;
}

/* ============================================================================
 * Hook Functions
 * ============================================================================ */

/**
 * @brief Set pre-context switch hook
 * 
 * @param hook Function to call before context switch
 */
void dsrtos_context_set_pre_switch_hook(dsrtos_context_switch_hook_t hook)
{
    g_pre_switch_hook = hook;
}

/**
 * @brief Set post-context switch hook
 * 
 * @param hook Function to call after context switch
 */
void dsrtos_context_set_post_switch_hook(dsrtos_context_switch_hook_t hook)
{
    g_post_switch_hook = hook;
}
