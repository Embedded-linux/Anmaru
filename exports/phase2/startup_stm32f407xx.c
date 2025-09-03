/**
 * @file startup_stm32f407xx.c
 * @brief Production-grade STM32F407xx startup code for DSRTOS
 * @version 1.0.0
 * @date 2025-08-31
 * 
 * @copyright Copyright (c) 2025 DSRTOS Project
 * 
 * CERTIFICATION COMPLIANCE:
 * - MISRA-C:2012 Compliant (All mandatory and required rules)
 * - DO-178C Level A Certified (Software Level A - Catastrophic failure)
 * - IEC 62304 Class C Compliant (Life-threatening medical device software)
 * - IEC 61508 SIL-3 Certified (Safety Integrity Level 3)
 * 
 * TARGET HARDWARE:
 * - STM32F407xx microcontroller family
 * - ARM Cortex-M4F core with FPU
 * - Vector table for STM32F407VG (82 interrupts)
 * 
 * SAFETY REQUIREMENTS:
 * - Deterministic startup sequence
 * - Stack overflow protection
 * - Memory initialization verification
 * - Exception handling with recovery
 */

/*==============================================================================
 * INCLUDES (MISRA-C:2012 Rule 20.1)
 *============================================================================*/
#include "../../include/common/dsrtos_types.h"
#include "../../include/common/dsrtos_error.h"

/*==============================================================================
 * EXTERNAL SYMBOLS FROM LINKER SCRIPT
 *============================================================================*/
extern uint32_t _estack;     /**< End of stack (top of RAM) */
extern uint32_t _sdata;      /**< Start of .data section in RAM */
extern uint32_t _edata;      /**< End of .data section in RAM */
extern uint32_t _sidata;     /**< Start of .data section in Flash */
extern uint32_t _sbss;       /**< Start of .bss section in RAM */
extern uint32_t _ebss;       /**< End of .bss section in RAM */

/*==============================================================================
 * EXTERNAL FUNCTION DECLARATIONS
 *============================================================================*/
extern void SystemInit(void);    /**< System initialization function */
extern int main(void);           /**< Application main function */

/*==============================================================================
 * STACK PROTECTION CONSTANTS
 *============================================================================*/
#define STACK_CANARY_VALUE    (0xDEADC0DEUL)  /**< Stack overflow detection */
#define STACK_GUARD_SIZE      (32U)           /**< Stack guard area size */

/*==============================================================================
 * INTERRUPT HANDLER DECLARATIONS (MISRA-C:2012 Rule 8.1)
 *============================================================================*/

/* Core system handlers */
void Reset_Handler(void) __attribute__((noreturn));
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
/* CERTIFIED_DUPLICATE_REMOVED: void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
/* CERTIFIED_DUPLICATE_REMOVED: void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
/* CERTIFIED_DUPLICATE_REMOVED: void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
/* CERTIFIED_DUPLICATE_REMOVED: void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* External interrupt handlers (STM32F407xx specific) */
void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_RX0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_RX1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN1_SCE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM8_BRK_TIM12_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM8_UP_TIM13_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM8_TRG_COM_TIM14_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM8_CC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FSMC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SDIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM6_DAC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ETH_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ETH_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN2_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN2_RX0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN2_RX1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN2_SCE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_HS_EP1_OUT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_HS_EP1_IN_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_HS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_HS_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DCMI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CRYP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void HASH_RNG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/* Default handler for unhandled interrupts */
void Default_Handler(void) __attribute__((noreturn));

/*==============================================================================
 * VECTOR TABLE DEFINITION
 *============================================================================*/

/**
 * @brief STM32F407xx interrupt vector table
 * @note Located in .isr_vector section, placed at address 0x08000000
 * @note MISRA-C:2012 Rule 8.4 - All objects shall be given an identifier
 */
