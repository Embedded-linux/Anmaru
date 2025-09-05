@/*
@ * DSRTOS - Dynamic Scheduler Real-Time Operating System
@ * Phase 8: PendSV Handler - Optimized ARM Cortex-M4 Assembly
@ * 
@ * Cycle-optimized context switching handler
@ * Target: < 200 CPU cycles for complete context switch
@ * 
@ * Copyright (c) 2025 DSRTOS Development Team
@ * SPDX-License-Identifier: MIT
@ */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

    .section .text.PendSV_Handler
    .global PendSV_Handler
    .type PendSV_Handler, %function

@ ============================================================================
@ External symbols
@ ============================================================================
    .extern g_current_context
    .extern g_next_context
    .extern g_context_switch_count
    .extern g_context_switch_cycles_total

@ ============================================================================
@ Equates for register addresses and bit positions
@ ============================================================================
    .equ SCB_ICSR,          0xE000ED04      @ Interrupt Control State Register
    .equ FPU_FPCCR,         0xE000EF34      @ FP Context Control Register
    .equ SYSTICK_VAL,       0xE000E018      @ SysTick current value
    
    .equ FPCCR_LSPACT_BIT,  0x00000001      @ Lazy state active bit
    .equ EXC_RETURN_FPU,    0xFFFFFFED      @ Thread mode, PSP, FPU
    .equ EXC_RETURN_NO_FPU, 0xFFFFFFFD      @ Thread mode, PSP, no FPU

@ ============================================================================
@ PendSV_Handler - Optimized context switch handler
@ Cycles breakdown (worst case with FPU):
@   Entry overhead:        ~12 cycles
@   Save context:          ~40 cycles (with FPU check)
@   Load next context:     ~35 cycles
@   Restore context:       ~40 cycles (with FPU check)
@   Exit overhead:         ~12 cycles
@   Total:                ~139 cycles (target: <200)
@ ============================================================================

    .align 4
PendSV_Handler:
    @ Disable interrupts to protect context switch
    cpsid   i                           @ 1 cycle
    
    @ Get current PSP
    mrs     r0, psp                     @ 2 cycles
    
    @ Check if FPU context needs saving (lazy stacking)
    ldr     r3, =FPU_FPCCR             @ 2 cycles
    ldr     r2, [r3]                   @ 2 cycles
    tst     r2, #FPCCR_LSPACT_BIT      @ 1 cycle
    beq     .Lno_fpu_save              @ 1-3 cycles (usually not taken)
    
    @ Save FPU context (S16-S31)
    vstmdb  r0!, {s16-s31}             @ 17 cycles
    
