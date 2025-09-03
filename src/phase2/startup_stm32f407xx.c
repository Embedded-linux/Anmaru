/**
 * @file startup_stm32f407xx.c
 * @brief STM32F407VG startup code
 */

#include "stm32f4xx.h"
#include "startup_stm32f407xx.h"

/* External symbols */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern int main(void);

/* Function declarations are now available from headers */

/* Weak interrupt handlers */
__attribute__((weak)) void NMI_Handler(void) { Default_Handler(); }
__attribute__((weak)) void HardFault_Handler(void) { Default_Handler(); }
__attribute__((weak)) void MemManage_Handler(void) { Default_Handler(); }
__attribute__((weak)) void BusFault_Handler(void) { Default_Handler(); }
__attribute__((weak)) void UsageFault_Handler(void) { Default_Handler(); }
__attribute__((weak)) void SVC_Handler(void) { Default_Handler(); }
__attribute__((weak)) void DebugMon_Handler(void) { Default_Handler(); }
__attribute__((weak)) void PendSV_Handler(void) { Default_Handler(); }
__attribute__((weak)) void SysTick_Handler(void) { Default_Handler(); }

/* Vector table */
__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(uintptr_t)(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler
};

void Reset_Handler(void)
{
    uint32_t* pSrc = &_sidata;
    uint32_t* pDest = &_sdata;
    
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }
    
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }
    
    SystemInit();
    
    (void)main();
    
    while (1) {
        __asm volatile ("nop");
    }
}

void Default_Handler(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    while (1) {
        __asm volatile ("nop");
    }
}