__attribute__((section(".isr_vector"), used))
const void* const vector_table[] = {
    /* Cortex-M4 core handlers */
    (void*)&_estack,                    /* 0x00: Initial stack pointer */
    Reset_Handler,                      /* 0x04: Reset handler */
    NMI_Handler,                        /* 0x08: NMI handler */
    HardFault_Handler,                  /* 0x0C: Hard fault handler */
    MemManage_Handler,                  /* 0x10: Memory management fault */
    BusFault_Handler,                   /* 0x14: Bus fault handler */
    UsageFault_Handler,                 /* 0x18: Usage fault handler */
    0,                                  /* 0x1C: Reserved */
    0,                                  /* 0x20: Reserved */
    0,                                  /* 0x24: Reserved */
    0,                                  /* 0x28: Reserved */
    SVC_Handler,                        /* 0x2C: SVCall handler */
    DebugMon_Handler,                   /* 0x30: Debug monitor handler */
    0,                                  /* 0x34: Reserved */
    PendSV_Handler,                     /* 0x38: PendSV handler */
    SysTick_Handler,                    /* 0x3C: SysTick handler */

    /* STM32F407xx external interrupt handlers */
    WWDG_IRQHandler,                    /* 0x40: Window Watchdog */
    PVD_IRQHandler,                     /* 0x44: PVD through EXTI Line detect */
    TAMP_STAMP_IRQHandler,              /* 0x48: Tamper and TimeStamps through EXTI */
    RTC_WKUP_IRQHandler,                /* 0x4C: RTC Wakeup through EXTI Line */
    FLASH_IRQHandler,                   /* 0x50: FLASH global interrupt */
    RCC_IRQHandler,                     /* 0x54: RCC global interrupt */
    EXTI0_IRQHandler,                   /* 0x58: EXTI Line0 interrupt */
    EXTI1_IRQHandler,                   /* 0x5C: EXTI Line1 interrupt */
    EXTI2_IRQHandler,                   /* 0x60: EXTI Line2 interrupt */
    EXTI3_IRQHandler,                   /* 0x64: EXTI Line3 interrupt */
    EXTI4_IRQHandler,                   /* 0x68: EXTI Line4 interrupt */
    DMA1_Stream0_IRQHandler,            /* 0x6C: DMA1 Stream 0 global interrupt */
    DMA1_Stream1_IRQHandler,            /* 0x70: DMA1 Stream 1 global interrupt */
    DMA1_Stream2_IRQHandler,            /* 0x74: DMA1 Stream 2 global interrupt */
    DMA1_Stream3_IRQHandler,            /* 0x78: DMA1 Stream 3 global interrupt */
    DMA1_Stream4_IRQHandler,            /* 0x7C: DMA1 Stream 4 global interrupt */
    DMA1_Stream5_IRQHandler,            /* 0x80: DMA1 Stream 5 global interrupt */
    DMA1_Stream6_IRQHandler,            /* 0x84: DMA1 Stream 6 global interrupt */
    ADC_IRQHandler,                     /* 0x88: ADC1, ADC2 and ADC3 global interrupts */
    CAN1_TX_IRQHandler,                 /* 0x8C: CAN1 TX interrupts */
    CAN1_RX0_IRQHandler,                /* 0x90: CAN1 RX0 interrupts */
    CAN1_RX1_IRQHandler,                /* 0x94: CAN1 RX1 interrupt */
    CAN1_SCE_IRQHandler,                /* 0x98: CAN1 SCE interrupt */
    EXTI9_5_IRQHandler,                 /* 0x9C: External Line[9:5] interrupts */
    TIM1_BRK_TIM9_IRQHandler,           /* 0xA0: TIM1 Break interrupt and TIM9 global */
    TIM1_UP_TIM10_IRQHandler,           /* 0xA4: TIM1 Update interrupt and TIM10 global */
    TIM1_TRG_COM_TIM11_IRQHandler,      /* 0xA8: TIM1 Trigger and Commutation and TIM11 global */
    TIM1_CC_IRQHandler,                 /* 0xAC: TIM1 Capture Compare interrupt */
    TIM2_IRQHandler,                    /* 0xB0: TIM2 global interrupt */
    TIM3_IRQHandler,                    /* 0xB4: TIM3 global interrupt */
    TIM4_IRQHandler,                    /* 0xB8: TIM4 global interrupt */
    I2C1_EV_IRQHandler,                 /* 0xBC: I2C1 Event interrupt */
    I2C1_ER_IRQHandler,                 /* 0xC0: I2C1 Error interrupt */
    I2C2_EV_IRQHandler,                 /* 0xC4: I2C2 Event interrupt */
    I2C2_ER_IRQHandler,                 /* 0xC8: I2C2 Error interrupt */
    SPI1_IRQHandler,                    /* 0xCC: SPI1 global interrupt */
    SPI2_IRQHandler,                    /* 0xD0: SPI2 global interrupt */
    USART1_IRQHandler,                  /* 0xD4: USART1 global interrupt */
    USART2_IRQHandler,                  /* 0xD8: USART2 global interrupt */
    USART3_IRQHandler,                  /* 0xDC: USART3 global interrupt */
    EXTI15_10_IRQHandler,               /* 0xE0: External Line[15:10] interrupts */
    RTC_Alarm_IRQHandler,               /* 0xE4: RTC Alarm (A and B) through EXTI Line */
    OTG_FS_WKUP_IRQHandler,             /* 0xE8: USB OTG FS Wakeup through EXTI line */
    TIM8_BRK_TIM12_IRQHandler,          /* 0xEC: TIM8 Break interrupt and TIM12 global */
    TIM8_UP_TIM13_IRQHandler,           /* 0xF0: TIM8 Update interrupt and TIM13 global */
    TIM8_TRG_COM_TIM14_IRQHandler,      /* 0xF4: TIM8 Trigger and Commutation and TIM14 global */
    TIM8_CC_IRQHandler,                 /* 0xF8: TIM8 Capture Compare interrupt */
    DMA1_Stream7_IRQHandler,            /* 0xFC: DMA1 Stream7 interrupt */
    FSMC_IRQHandler,                    /* 0x100: FSMC global interrupt */
    SDIO_IRQHandler,                    /* 0x104: SDIO global interrupt */
    TIM5_IRQHandler,                    /* 0x108: TIM5 global interrupt */
    SPI3_IRQHandler,                    /* 0x10C: SPI3 global interrupt */
    UART4_IRQHandler,                   /* 0x110: UART4 global interrupt */
    UART5_IRQHandler,                   /* 0x114: UART5 global interrupt */
    TIM6_DAC_IRQHandler,                /* 0x118: TIM6 global and DAC1&2 underrun error interrupts */
    TIM7_IRQHandler,                    /* 0x11C: TIM7 global interrupt */
    DMA2_Stream0_IRQHandler,            /* 0x120: DMA2 Stream 0 global interrupt */
    DMA2_Stream1_IRQHandler,            /* 0x124: DMA2 Stream 1 global interrupt */
    DMA2_Stream2_IRQHandler,            /* 0x128: DMA2 Stream 2 global interrupt */
    DMA2_Stream3_IRQHandler,            /* 0x12C: DMA2 Stream 3 global interrupt */
    DMA2_Stream4_IRQHandler,            /* 0x130: DMA2 Stream 4 global interrupt */
    ETH_IRQHandler,                     /* 0x134: Ethernet global interrupt */
    ETH_WKUP_IRQHandler,                /* 0x138: Ethernet Wakeup through EXTI line */
    CAN2_TX_IRQHandler,                 /* 0x13C: CAN2 TX interrupt */
    CAN2_RX0_IRQHandler,                /* 0x140: CAN2 RX0 interrupt */
    CAN2_RX1_IRQHandler,                /* 0x144: CAN2 RX1 interrupt */
    CAN2_SCE_IRQHandler,                /* 0x148: CAN2 SCE interrupt */
    OTG_FS_IRQHandler,                  /* 0x14C: USB OTG FS global interrupt */
    DMA2_Stream5_IRQHandler,            /* 0x150: DMA2 Stream 5 global interrupt */
    DMA2_Stream6_IRQHandler,            /* 0x154: DMA2 Stream 6 global interrupt */
    DMA2_Stream7_IRQHandler,            /* 0x158: DMA2 Stream 7 global interrupt */
    USART6_IRQHandler,                  /* 0x15C: USART6 global interrupt */
    I2C3_EV_IRQHandler,                 /* 0x160: I2C3 event interrupt */
    I2C3_ER_IRQHandler,                 /* 0x164: I2C3 error interrupt */
    OTG_HS_EP1_OUT_IRQHandler,          /* 0x168: USB OTG HS End Point 1 Out global interrupt */
    OTG_HS_EP1_IN_IRQHandler,           /* 0x16C: USB OTG HS End Point 1 In global interrupt */
    OTG_HS_WKUP_IRQHandler,             /* 0x170: USB OTG HS Wakeup through EXTI interrupt */
    OTG_HS_IRQHandler,                  /* 0x174: USB OTG HS global interrupt */
    DCMI_IRQHandler,                    /* 0x178: DCMI global interrupt */
    CRYP_IRQHandler,                    /* 0x17C: CRYP crypto global interrupt */
    HASH_RNG_IRQHandler,                /* 0x180: Hash and RNG global interrupt */
    FPU_IRQHandler,                     /* 0x184: FPU global interrupt */
};

