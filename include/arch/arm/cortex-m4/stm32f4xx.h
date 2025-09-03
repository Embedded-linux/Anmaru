/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Project
 *
 * File: stm32f4xx.h
 * Description: STM32F4xx MCU peripheral definitions
 * Phase: 1 - Boot & Board Bring-up
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef STM32F4XX_H
#define STM32F4XX_H

#include <stdint.h>
/* IO definitions */
#ifndef __IO
#define __IO    volatile
#endif
#ifndef __I
#define __I     volatile const  
#endif
#ifndef __O
#define __O     volatile
#endif

#include "dsrtos_types.h"

/* System Core Clock */
extern uint32_t SystemCoreClock;

/* In stm32f4xx.h or system_stm32f4xx.h */
void SystemInit(void);
void SystemCoreClockUpdate(void);

/* IO definitions (access restrictions to peripheral registers) */
#ifndef __IO
#define __IO    volatile
#endif

#ifndef __I
#define __I     volatile const
#endif

#ifndef __O
#define __O     volatile
#endif


/* Configuration Constants */
#define HSE_VALUE           8000000U
#define HSE_STARTUP_TIMEOUT 0x05000U
#define HSI_VALUE           16000000U
#define VECT_TAB_OFFSET     0x00U

/* PLL Parameters */
#define PLL_M               8U
#define PLL_N               336U
#define PLL_P               2U
#define PLL_Q               7U

/* Memory mapping */
#define FLASH_BASE          0x08000000UL
#define SRAM1_BASE          0x20000000UL
#define SRAM2_BASE          0x2001C000UL
#define PERIPH_BASE         0x40000000UL
#define BKPSRAM_BASE        0x40024000UL
#define CCMDATARAM_BASE     0x10000000UL

/* Peripheral memory map */
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x10000000UL)

/* AHB1 peripherals */
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE          (AHB1PERIPH_BASE + 0x1000UL)
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x3C00UL)
#define DMA1_BASE           (AHB1PERIPH_BASE + 0x6000UL)
#define DMA2_BASE           (AHB1PERIPH_BASE + 0x6400UL)

/* APB1 peripherals */
#define TIM2_BASE           (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE           (APB1PERIPH_BASE + 0x0400UL)
#define TIM4_BASE           (APB1PERIPH_BASE + 0x0800UL)
#define TIM5_BASE           (APB1PERIPH_BASE + 0x0C00UL)
#define RTC_BASE            (APB1PERIPH_BASE + 0x2800UL)
#define IWDG_BASE           (APB1PERIPH_BASE + 0x3000UL)
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)
#define USART3_BASE         (APB1PERIPH_BASE + 0x4800UL)

/* APB2 peripherals */
#define TIM1_BASE           (APB2PERIPH_BASE + 0x0000UL)
#define USART1_BASE         (APB2PERIPH_BASE + 0x1000UL)
#define USART6_BASE         (APB2PERIPH_BASE + 0x1400UL)
#define ADC1_BASE           (APB2PERIPH_BASE + 0x2000UL)
#define SYSCFG_BASE         (APB2PERIPH_BASE + 0x3800UL)
#define EXTI_BASE           (APB2PERIPH_BASE + 0x3C00UL)

/* Core peripherals */
#define SCS_BASE            (0xE000E000UL)
#define ITM_BASE            (0xE0000000UL)
#define DWT_BASE            (0xE0001000UL)
#define TPI_BASE            (0xE0040000UL)
#define CoreDebug_BASE      (0xE000EDF0UL)
#define SysTick_BASE        (SCS_BASE + 0x0010UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define SCB_BASE            (SCS_BASE + 0x0D00UL)
#define MPU_BASE            (SCS_BASE + 0x0D90UL)

/* Peripheral structure definitions */
typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t PLLCFGR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    __IO uint32_t AHB3RSTR;
    uint32_t      RESERVED0;
    __IO uint32_t APB1RSTR;
    __IO uint32_t APB2RSTR;
    uint32_t      RESERVED1[2];
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t AHB3ENR;
    uint32_t      RESERVED2;
    __IO uint32_t APB1ENR;
    __IO uint32_t APB2ENR;
    uint32_t      RESERVED3[2];
    __IO uint32_t AHB1LPENR;
    __IO uint32_t AHB2LPENR;
    __IO uint32_t AHB3LPENR;
    uint32_t      RESERVED4;
    __IO uint32_t APB1LPENR;
    __IO uint32_t APB2LPENR;
    uint32_t      RESERVED5[2];
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
    uint32_t      RESERVED6[2];
    __IO uint32_t SSCGR;
    __IO uint32_t PLLI2SCFGR;
} RCC_TypeDef;

typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t OPTCR;
    __IO uint32_t OPTCR1;
} FLASH_TypeDef;

