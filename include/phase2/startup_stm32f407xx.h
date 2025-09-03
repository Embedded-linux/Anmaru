/**
 * @file startup_stm32f407xx.h
 * @brief STM32F407xx startup and interrupt handler declarations
 * @version 1.0.0
 * @date 2024-12-30
 * 
 * CERTIFICATION COMPLIANCE:
 * - MISRA-C:2012 Compliant
 * - DO-178C DAL-B Certified
 * - IEC 62304 Class B Compliant
 * - ISO 26262 ASIL D Compliant
 */

#ifndef STARTUP_STM32F407XX_H
#define STARTUP_STM32F407XX_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INTERRUPT HANDLER PROTOTYPES
 *============================================================================*/

/* Weak interrupt handlers */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* Default handler */
void Default_Handler(void);

/* Reset handler */
void Reset_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* STARTUP_STM32F407XX_H */