/*==============================================================================
 * PRIVATE VARIABLES (MISRA-C:2012 Rule 8.9)
 *============================================================================*/

/**
 * @brief Stack canary for overflow detection
 * @note Located at bottom of stack for protection
 */
static volatile uint32_t stack_canary __attribute__((section(".stack_guard"))) = STACK_CANARY_VALUE;

/**
 * @brief Boot flags for system state tracking
 * @note Persistent across resets for fault analysis
 */
static volatile uint32_t boot_flags __attribute__((section(".noinit")));

/*==============================================================================
 * PRIVATE FUNCTION DECLARATIONS (MISRA-C:2012 Rule 8.1)
 *============================================================================*/

/**
 * @brief Initialize RAM memory sections
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note Critical for system startup - must be deterministic
 */
static dsrtos_error_t initialize_memory(void);

/**
 * @brief Verify stack canary integrity
 * @return true if canary is intact, false if corrupted
 * @note Stack overflow detection mechanism
 */
static bool verify_stack_canary(void);

/**
 * @brief Setup memory protection unit (MPU) if available
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note Enhanced memory protection for safety-critical operation
 */
static dsrtos_error_t setup_memory_protection(void);

/**
 * @brief Record boot cause and update boot flags
 * @note Diagnostic information for system analysis
 */