typedef struct {
    __IO uint32_t SR;
    __IO uint32_t DR;
    __IO uint32_t BRR;
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t CR3;
    __IO uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    __IO uint32_t KR;
    __IO uint32_t PR;
    __IO uint32_t RLR;
    __IO uint32_t SR;
} IWDG_TypeDef;

typedef struct {
    __IO uint32_t TR;
    __IO uint32_t DR;
    __IO uint32_t CR;
    __IO uint32_t ISR;
    __IO uint32_t PRER;
    __IO uint32_t WUTR;
    __IO uint32_t CALIBR;
    __IO uint32_t ALRMAR;
    __IO uint32_t ALRMBR;
    __IO uint32_t WPR;
    __IO uint32_t SSR;
    __IO uint32_t SHIFTR;
    __IO uint32_t TSTR;
    __IO uint32_t TSDR;
    __IO uint32_t TSSSR;
    __IO uint32_t CALR;
    __IO uint32_t TAFCR;
    __IO uint32_t ALRMASSR;
    __IO uint32_t ALRMBSSR;
    uint32_t RESERVED7;
    __IO uint32_t BKP0R;
    __IO uint32_t BKP1R;
    __IO uint32_t BKP2R;
    __IO uint32_t BKP3R;
    __IO uint32_t BKP4R;
    __IO uint32_t BKP5R;
    __IO uint32_t BKP6R;
    __IO uint32_t BKP7R;
    __IO uint32_t BKP8R;
    __IO uint32_t BKP9R;
    __IO uint32_t BKP10R;
    __IO uint32_t BKP11R;
    __IO uint32_t BKP12R;
    __IO uint32_t BKP13R;
    __IO uint32_t BKP14R;
    __IO uint32_t BKP15R;
    __IO uint32_t BKP16R;
    __IO uint32_t BKP17R;
    __IO uint32_t BKP18R;
    __IO uint32_t BKP19R;
} RTC_TypeDef;

/* Peripheral declarations */
#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *) GPIOE_BASE)
#define RCC                 ((RCC_TypeDef *) RCC_BASE)
#define FLASH               ((FLASH_TypeDef *) FLASH_R_BASE)
#define USART2              ((USART_TypeDef *) USART2_BASE)
#define IWDG                ((IWDG_TypeDef *) IWDG_BASE)
#define RTC                 ((RTC_TypeDef *) RTC_BASE)

/* RCC Bit Definitions */
#define RCC_CR_HSEON        ((uint32_t)0x00010000)
#define RCC_CR_HSERDY       ((uint32_t)0x00020000)
#define RCC_CR_PLLON        ((uint32_t)0x01000000)
#define RCC_CR_PLLRDY       ((uint32_t)0x02000000)

#define RCC_CFGR_SW         ((uint32_t)0x00000003)
#define RCC_CFGR_SW_HSI     ((uint32_t)0x00000000)
#define RCC_CFGR_SW_HSE     ((uint32_t)0x00000001)
#define RCC_CFGR_SW_PLL     ((uint32_t)0x00000002)
#define RCC_CFGR_SWS        ((uint32_t)0x0000000C)
#define RCC_CFGR_SWS_PLL    ((uint32_t)0x00000008)

#define RCC_CFGR_HPRE_DIV1  ((uint32_t)0x00000000)
#define RCC_CFGR_PPRE1_DIV4 ((uint32_t)0x00001400)
#define RCC_CFGR_PPRE2_DIV2 ((uint32_t)0x00008000)

#define RCC_PLLCFGR_PLLSRC_HSE ((uint32_t)0x00400000)
#define RCC_PLLCFGR_PLLM    ((uint32_t)0x0000003F)
#define RCC_PLLCFGR_PLLN    ((uint32_t)0x00007FC0)
#define RCC_PLLCFGR_PLLP    ((uint32_t)0x00030000)
#define RCC_PLLCFGR_PLLQ    ((uint32_t)0x0F000000)

/* RCC Enable bits */
#define RCC_AHB1ENR_GPIOAEN ((uint32_t)0x00000001)
#define RCC_AHB1ENR_GPIOBEN ((uint32_t)0x00000002)
#define RCC_AHB1ENR_GPIOCEN ((uint32_t)0x00000004)
#define RCC_AHB1ENR_GPIODEN ((uint32_t)0x00000008)
#define RCC_AHB1ENR_GPIOEEN ((uint32_t)0x00000010)
#define RCC_AHB1ENR_SRAM1EN ((uint32_t)0x00010000)
#define RCC_AHB1ENR_SRAM2EN ((uint32_t)0x00020000)
#define RCC_AHB1ENR_CCMDATARAMEN ((uint32_t)0x00100000)

#define RCC_APB1ENR_USART2EN ((uint32_t)0x00020000)

