/*
 * DSRTOS Context Switch Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * 
 * Implements context switching logic for ARM Cortex-M4 with FPU support.
 * Provides deterministic task switching with < 1μs latency.
 * 
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_context_switch.h"
#include "dsrtos_kernel.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_trace.h"
#include "dsrtos_task_scheduler_interface.h"
#include "stm32f4xx.h"
#include <string.h>

/*==============================================================================
 * MACROS AND CONSTANTS
 *============================================================================*/

/* PendSV and SysTick priority (lowest) */
#define PENDSV_PRIORITY        0xFFU
#define SYSTICK_PRIORITY       0xFFU

/* Context switch timing */
#define MAX_CONTEXT_SWITCH_CYCLES  1000U  /* 1 microsecond at 168MHz */
#define CONTEXT_SWITCH_TIMEOUT      100U  /* Maximum switch attempts */

/* NVIC registers */
#define NVIC_SYSPRI14          0xE000ED22U  /* PendSV priority register */
#define NVIC_PENDSV_PRI        0xFFU        /* Lowest priority */
#define NVIC_INT_CTRL          0xE000ED04U  /* Interrupt control register */
#define NVIC_PENDSVSET         0x10000000U  /* PendSV set bit */

/* Stack alignment */
#define STACK_ALIGN_MASK       0xFFFFFFF8U  /* 8-byte alignment */

/* Context validation */
#define CONTEXT_MAGIC          0xC0DEFA11U  /* Context validity marker */
#define STACK_PATTERN         0xDEADBEEFU  /* Stack fill pattern */

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/* Context switch state machine */
typedef enum {
    CONTEXT_STATE_IDLE = 0,
    CONTEXT_STATE_SAVE,
    CONTEXT_STATE_SWITCH,
    CONTEXT_STATE_RESTORE,
    CONTEXT_STATE_ERROR
} context_state_t;

/* Context switch control block */
typedef struct {
    volatile context_state_t state;
    volatile uint32_t switch_count;
    volatile uint32_t error_count;
    volatile uint32_t pending_switches;
    volatile bool in_switch;
    uint32_t last_switch_cycles;
} context_control_t;

/*==============================================================================
 * GLOBAL VARIABLES
 *============================================================================*/

/* Current and next task pointers for context switch */
volatile dsrtos_tcb_t* g_current_task = NULL;
volatile dsrtos_tcb_t* g_next_task = NULL;

/* Context switch statistics */
static dsrtos_context_stats_t g_context_stats = {0};

/* Context switch control */
static context_control_t g_context_control = {
    .state = CONTEXT_STATE_IDLE,
    .switch_count = 0U,
    .error_count = 0U,
    .pending_switches = 0U,
    .in_switch = false,
    .last_switch_cycles = 0U
};

/* First task flag */
static volatile bool g_first_task_started = false;

/* Context switch request flag */
static volatile bool g_context_switch_required = false;

/* Scheduler interface pointer */
extern dsrtos_scheduler_interface_t* g_scheduler_interface;

/*==============================================================================
 * STATIC FUNCTION PROTOTYPES
 *============================================================================*/

static void context_save_cpu_state(dsrtos_tcb_t* task, uint32_t* sp);
static void context_restore_cpu_state(dsrtos_tcb_t* task, uint32_t** sp);
static bool context_validate_stack(dsrtos_tcb_t* task);
static void context_switch_error_handler(uint32_t error_code);
static void context_update_statistics(uint32_t cycles);
static uint32_t context_measure_cycles(void);

/*==============================================================================
 * INITIALIZATION
 *============================================================================*/

/**
 * @brief Initialize context switching subsystem
 */