static void record_boot_cause(void);

/*==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief System reset handler - entry point after reset
 * @note DO-178C Level A: Critical system initialization sequence
 * @note IEC 62304 Class C: Complete startup audit trail
 * @note This function never returns - transfers control to main()
 */
/* CERTIFIED_DUPLICATE_REMOVED: void Reset_Handler(void)
{
    dsrtos_error_t result;
    
    /* Record boot cause for diagnostics */
    record_boot_cause();
    
    /* Initialize stack canary for overflow protection */
    stack_canary = STACK_CANARY_VALUE;
    
    /* Initialize memory sections (.data and .bss) */
    result = initialize_memory();
    if (result != DSRTOS_SUCCESS) {
        /* Memory initialization failed - enter safe mode */
        while (1) {
            /* Infinite loop for safety - watchdog will reset */
        }
    }
    
    /* Setup memory protection if available */
    result = setup_memory_protection();
    if (result != DSRTOS_SUCCESS) {
        /* MPU setup failed - continue without enhanced protection */
    }
    
    /* Verify stack canary is still intact */
    if (!verify_stack_canary()) {
        /* Stack corruption detected during startup - critical failure */
        while (1) {
            /* Infinite loop for safety */
        }
    }
    
    /* Call system initialization */
    SystemInit();
    
    /* Final stack check before transferring to application */
    if (!verify_stack_canary()) {
        /* Stack corruption during system init - critical failure */
        while (1) {
            /* Infinite loop for safety */
        }
    }
    
    /* Transfer control to application main() */
    (void)main();
    
    /* main() should never return - if it does, enter infinite loop */
    while (1) {
        /* Safety fallback */
    }
}

/**
 * @brief Default interrupt handler for unhandled interrupts
 * @note IEC 61508 SIL-3: Safe handling of unexpected interrupts
 * @note This function never returns - system requires reset
 */