.Lno_fpu_save:
    @ Save core registers (R4-R11, LR)
    stmdb   r0!, {r4-r11, lr}          @ 10 cycles
    
    @ Load current context pointer
    ldr     r3, =g_current_context      @ 2 cycles
    ldr     r1, [r3]                   @ 2 cycles
    
    @ Save current stack pointer
    str     r0, [r1]                   @ 2 cycles
    
    @ Increment context switch counter (for statistics)
    ldr     r2, =g_context_switch_count @ 2 cycles
    ldr     r4, [r2]                   @ 2 cycles
    adds    r4, r4, #1                 @ 1 cycle
    str     r4, [r2]                   @ 2 cycles
    
    @ Load next context pointer
    ldr     r3, =g_next_context        @ 2 cycles
    ldr     r1, [r3]                   @ 2 cycles
    
    @ Update current context to next
    ldr     r2, =g_current_context     @ 2 cycles
    str     r1, [r2]                   @ 2 cycles
    
    @ Clear next context pointer
    movs    r2, #0                     @ 1 cycle
    str     r2, [r3]                   @ 2 cycles
    
    @ Load new stack pointer
    ldr     r0, [r1]                   @ 2 cycles
    
    @ Check if we have MPU regions to switch
    ldr     r2, [r1, #8]               @ 2 cycles (mpu.enabled offset)
    cbz     r2, .Lno_mpu_switch        @ 1-3 cycles
    
    @ Fast MPU region switch (optimized for 2 regions)
    @ This is a common case for task isolation
    push    {r0, r1}                   @ 2 cycles
    bl      dsrtos_mpu_fast_switch     @ 4 cycles + function
    pop     {r0, r1}                   @ 2 cycles
    
.Lno_mpu_switch:
    @ Restore core registers (R4-R11, LR)
    ldmia   r0!, {r4-r11, lr}          @ 10 cycles
    
    @ Check if FPU context needs restoring
    tst     lr, #0x10                  @ 1 cycle (check bit 4 of EXC_RETURN)
    bne     .Lno_fpu_restore           @ 1-3 cycles
    
    @ Restore FPU context (S16-S31)
    vldmia  r0!, {s16-s31}             @ 17 cycles
    
.Lno_fpu_restore:
    @ Update PSP
    msr     psp, r0                    @ 2 cycles
    
    @ Ensure instructions are completed before enabling interrupts
    isb                                @ 2-4 cycles
    
    @ Re-enable interrupts
    cpsie   i                          @ 1 cycle
    
    @ Return using restored LR (EXC_RETURN value)
    bx      lr                         @ 3 cycles
    
    .size PendSV_Handler, .-PendSV_Handler

@ ============================================================================
@ dsrtos_mpu_fast_switch - Fast MPU region switching
@ Optimized for 2 regions (common case)
@ Input: r1 = context pointer
@ Cycles: ~25 cycles for 2 regions
@ ============================================================================
    .align 4
    .global dsrtos_mpu_fast_switch
    .type dsrtos_mpu_fast_switch, %function

dsrtos_mpu_fast_switch:
    push    {r4-r7}                    @ 5 cycles
    
    @ Load MPU base address
    ldr     r4, =0xE000ED9C            @ 2 cycles (MPU_RNR address)
    
    @ Configure region 0
    movs    r5, #0                     @ 1 cycle
    str     r5, [r4]                   @ 2 cycles (select region 0)
    
    ldr     r6, [r1, #16]              @ 2 cycles (regions[0].rbar)
    ldr     r7, [r1, #20]              @ 2 cycles (regions[0].rasr)
    str     r6, [r4, #4]               @ 2 cycles (MPU_RBAR)
    str     r7, [r4, #8]               @ 2 cycles (MPU_RASR)
    
    @ Configure region 1
    movs    r5, #1                     @ 1 cycle
    str     r5, [r4]                   @ 2 cycles (select region 1)
    
    ldr     r6, [r1, #24]              @ 2 cycles (regions[1].rbar)
    ldr     r7, [r1, #28]              @ 2 cycles (regions[1].rasr)
    str     r6, [r4, #4]               @ 2 cycles (MPU_RBAR)
    str     r7, [r4, #8]               @ 2 cycles (MPU_RASR)
    
    @ Memory barrier
    dsb                                @ 2-4 cycles
    isb                                @ 2-4 cycles
    
    pop     {r4-r7}                    @ 5 cycles
    bx      lr                         @ 3 cycles
    
    .size dsrtos_mpu_fast_switch, .-dsrtos_mpu_fast_switch

@ ============================================================================
@ SVC_Handler - System call handler for first context switch
@ ============================================================================
    .align 4
    .global SVC_Handler
    .type SVC_Handler, %function

SVC_Handler:
    @ Get the SVC number
    mrs     r0, psp
    ldr     r1, [r0, #24]              @ Get PC from stack frame
    ldrb    r2, [r1, #-2]              @ Get SVC number
    
    @ Check for context switch request (SVC #0)
    cbnz    r2, .Lsvc_not_switch
    
    @ Perform first context switch
    @ Load first task context
    ldr     r3, =g_next_context
    ldr     r1, [r3]
    ldr     r2, =g_current_context
    str     r1, [r2]
    
    @ Clear next context
    movs    r2, #0
    str     r2, [r3]
    
    @ Load stack pointer from context
    ldr     r0, [r1]
    
    @ Load registers from stack
    ldmia   r0!, {r4-r11, lr}
    
    @ Set PSP
    msr     psp, r0
    
    @ Switch to use PSP
    movs    r0, #2
    msr     control, r0
    isb
    
    @ Return to thread mode
    bx      lr
    
.Lsvc_not_switch:
    @ Handle other SVC calls
    b       dsrtos_svc_handler_c
    
    .size SVC_Handler, .-SVC_Handler

@ ============================================================================
@ HardFault_Handler - Optimized hard fault handler
@ ============================================================================
    .align 4
    .global HardFault_Handler
    .type HardFault_Handler, %function

HardFault_Handler:
    @ Determine which stack was used
    tst     lr, #4
    ite     eq
    mrseq   r0, msp
    mrsne   r0, psp
    
    @ Pass stack pointer to C handler
    b       dsrtos_hardfault_handler_c
    
    .size HardFault_Handler, .-HardFault_Handler

@ ============================================================================
@ Fast context switch primitives
@ ============================================================================

@ dsrtos_context_switch_asm - Trigger context switch
    .align 4
    .global dsrtos_context_switch_asm
    .type dsrtos_context_switch_asm, %function

dsrtos_context_switch_asm:
    @ Set PendSV
    ldr     r0, =SCB_ICSR
    ldr     r1, =0x10000000            @ PENDSVSET bit
    str     r1, [r0]
    
    @ Memory barriers
    dsb
    isb
    
    bx      lr
    
    .size dsrtos_context_switch_asm, .-dsrtos_context_switch_asm

@ dsrtos_get_cpu_cycles - Get current CPU cycle count
    .align 4
    .global dsrtos_get_cpu_cycles
    .type dsrtos_get_cpu_cycles, %function

dsrtos_get_cpu_cycles:
    @ Read DWT cycle counter
    ldr     r0, =0xE0001004            @ DWT_CYCCNT
    ldr     r0, [r0]
    bx      lr
    
    .size dsrtos_get_cpu_cycles, .-dsrtos_get_cpu_cycles

@ dsrtos_enable_cycle_counter - Enable DWT cycle counter
    .align 4
    .global dsrtos_enable_cycle_counter
    .type dsrtos_enable_cycle_counter, %function

dsrtos_enable_cycle_counter:
    @ Enable DWT and cycle counter
    ldr     r0, =0xE0001000            @ DWT_CTRL
    ldr     r1, [r0]
    orr     r1, r1, #1                 @ Set CYCCNTENA bit
    str     r1, [r0]
    
    @ Reset cycle counter
    ldr     r0, =0xE0001004            @ DWT_CYCCNT
    movs    r1, #0
    str     r1, [r0]
    
    bx      lr
    
    .size dsrtos_enable_cycle_counter, .-dsrtos_enable_cycle_counter

@ ============================================================================
@ Stack initialization with pattern fill
@ ============================================================================
    .align 4
    .global dsrtos_stack_init_asm
    .type dsrtos_stack_init_asm, %function

dsrtos_stack_init_asm:
    @ Parameters:
    @ r0 = stack top
    @ r1 = task entry point
    @ r2 = task parameter
    @ r3 = exit handler
    
    @ Align stack to 8 bytes
    bic     r0, r0, #7
    
    @ Build initial stack frame (hardware saved registers)
    @ xPSR
    ldr     r12, =0x01000000           @ Thumb state
    stmdb   r0!, {r12}
    
    @ PC (task entry point)
    stmdb   r0!, {r1}
    
    @ LR (exit handler)
    stmdb   r0!, {r3}
    
    @ R12, R3, R2, R1
    movs    r12, #0
    stmdb   r0!, {r12}                 @ R12
    stmdb   r0!, {r12}                 @ R3
    stmdb   r0!, {r12}                 @ R2  
    stmdb   r0!, {r12}                 @ R1
    
    @ R0 (task parameter)
    stmdb   r0!, {r2}
    
    @ Software saved registers (R4-R11)
    movs    r1, #0
    movs    r2, #0
    movs    r3, #0
    movs    r4, #0
    movs    r5, #0
    movs    r6, #0
    movs    r7, #0
    movs    r12, #0
    
    @ LR with EXC_RETURN value
    ldr     lr, =EXC_RETURN_NO_FPU
    
    @ Save R4-R11, LR
    stmdb   r0!, {r1-r7, r12, lr}
    
    @ Return stack pointer
    bx      lr
    
    .size dsrtos_stack_init_asm, .-dsrtos_stack_init_asm

    .end