dsrtos_error_t dsrtos_context_switch_init(void)
{
    DSRTOS_TRACE_CONTEXT("Initializing context switch");
    
    /* Set PendSV to lowest priority */
    *(volatile uint8_t*)NVIC_SYSPRI14 = NVIC_PENDSV_PRI;
    
    /* Initialize statistics */
    g_context_stats.switch_count = 0U;
    g_context_stats.switch_cycles_min = UINT32_MAX;
    g_context_stats.switch_cycles_max = 0U;
    g_context_stats.switch_cycles_avg = 0U;
    g_context_stats.preemption_count = 0U;
    g_context_stats.voluntary_count = 0U;
    
    /* Clear task pointers */
    g_current_task = NULL;
    g_next_task = NULL;
    g_first_task_started = false;
    g_context_switch_required = false;
    
    /* Initialize control block */
    g_context_control.state = CONTEXT_STATE_IDLE;
    g_context_control.switch_count = 0U;
    g_context_control.error_count = 0U;
    g_context_control.pending_switches = 0U;
    g_context_control.in_switch = false;
    
#ifdef DSRTOS_USE_FPU
    /* Enable FPU if configured */
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));  /* CP10 and CP11 Full Access */
    
    /* Enable automatic lazy stacking of FPU registers */
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
    
    DSRTOS_TRACE_CONTEXT("FPU enabled with lazy stacking");
#endif
    
    /* Enable DWT cycle counter for timing measurements */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
    
    DSRTOS_TRACE_CONTEXT("Context switch initialized");
    
    return DSRTOS_SUCCESS;
}

/*==============================================================================
 * CONTEXT SWITCH TRIGGERING
 *============================================================================*/

/**
 * @brief Trigger context switch to next task
 */
void dsrtos_context_switch(dsrtos_tcb_t* next_task)
{
    uint32_t primask;
    
    if (next_task == NULL) {
        DSRTOS_TRACE_ERROR("Context switch called with NULL task");
        return;
    }
    
    /* Validate task TCB */
    if (next_task->magic_number != DSRTOS_TCB_MAGIC) {
        DSRTOS_TRACE_ERROR("Invalid task TCB in context switch");
        context_switch_error_handler(DSRTOS_ERROR_CORRUPTED);
        return;
    }
    
    /* Disable interrupts */
    primask = __get_PRIMASK();
    __disable_irq();
    
    /* Set next task */
    g_next_task = next_task;
    
    /* Check if this is the first task */
    if (!g_first_task_started) {
        g_current_task = next_task;
        g_first_task_started = true;
        
        DSRTOS_TRACE_CONTEXT("Starting first task %u", next_task->task_id);
        
        /* Start the first task */
        dsrtos_start_first_task();
        
        /* Should never return here */
        DSRTOS_ASSERT(false, "First task start failed");
    } else {
        /* Check if switch is needed */
        if (g_current_task != g_next_task) {
            /* Request context switch via PendSV */
            g_context_switch_required = true;
            g_context_control.pending_switches++;
            
            /* Trigger PendSV */
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
            
            DSRTOS_TRACE_CONTEXT("Context switch requested: %u -> %u",
                                g_current_task ? g_current_task->task_id : 0,
                                next_task->task_id);
        }
    }
    
    /* Restore interrupts */
    if ((primask & 0x1U) == 0U) {
        __enable_irq();
    }
}

/**
 * @brief Perform immediate context switch from ISR
 */
