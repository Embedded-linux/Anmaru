/*
 * DSRTOS Context Switch Assembly Implementation
 * ARM Cortex-M4 with FPU Support
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B
 * Safety-Critical Context Switching
 */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

    /* External symbols */
    .global PendSV_Handler
    .global dsrtos_start_first_task
    .global dsrtos_switch_context
    .global g_current_task
    .global g_next_task
    .global dsrtos_switch_context_handler

    /* Constants */
    .equ NVIC_INT_CTRL,     0xE000ED04
    .equ NVIC_PENDSVSET,    0x10000000
    .equ NVIC_SYSPRI14,     0xE000ED22
    .equ NVIC_PENDSV_PRI,   0xFF
    .equ FPU_FPCCR,         0xE000EF34

    .section .text.PendSV_Handler
    .type PendSV_Handler, %function
    .thumb_func

/*
 * PendSV Exception Handler
 * Performs the actual context switch
 */
PendSV_Handler:
    /* Disable interrupts to prevent corruption */
    cpsid   i
    isb
    
    /* Get current task TCB pointer */
    ldr     r3, =g_current_task
    ldr     r1, [r3]
    
    /* Skip context save if first switch (current_task == NULL) */
    cbz     r1, context_restore
    
    /* Save current context */
    mrs     r0, psp                 /* Get process stack pointer */
    isb
    
    /* Check FPU usage and save if needed */
#ifdef DSRTOS_USE_FPU
    tst     lr, #0x10              /* Test bit 4 of LR (FPU usage) */
    it      eq
    vstmdbeq r0!, {s16-s31}        /* Save FPU registers if used */
#endif
    
    /* Save core registers R4-R11 */
    stmdb   r0!, {r4-r11}          /* Save R4-R11 on process stack */
    
    /* Save stack pointer to current task's TCB */
    str     r0, [r1, #0]           /* TCB->stack_pointer = PSP */
    
    /* Call C handler for statistics and validation */
    push    {r14}                  /* Save LR */
    bl      dsrtos_switch_context_handler
    pop     {r14}                  /* Restore LR */

context_restore:
    /* Get next task TCB pointer */
    ldr     r3, =g_next_task
    ldr     r1, [r3]
    
    /* Update current task pointer */
    ldr     r3, =g_current_task
    str     r1, [r3]
    
    /* Get new task's stack pointer from TCB */
    ldr     r0, [r1, #0]           /* PSP = TCB->stack_pointer */
    
    /* Restore core registers R4-R11 */
    ldmia   r0!, {r4-r11}          /* Restore R4-R11 from process stack */
    
    /* Restore FPU registers if needed */
#ifdef DSRTOS_USE_FPU
    tst     lr, #0x10              /* Test bit 4 of LR */
    it      eq
    vldmiaeq r0!, {s16-s31}        /* Restore FPU registers if used */
#endif
    
    /* Set PSP to new task's stack */
    msr     psp, r0
    isb
    
    /* Ensure we return using PSP */
    orr     lr, lr, #0x04
    
    /* Enable interrupts */
    cpsie   i
    
    /* Return - will restore remaining context */
    bx      lr
    
    .size PendSV_Handler, .-PendSV_Handler

/*
 * Start first task
 * Initialize and switch to first task
 */
    .section .text.dsrtos_start_first_task
    .type dsrtos_start_first_task, %function
    .thumb_func

dsrtos_start_first_task:
    /* Set PendSV to lowest priority */
    ldr     r0, =NVIC_SYSPRI14
    ldr     r1, =NVIC_PENDSV_PRI
    strb    r1, [r0]
    
    /* Set PSP to 0 for initial task */
    movs    r0, #0
    msr     psp, r0
    
    /* Set CONTROL register to use PSP */
    movs    r0, #2
    msr     control, r0
    isb
    
    /* Trigger PendSV to start first task */
    ldr     r0, =NVIC_INT_CTRL
    ldr     r1, =NVIC_PENDSVSET
    str     r1, [r0]
    
    /* Enable interrupts */
    cpsie   i
    cpsie   f
    dsb
    isb
    
    /* Should never reach here */
1:  b       1b
    
    .size dsrtos_start_first_task, .-dsrtos_start_first_task

/*
 * Switch context immediately
 * Trigger PendSV for context switch
 */
    .section .text.dsrtos_switch_context
    .type dsrtos_switch_context, %function
    .thumb_func

dsrtos_switch_context:
    /* Trigger PendSV exception */
    ldr     r0, =NVIC_INT_CTRL
    ldr     r1, =NVIC_PENDSVSET
    str     r1, [r0]
    
    /* Return */
    bx      lr
    
    .size dsrtos_switch_context, .-dsrtos_switch_context

    .end