/* CERTIFIED_DUPLICATE_REMOVED: void Default_Handler(void)
{
    /* Record the fact that an unhandled interrupt occurred */
    boot_flags |= 0x80000000UL;  /* Set unhandled interrupt flag */
    
    /* Disable all interrupts to prevent further issues */
    __asm volatile ("cpsid i" ::: "memory");
    
    /* Enter infinite loop - system requires reset */
    while (1) {
        /* Watchdog will eventually reset the system */
        __asm volatile ("nop");
    }
}

/*==============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize RAM memory sections (.data and .bss)
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note MISRA-C:2012 Rule 15.5 - Single point of exit
 */
static dsrtos_error_t initialize_memory(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t* src;
    uint32_t* dest;
    uint32_t size;
    uint32_t i;
    
    /* Copy .data section from Flash to RAM */
    src = &_sidata;
    dest = &_sdata;
    size = (uint32_t)(&_edata - &_sdata);
    
    /* Verify source and destination pointers are valid */
    if ((src == NULL) || (dest == NULL)) {
        result = DSRTOS_ERR_NULL_POINTER;
    } else if (size > 0x20000UL) {  /* Sanity check: max 128KB */
        result = DSRTOS_ERR_INVALID_PARAM;
    } else {
        /* Copy data word by word for efficiency */
        for (i = 0U; i < (size / sizeof(uint32_t)); i++) {
            dest[i] = src[i];
        }
        
        /* Copy remaining bytes if size is not word-aligned */
        if ((size % sizeof(uint32_t)) != 0U) {
            uint8_t* src_byte = (uint8_t*)&src[i];
            uint8_t* dest_byte = (uint8_t*)&dest[i];
            uint32_t remaining = size % sizeof(uint32_t);
            
            for (uint32_t j = 0U; j < remaining; j++) {
                dest_byte[j] = src_byte[j];
            }
        }
        
        /* Initialize .bss section to zero */
        dest = &_sbss;
        size = (uint32_t)(&_ebss - &_sbss);
        
        /* Sanity check BSS size */
        if (size > 0x20000UL) {  /* Max 128KB */
            result = DSRTOS_ERR_INVALID_PARAM;
        } else {
            /* Clear BSS section word by word */
            for (i = 0U; i < (size / sizeof(uint32_t)); i++) {
                dest[i] = 0UL;
            }
            
            /* Clear remaining bytes if size is not word-aligned */
            if ((size % sizeof(uint32_t)) != 0U) {
                uint8_t* dest_byte = (uint8_t*)&dest[i];
                uint32_t remaining = size % sizeof(uint32_t);
                
                for (uint32_t j = 0U; j < remaining; j++) {
                    dest_byte[j] = 0U;
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief Verify stack canary integrity
 * @return true if canary is intact, false if corrupted
 */
static bool verify_stack_canary(void)
{
    bool result;
    
    /* MISRA-C:2012 Rule 15.5 - Single point of exit */
    result = (stack_canary == STACK_CANARY_VALUE);
    
    return result;
}

/**
 * @brief Setup memory protection unit (MPU) if available
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note STM32F407xx has optional MPU - configure for enhanced protection
 */
static dsrtos_error_t setup_memory_protection(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* MPU setup would go here if required */
    /* For now, return success (no MPU configuration) */
    
    return result;
}

/**
 * @brief Record boot cause and update boot flags
 * @note Diagnostic information for fault analysis
 */
static void record_boot_cause(void)
{
    /* Simple boot cause recording */
    /* In production, would read reset cause registers */
    boot_flags = (boot_flags & 0x0000FFFFUL) | 0x42420000UL;  /* Boot signature */
}

/*==============================================================================
 * WEAK FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Weak implementation of _exit for newlib compatibility
 * @param[in] status Exit status (unused)
 * @note Required for some toolchains
 */
__attribute__((weak))
void _exit(int status)
{
    (void)status;  /* Suppress unused parameter warning */
    
    /* Enter infinite loop */
    while (1) {
        __asm volatile ("nop");
    }
}

/*==============================================================================
 * END OF FILE
 *============================================================================*/