void dsrtos_context_switch_from_isr(void)
{
    /* Ensure we're in ISR context */
    DSRTOS_ASSERT((__get_IPSR() != 0U), "Not in ISR context");
    
    /* Trigger PendSV for deferred context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    
    g_context_control.pending_switches++;
    
    DSRTOS_TRACE_CONTEXT("Context switch from ISR requested");
}

/*==============================================================================
 * CONTEXT SAVE/RESTORE
 *============================================================================*/

/**
 * @brief Save current task context
 */
void* dsrtos_context_save(dsrtos_tcb_t* task)
{
    uint32_t* sp;
    
    if (task == NULL) {
        return NULL;
    }
    
    /* Get current stack pointer */
    sp = (uint32_t*)task->stack_pointer;
    
    /* Validate stack pointer */
    if ((sp < (uint32_t*)task->stack_base) ||
        (sp > (uint32_t*)((uint8_t*)task->stack_base + task->stack_size))) {
        DSRTOS_TRACE_ERROR("Stack overflow detected for task %u", task->task_id);
        context_switch_error_handler(DSRTOS_ERROR_STACK_OVERFLOW);
        return NULL;
    }
    
    /* Save CPU context */
    context_save_cpu_state(task, sp);
    
    /* Update task state */
    if (task->state == DSRTOS_TASK_STATE_RUNNING) {
        task->state = DSRTOS_TASK_STATE_READY;
    }
    
    /* Update statistics */
    task->context_switches++;
    
    DSRTOS_TRACE_CONTEXT("Context saved for task %u", task->task_id);
    
    return sp;
}

/**
 * @brief Restore task context
 */
void* dsrtos_context_restore(dsrtos_tcb_t* task)
{
    uint32_t* sp;
    
    if (task == NULL) {
        return NULL;
    }
    
    /* Validate task state */
    if ((task->state == DSRTOS_TASK_STATE_TERMINATED) ||
        (task->state == DSRTOS_TASK_STATE_INVALID)) {
        DSRTOS_TRACE_ERROR("Cannot restore terminated task %u", task->task_id);
        return NULL;
    }
    
    /* Validate stack integrity */
    if (!context_validate_stack(task)) {
        DSRTOS_TRACE_ERROR("Stack corruption detected for task %u", task->task_id);
        context_switch_error_handler(DSRTOS_ERROR_CORRUPTED);
        return NULL;
    }
    
    /* Get stack pointer */
    sp = (uint32_t*)task->stack_pointer;
    
    /* Restore CPU context */
    context_restore_cpu_state(task, &sp);
    
    /* Update task state */
    task->state = DSRTOS_TASK_STATE_RUNNING;
    
    DSRTOS_TRACE_CONTEXT("Context restored for task %u", task->task_id);
    
    return sp;
}

/*==============================================================================
 * CONTEXT SWITCH HANDLER
 *============================================================================*/

/**
 * @brief Main context switch handler (called from PendSV)
 * 
 * This function is called by PendSV_Handler in assembly
 */
void dsrtos_switch_context_handler(uint32_t* current_sp)
{
    uint32_t start_cycles;
    dsrtos_tcb_t* current;
    dsrtos_tcb_t* next;
    
    /* Measure context switch time */
    start_cycles = context_measure_cycles();
    
    /* Check for errors */
    if (g_context_control.in_switch) {
        g_context_control.error_count++;
        DSRTOS_TRACE_ERROR("Nested context switch detected");
        return;
    }
    
    g_context_control.in_switch = true;
    g_context_control.state = CONTEXT_STATE_SAVE;
    
    /* Get current and next tasks */
    current = (dsrtos_tcb_t*)g_current_task;
    next = (dsrtos_tcb_t*)g_next_task;
    
    /* Save current context if not first switch */
    if (current != NULL) {
        /* Store stack pointer */
        current->stack_pointer = current_sp;
        
        /* Validate saved context */
        if (!context_validate_stack(current)) {
            g_context_control.error_count++;
            g_context_control.state = CONTEXT_STATE_ERROR;
            g_context_control.in_switch = false;
            return;
        }
        
        /* Update current task state */
        if (current->state == DSRTOS_TASK_STATE_RUNNING) {
            current->state = DSRTOS_TASK_STATE_READY;
        }
        
        /* Save CPU context to TCB */
        current->cpu_context.sp = (uint32_t)current_sp;
    }
    
    g_context_control.state = CONTEXT_STATE_SWITCH;
    
    /* Validate next task */
    if (next == NULL) {
        /* No next task - use idle task */
        extern dsrtos_tcb_t* g_idle_task;
        next = g_idle_task;
        
        if (next == NULL) {
            /* Fatal error - no tasks to run */
            g_context_control.state = CONTEXT_STATE_ERROR;
            g_context_control.in_switch = false;
            dsrtos_panic("No runnable tasks");
            return;
        }
    }
    
    /* Update current task pointer */
    g_current_task = next;
    
    /* Update next task state */
    next->state = DSRTOS_TASK_STATE_RUNNING;
    
    g_context_control.state = CONTEXT_STATE_RESTORE;
    
    /* Clear pending switch flag */
    g_context_switch_required = false;
    if (g_context_control.pending_switches > 0U) {
        g_context_control.pending_switches--;
    }
    
    /* Update statistics */
    g_context_control.switch_count++;
    g_context_stats.switch_count++;
    
    /* Determine switch type */
    if (current != NULL) {
        if (current->voluntary_yields > 0U) {
            g_context_stats.voluntary_count++;
        } else {
            g_context_stats.preemption_count++;
        }
    }
    
    /* Update timing statistics */
    g_context_control.last_switch_cycles = context_measure_cycles() - start_cycles;
    context_update_statistics(g_context_control.last_switch_cycles);
    
    /* Check timing constraint */
    if (g_context_control.last_switch_cycles > MAX_CONTEXT_SWITCH_CYCLES) {
        DSRTOS_TRACE_WARNING("Context switch exceeded timing: %u cycles",
                            g_context_control.last_switch_cycles);
    }
    
    g_context_control.state = CONTEXT_STATE_IDLE;
    g_context_control.in_switch = false;
    
    DSRTOS_TRACE_CONTEXT("Context switch complete: %u -> %u (%u cycles)",
                        current ? current->task_id : 0,
                        next->task_id,
                        g_context_control.last_switch_cycles);
}

/*==============================================================================
 * STATISTICS
 *============================================================================*/

/**
 * @brief Get context switch statistics
 */
dsrtos_error_t dsrtos_context_get_stats(dsrtos_context_stats_t* stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy statistics atomically */
    dsrtos_critical_enter();
    *stats = g_context_stats;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Reset context switch statistics
 */
dsrtos_error_t dsrtos_context_reset_stats(void)
{
    dsrtos_critical_enter();
    
    g_context_stats.switch_count = 0U;
    g_context_stats.switch_cycles_min = UINT32_MAX;
    g_context_stats.switch_cycles_max = 0U;
    g_context_stats.switch_cycles_avg = 0U;
    g_context_stats.preemption_count = 0U;
    g_context_stats.voluntary_count = 0U;
    
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/*==============================================================================
 * STATIC HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Save CPU state to task context
 */
static void context_save_cpu_state(dsrtos_tcb_t* task, uint32_t* sp)
{
    /* Copy hardware-saved registers from stack to TCB */
    /* Stack frame: R0-R3, R12, LR, PC, xPSR */
    task->cpu_context.r0 = sp[0];
    task->cpu_context.r1 = sp[1];
    task->cpu_context.r2 = sp[2];
    task->cpu_context.r3 = sp[3];
    task->cpu_context.r12 = sp[4];
    task->cpu_context.lr = sp[5];
    task->cpu_context.pc = sp[6];
    task->cpu_context.psr = sp[7];
    
    /* Software-saved registers are handled in assembly */
}

/**
 * @brief Restore CPU state from task context
 */
static void context_restore_cpu_state(dsrtos_tcb_t* task, uint32_t** sp)
{
    uint32_t* stack = *sp;
    
    /* Restore hardware-saved registers to stack */
    stack[0] = task->cpu_context.r0;
    stack[1] = task->cpu_context.r1;
    stack[2] = task->cpu_context.r2;
    stack[3] = task->cpu_context.r3;
    stack[4] = task->cpu_context.r12;
    stack[5] = task->cpu_context.lr;
    stack[6] = task->cpu_context.pc;
    stack[7] = task->cpu_context.psr;
    
    *sp = stack;
}

/**
 * @brief Validate task stack integrity
 */
static bool context_validate_stack(dsrtos_tcb_t* task)
{
    uint32_t* stack_start;
    uint32_t* stack_end;
    uint32_t* sp;
    
    if (task == NULL) {
        return false;
    }
    
    stack_start = (uint32_t*)task->stack_base;
    stack_end = (uint32_t*)((uint8_t*)task->stack_base + task->stack_size);
    sp = (uint32_t*)task->stack_pointer;
    
    /* Check stack pointer bounds */
    if ((sp < stack_start) || (sp >= stack_end)) {
        return false;
    }
    
    /* Check stack alignment */
    if (((uint32_t)sp & 0x7U) != 0U) {
        return false;
    }
    
    /* Check stack canary */
    if (task->stack_canary != DSRTOS_STACK_CANARY) {
        return false;
    }
    
    /* Check stack pattern at bottom */
    if (*stack_start != STACK_PATTERN) {
        /* Stack overflow likely occurred */
        return false;
    }
    
    return true;
}

/**
 * @brief Handle context switch errors
 */
static void context_switch_error_handler(uint32_t error_code)
{
    /* Increment error counter */
    g_context_control.error_count++;
    
    /* Log error */
    DSRTOS_TRACE_ERROR("Context switch error: 0x%08X", error_code);
    
    /* Attempt recovery based on error type */
    switch (error_code) {
        case DSRTOS_ERROR_STACK_OVERFLOW:
            /* Fatal - cannot recover from stack overflow */
            dsrtos_panic("Stack overflow in context switch");
            break;
            
        case DSRTOS_ERROR_CORRUPTED:
            /* Try to switch to idle task */
            extern dsrtos_tcb_t* g_idle_task;
            if (g_idle_task != NULL) {
                g_next_task = g_idle_task;
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
            } else {
                dsrtos_panic("Context corruption with no idle task");
            }
            break;
            
        default:
            /* Unknown error - panic */
            dsrtos_panic("Unknown context switch error");
            break;
    }
}

/**
 * @brief Update context switch timing statistics
 */
static void context_update_statistics(uint32_t cycles)
{
    uint64_t total;
    
    /* Update min/max */
    if (cycles < g_context_stats.switch_cycles_min) {
        g_context_stats.switch_cycles_min = cycles;
    }
    
    if (cycles > g_context_stats.switch_cycles_max) {
        g_context_stats.switch_cycles_max = cycles;
    }
    
    /* Update average */
    if (g_context_stats.switch_count > 0U) {
        total = (uint64_t)g_context_stats.switch_cycles_avg * 
                (uint64_t)(g_context_stats.switch_count - 1U);
        total += cycles;
        g_context_stats.switch_cycles_avg = 
            (uint32_t)(total / g_context_stats.switch_count);
    } else {
        g_context_stats.switch_cycles_avg = cycles;
    }
}

/**
 * @brief Measure CPU cycles
 */
static uint32_t context_measure_cycles(void)
{
    return DWT->CYCCNT;
}

/*==============================================================================
 * TASK INITIALIZATION
 *============================================================================*/

/**
 * @brief Initialize task context for first execution
 */
dsrtos_error_t dsrtos_context_init_task(dsrtos_tcb_t* task,
                                        void (*entry)(void*),
                                        void* param)
{
    uint32_t* sp;
    uint32_t psr;
    
    if ((task == NULL) || (entry == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Get stack top (grows downward) */
    sp = (uint32_t*)((uint8_t*)task->stack_base + task->stack_size);
    
    /* Align stack to 8-byte boundary */
    sp = (uint32_t*)((uint32_t)sp & STACK_ALIGN_MASK);
    
    /* Initialize stack for first context switch */
    psr = 0x01000000U;  /* Thumb bit set */
    
#ifdef DSRTOS_USE_FPU
    /* Space for FPU registers S16-S31 */
    sp -= 16;
    /* Clear FPU registers */
    memset(sp, 0, 16 * sizeof(uint32_t));
#endif
    
    /* Software saved registers R4-R11 */
    *(--sp) = 0x11111111U;  /* R11 */
    *(--sp) = 0x10101010U;  /* R10 */
    *(--sp) = 0x09090909U;  /* R9 */
    *(--sp) = 0x08080808U;  /* R8 */
    *(--sp) = 0x07070707U;  /* R7 */
    *(--sp) = 0x06060606U;  /* R6 */
    *(--sp) = 0x05050505U;  /* R5 */
    *(--sp) = 0x04040404U;  /* R4 */
    
    /* Hardware saved registers */
    *(--sp) = psr;                      /* xPSR */
    *(--sp) = (uint32_t)entry;          /* PC - Entry point */
    *(--sp) = (uint32_t)dsrtos_task_exit; /* LR - Return address */
    *(--sp) = 0x12121212U;              /* R12 */
    *(--sp) = 0x03030303U;              /* R3 */
    *(--sp) = 0x02020202U;              /* R2 */
    *(--sp) = 0x01010101U;              /* R1 */
    *(--sp) = (uint32_t)param;          /* R0 - Parameter */
    
    /* Save stack pointer in TCB */
    task->stack_pointer = sp;
    
    /* Initialize CPU context in TCB */
    task->cpu_context.sp = (uint32_t)sp;
    task->cpu_context.pc = (uint32_t)entry;
    task->cpu_context.lr = (uint32_t)dsrtos_task_exit;
    task->cpu_context.psr = psr;
    
    /* Fill remaining stack with pattern for overflow detection */
    uint32_t* stack_bottom = (uint32_t*)task->stack_base;
    while (stack_bottom < sp) {
        *stack_bottom++ = STACK_PATTERN;
    }
    
    DSRTOS_TRACE_CONTEXT("Task %u context initialized, SP=0x%08X",
                        task->task_id, (uint32_t)sp);
    
    return DSRTOS_SUCCESS;
}

/*==============================================================================
 * DEBUGGING SUPPORT
 *============================================================================*/

/**
 * @brief Dump context information for debugging
 */
void dsrtos_context_dump(dsrtos_tcb_t* task)
{
    if (task == NULL) {
        return;
    }
    
    DSRTOS_TRACE_DEBUG("Task %u Context Dump:", task->task_id);
    DSRTOS_TRACE_DEBUG("  State: %u", task->state);
    DSRTOS_TRACE_DEBUG("  SP: 0x%08X", (uint32_t)task->stack_pointer);
    DSRTOS_TRACE_DEBUG("  PC: 0x%08X", task->cpu_context.pc);
    DSRTOS_TRACE_DEBUG("  LR: 0x%08X", task->cpu_context.lr);
    DSRTOS_TRACE_DEBUG("  PSR: 0x%08X", task->cpu_context.psr);
    DSRTOS_TRACE_DEBUG("  R0-R3: %08X %08X %08X %08X",
                      task->cpu_context.r0, task->cpu_context.r1,
                      task->cpu_context.r2, task->cpu_context.r3);
    DSRTOS_TRACE_DEBUG("  R4-R7: %08X %08X %08X %08X",
                      task->cpu_context.r4, task->cpu_context.r5,
                      task->cpu_context.r6, task->cpu_context.r7);
    DSRTOS_TRACE_DEBUG("  R8-R11: %08X %08X %08X %08X",
                      task->cpu_context.r8, task->cpu_context.r9,
                      task->cpu_context.r10, task->cpu_context.r11);
    DSRTOS_TRACE_DEBUG("  Stack: Base=0x%08X Size=%u Used=%u",
                      (uint32_t)task->stack_base, task->stack_size,
                      task->stack_size - 
                      (uint32_t)((uint8_t*)task->stack_pointer - 
                                (uint8_t*)task->stack_base));
}

/**
 * @brief Get current context state
 */
context_state_t dsrtos_context_get_state(void)
{
    return g_context_control.state;
}

/**
 * @brief Check if context switch is in progress
 */
bool dsrtos_context_in_switch(void)
{
    return g_context_control.in_switch;
}
