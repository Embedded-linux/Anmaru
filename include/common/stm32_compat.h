#ifndef STM32_COMPAT_H
#define STM32_COMPAT_H

/* STM32F4xx header should be included separately */

/* NVIC Priority Group definitions if missing */
#ifndef NVIC_PRIORITYGROUP_4
#define NVIC_PRIORITYGROUP_4         (0x00000003U)
#endif

/* SysTick definitions if missing */
#ifndef SysTick_LOAD_RELOAD_Msk
#define SysTick_LOAD_RELOAD_Msk      (0xFFFFFFUL)
#endif

#ifndef SysTick_CTRL_TICKINT_Msk
#define SysTick_CTRL_TICKINT_Msk     (1UL << 1)
#endif

#ifndef SysTick_CTRL_CLKSOURCE_Msk
#define SysTick_CTRL_CLKSOURCE_Msk   (1UL << 2)
#endif

/* RCC bit positions if missing */
#ifndef RCC_CFGR_SWS_Pos
#define RCC_CFGR_SWS_Pos             (2U)
#define RCC_CFGR_HPRE_Pos            (4U)
#define RCC_CFGR_PPRE1_Pos           (10U)
#define RCC_CFGR_PPRE2_Pos           (13U)
#endif

/* PLL bit positions if missing */
#ifndef RCC_PLLCFGR_PLLM_Pos
#define RCC_PLLCFGR_PLLM_Pos         (0U)
#define RCC_PLLCFGR_PLLN_Pos         (6U)
#define RCC_PLLCFGR_PLLP_Pos         (16U)
#define RCC_PLLCFGR_PLLQ_Pos         (24U)
#endif

/* Timer definitions - simplified approach */
#ifndef TIM2_CR1
#define TIM2_CR1    ((volatile uint32_t*)(TIM2_BASE + 0x00))
#define TIM2_DIER   ((volatile uint32_t*)(TIM2_BASE + 0x0C))
#define TIM2_SR     ((volatile uint32_t*)(TIM2_BASE + 0x10))
#define TIM2_CNT    ((volatile uint32_t*)(TIM2_BASE + 0x24))
#endif

#ifndef TIM2_IRQn
#define TIM2_IRQn                    (28)
#endif

#ifndef TIM_DIER_UIE
#define TIM_DIER_UIE                 (1UL << 0)
#endif

#ifndef TIM_SR_UIF
#define TIM_SR_UIF                   (1UL << 0)
#endif

#ifndef TIM_CR1_CEN
#define TIM_CR1_CEN                  (1UL << 0)
#endif

/* Missing peripheral enables */
#ifndef RCC_APB1ENR_TIM2EN
#define RCC_APB1ENR_TIM2EN           (1UL << 0)
#endif

#ifndef RCC_APB2ENR_USART1EN
#define RCC_APB2ENR_USART1EN         (1UL << 4)
#endif

#if 0
/* Timer definitions if missing */
#ifndef TIM2_BASE
#define TIM2_BASE              (0x40000000UL)
#define TIM2                   ((TIM_TypeDef *) TIM2_BASE)
#endif

#ifndef TIM2_IRQn
#define TIM2_IRQn              28
#endif

#ifndef TIM_DIER_UIE
#define TIM_DIER_UIE           (1UL << 0)
#endif

#ifndef TIM_SR_UIF
#define TIM_SR_UIF             (1UL << 0)
#endif

#ifndef TIM_CR1_CEN
#define TIM_CR1_CEN            (1UL << 0)
#endif

/* Timer TypeDef if missing */
#ifndef TIM_TypeDef
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;
#endif
#endif

#endif /* STM32_COMPAT_H */