/* RCC Reset flags */
#define RCC_CSR_LPWRRSTF    ((uint32_t)0x80000000)
#define RCC_CSR_WWDGRSTF    ((uint32_t)0x40000000)
#define RCC_CSR_IWDGRSTF    ((uint32_t)0x20000000)
#define RCC_CSR_SFTRSTF     ((uint32_t)0x10000000)
#define RCC_CSR_PORRSTF     ((uint32_t)0x08000000)
#define RCC_CSR_PINRSTF     ((uint32_t)0x04000000)
#define RCC_CSR_RMVF        ((uint32_t)0x01000000)

/* Flash Access Control Register bits */
#define FLASH_ACR_LATENCY   ((uint32_t)0x00000007)
#define FLASH_ACR_LATENCY_5WS ((uint32_t)0x00000005)
#define FLASH_ACR_PRFTEN    ((uint32_t)0x00000100)
#define FLASH_ACR_ICEN      ((uint32_t)0x00000200)
#define FLASH_ACR_DCEN      ((uint32_t)0x00000400)

/* Flash Control Register bits */
#define FLASH_CR_LOCK       ((uint32_t)0x80000000)

/* USART Control Register bits */
#define USART_CR1_UE        ((uint32_t)0x00002000)
#define USART_CR1_TE        ((uint32_t)0x00000008)
#define USART_CR1_RE        ((uint32_t)0x00000004)

/* Include CMSIS Core */
#include "core_cm4.h"

/* System Core Clock */
extern uint32_t SystemCoreClock;

/* Function prototypes */
void SystemInit(void);
void SystemCoreClockUpdate(void);

#endif /* STM32F4XX_H */
extern uint32_t SystemCoreClock;

/* Additional RCC bit definitions */
#define RCC_PLLCFGR_PLLSRC  ((uint32_t)0x00400000)
#define RCC_CFGR_HPRE       ((uint32_t)0x000000F0)
#define RCC_CFGR_PPRE1      ((uint32_t)0x00001C00)
#define RCC_CFGR_PPRE2      ((uint32_t)0x0000E000)

/* Common definitions */
#define RESET               0U
#define SET                 1U

/* Flash ACR additional bits */
#define FLASH_ACR_PRFTEN    ((uint32_t)0x00000100)
#define FLASH_ACR_ICEN      ((uint32_t)0x00000200)
#define FLASH_ACR_DCEN      ((uint32_t)0x00000400)
#define FLASH_ACR_LATENCY   ((uint32_t)0x00000007)
#define FLASH_ACR_LATENCY_5WS ((uint32_t)0x00000005)

/* Additional peripheral clock enable bits */
#define RCC_APB2ENR_SYSCFGEN ((uint32_t)0x00004000)

/* SYSCFG registers */
typedef struct {
    __IO uint32_t MEMRMP;
    __IO uint32_t PMC;
    __IO uint32_t EXTICR[4];
    uint32_t RESERVED[2];
    __IO uint32_t CMPCR;
} SYSCFG_TypeDef;

#define SYSCFG_BASE         (APB2PERIPH_BASE + 0x3800UL)
#define SYSCFG              ((SYSCFG_TypeDef *) SYSCFG_BASE)

/* EXTI registers */
typedef struct {
    __IO uint32_t IMR;
    __IO uint32_t EMR;
    __IO uint32_t RTSR;
    __IO uint32_t FTSR;
    __IO uint32_t SWIER;
    __IO uint32_t PR;
} EXTI_TypeDef;

#define EXTI_BASE           (APB2PERIPH_BASE + 0x3C00UL)
#define EXTI                ((EXTI_TypeDef *) EXTI_BASE)


/* USART peripherals */
#define USART1_BASE         (APB2PERIPH_BASE + 0x1000UL)
#define USART3_BASE         (APB1PERIPH_BASE + 0x4800UL)
#define UART4_BASE          (APB1PERIPH_BASE + 0x4C00UL)
#define UART5_BASE          (APB1PERIPH_BASE + 0x5000UL)
#define USART6_BASE         (APB2PERIPH_BASE + 0x1400UL)

#define USART1              ((USART_TypeDef *) USART1_BASE)
#define USART3              ((USART_TypeDef *) USART3_BASE)
#define UART4               ((USART_TypeDef *) UART4_BASE)
#define UART5               ((USART_TypeDef *) UART5_BASE)
#define USART6              ((USART_TypeDef *) USART6_BASE)

/* RCC USART enable bits */
#define RCC_APB2ENR_USART1EN ((uint32_t)0x00000010)
#define RCC_APB2ENR_USART6EN ((uint32_t)0x00000020)
#define RCC_APB1ENR_USART3EN ((uint32_t)0x00040000)
#define RCC_APB1ENR_UART4EN  ((uint32_t)0x00080000)
#define RCC_APB1ENR_UART5EN  ((uint32_t)0x00100000)
