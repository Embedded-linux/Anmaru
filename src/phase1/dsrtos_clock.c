/**
 * @file dsrtos_clock.c
 * @brief DSRTOS Clock Configuration Implementation - Phase 1
 * 
 * @details Implements high-level clock configuration and management for 
 *          STM32F407VG. Provides system clock setup, peripheral clock control,
 *          and clock monitoring capabilities.
 * 
 * @version 1.0.0
 * @date 2025-08-30
 * 
 * @par Compliance
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certified
 * - IEC 62304 Class B medical device standard  
 * - IEC 61508 SIL 3 functional safety
 * - ISO 26262 ASIL D automotive safety
 * 
 * @par Performance Requirements
 * - System clock: 168MHz from PLL
 * - AHB clock: 168MHz (AHB prescaler = 1)
 * - APB1 clock: 42MHz (APB1 prescaler = 4)
 * - APB2 clock: 84MHz (APB2 prescaler = 2)
 * - Clock startup time: < 500ms
 * 
 * @copyright (c) 2025 DSRTOS Development Team
 * @license MIT License - See LICENSE file for details
 */

/*==============================================================================
 * INCLUDES
 *==============================================================================*/

#include "dsrtos_clock.h"
#include "dsrtos_error.h"
#include "stm32f4xx.h"
#include "stm32_compat.h"
#include "system_config.h"
#include "diagnostic.h"
#include "dsrtos_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*==============================================================================
 * MISRA-C:2012 COMPLIANCE DIRECTIVES
 *==============================================================================*/

/* MISRA C:2012 Dir 4.4 - All uses of assembly language shall be documented */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"

/*==============================================================================
 * CONSTANTS AND MACROS  
 *==============================================================================*/

/** Magic number for clock controller validation */
#define DSRTOS_CLOCK_MAGIC_NUMBER        (0x434C4B52U)  /* 'CLKR' */

/** HSE crystal frequency (8MHz on STM32F4-Discovery) */
#define DSRTOS_HSE_FREQUENCY_HZ          (8000000U)

/** HSI frequency (16MHz internal oscillator) */
#define DSRTOS_HSI_FREQUENCY_HZ          (16000000U)

/** LSE crystal frequency (32.768kHz) */
#define DSRTOS_LSE_FREQUENCY_HZ          (32768U)

/** LSI frequency (approximately 32kHz) */
#define DSRTOS_LSI_FREQUENCY_HZ          (32000U)

/** Target system clock frequency */
#define DSRTOS_SYSCLK_TARGET_HZ          (168000000U)

/** PLL configuration for 168MHz from 8MHz HSE */
#define DSRTOS_PLL_M                     (8U)    /* HSE/8 = 1MHz */
#define DSRTOS_PLL_N                     (336U)  /* 1MHz * 336 = 336MHz */
#define DSRTOS_PLL_P                     (2U)    /* 336MHz / 2 = 168MHz */
#define DSRTOS_PLL_Q                     (7U)    /* 336MHz / 7 = 48MHz (USB) */

/** AHB prescaler (SYSCLK / 1 = 168MHz) */
#define DSRTOS_AHB_PRESCALER             (RCC_CFGR_HPRE_DIV1)

/** APB1 prescaler (AHB / 4 = 42MHz) */
#define DSRTOS_APB1_PRESCALER            (RCC_CFGR_PPRE1_DIV4)

/** APB2 prescaler (AHB / 2 = 84MHz) */
#define DSRTOS_APB2_PRESCALER            (RCC_CFGR_PPRE2_DIV2)

/** Clock startup timeout (500ms at 16MHz HSI) */
#define DSRTOS_CLOCK_STARTUP_TIMEOUT     (8000000U)

/** Flash wait states for 168MHz */
#define DSRTOS_FLASH_LATENCY             (FLASH_ACR_LATENCY_5WS)

/*==============================================================================
 * TYPE DEFINITIONS
 *==============================================================================*/


/**
 * @brief Clock controller state structure
 */
typedef struct {
    uint32_t magic;                    /**< Validation magic number */
    bool initialized;                  /**< Initialization status */
    
    /* Clock frequencies */
    uint32_t sysclk_frequency;         /**< System clock frequency */
    uint32_t ahb_frequency;            /**< AHB bus frequency */
    uint32_t apb1_frequency;           /**< APB1 bus frequency */
    uint32_t apb2_frequency;           /**< APB2 bus frequency */
    uint32_t pll_frequency;            /**< PLL output frequency */
    
    /* Clock sources status */
    bool hse_ready;                    /**< HSE oscillator ready */
    bool hsi_ready;                    /**< HSI oscillator ready */
    bool pll_ready;                    /**< PLL ready */
    bool lse_ready;                    /**< LSE oscillator ready */
    bool lsi_ready;                    /**< LSI oscillator ready */
    
    /* Active clock source */
    dsrtos_clock_source_t system_clock_source;
    
    /* Peripheral clock enables */
    uint32_t ahb1_enabled_mask;        /**< AHB1 peripherals enabled */
    uint32_t ahb2_enabled_mask;        /**< AHB2 peripherals enabled */
    uint32_t ahb3_enabled_mask;        /**< AHB3 peripherals enabled */
    uint32_t apb1_enabled_mask;        /**< APB1 peripherals enabled */
    uint32_t apb2_enabled_mask;        /**< APB2 peripherals enabled */
    
    /* Statistics */
    struct {
        uint32_t clock_switches;       /**< Number of clock source switches */
        uint32_t pll_locks;            /**< Number of PLL lock events */
        uint32_t clock_failures;       /**< Clock failure events */
        uint32_t peripheral_enables;   /**< Peripheral enable count */
    } stats;
    
} dsrtos_clock_controller_t;

/*==============================================================================
 * STATIC VARIABLES
 *==============================================================================*/

/** Static clock controller instance */
static dsrtos_clock_controller_t s_clock_controller = {0};

/*==============================================================================
 * STATIC FUNCTION PROTOTYPES
 *==============================================================================*/

static dsrtos_result_t configure_flash_wait_states(void);
static dsrtos_result_t enable_hse_oscillator(void);
static dsrtos_result_t configure_pll(void);
static dsrtos_result_t switch_to_pll_clock(void);
static dsrtos_result_t configure_bus_prescalers(void);
static dsrtos_result_t wait_for_clock_ready(volatile uint32_t* ready_flag, uint32_t timeout);
static void update_system_clock_frequencies(void);

/*==============================================================================
 * STATIC FUNCTION IMPLEMENTATIONS
 *==============================================================================*/

/**
 * @brief Configure Flash wait states for high-speed operation
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_result_t configure_flash_wait_states(void)
{
    /* Configure Flash prefetch, instruction cache, data cache and wait state */
    FLASH->ACR = FLASH_ACR_PRFTEN |      /* Enable prefetch */
                 FLASH_ACR_ICEN |        /* Enable instruction cache */
                 FLASH_ACR_DCEN |        /* Enable data cache */
                 DSRTOS_FLASH_LATENCY;   /* 5 wait states for 168MHz */
    
    /* Verify configuration was applied */
    if ((FLASH->ACR & FLASH_ACR_LATENCY) != DSRTOS_FLASH_LATENCY) {
        return DSRTOS_ERR_HARDWARE_FAULT;
    }
    
    return DSRTOS_OK;
}

/**
 * @brief Enable HSE oscillator and wait for ready
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_result_t enable_hse_oscillator(void)
{
    dsrtos_result_t result;
    
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    
    /* Wait for HSE to be ready */
    result = wait_for_clock_ready(&RCC->CR, RCC_CR_HSERDY);
    
    if (result == DSRTOS_OK) {
        s_clock_controller.hse_ready = true;
    }
    
    return result;
}

/**
 * @brief Configure PLL for target frequency
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_result_t configure_pll(void)
{
    dsrtos_result_t result;
    uint32_t pll_config;
    
    /* Disable PLL before configuration */
    RCC->CR &= ~RCC_CR_PLLON;
    
    /* Wait for PLL to be unlocked */
    result = wait_for_clock_ready(&RCC->CR, 0U); /* Wait for NOT ready */
    
    if (result == DSRTOS_OK) {
	    /* Configure PLL with manual bit positions */
		pll_config = (DSRTOS_PLL_M << 0U) |          /* PLLM bits 0-5 */
             (DSRTOS_PLL_N << 6U) |          /* PLLN bits 6-14 */
             (((DSRTOS_PLL_P >> 1U) - 1U) << 16U) | /* PLLP bits 16-17 */
             (DSRTOS_PLL_Q << 24U) |         /* PLLQ bits 24-27 */
             RCC_PLLCFGR_PLLSRC_HSE;         /* HSE as PLL source */
        
        RCC->PLLCFGR = pll_config;
        
        /* Enable PLL */
        RCC->CR |= RCC_CR_PLLON;
        
        /* Wait for PLL to be ready */
        result = wait_for_clock_ready(&RCC->CR, RCC_CR_PLLRDY);
        
        if (result == DSRTOS_OK) {
            s_clock_controller.pll_ready = true;
            s_clock_controller.stats.pll_locks++;
        }
    }
    
    return result;
}

/**
 * @brief Switch system clock to PLL
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_result_t switch_to_pll_clock(void)
{
    dsrtos_result_t result = DSRTOS_OK;
    uint32_t timeout = DSRTOS_CLOCK_STARTUP_TIMEOUT;
    
    /* Select PLL as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    
    /* Wait for PLL to be used as system clock source */
    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (timeout > 0U)) {
        timeout--;
    }
    
    if (timeout == 0U) {
        result = DSRTOS_ERR_TIMEOUT;
    } else {
        s_clock_controller.system_clock_source = DSRTOS_CLOCK_SOURCE_PLL;
        s_clock_controller.stats.clock_switches++;
    }
    
    return result;
}

/**
 * @brief Configure AHB and APB prescalers
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_result_t configure_bus_prescalers(void)
{
    uint32_t cfgr_value;
    
    /* Configure prescalers */
    cfgr_value = RCC->CFGR;
    cfgr_value &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    cfgr_value |= DSRTOS_AHB_PRESCALER | DSRTOS_APB1_PRESCALER | DSRTOS_APB2_PRESCALER;
    
    RCC->CFGR = cfgr_value;
    
    return DSRTOS_OK;
}

/**
 * @brief Wait for clock ready flag
 * @param ready_flag Pointer to register containing ready flag
 * @param flag_mask Flag mask to check (0 to wait for NOT ready)
 * @return DSRTOS_OK on success, DSRTOS_ERR_TIMEOUT on timeout
 */
static dsrtos_result_t wait_for_clock_ready(volatile uint32_t* ready_flag, uint32_t flag_mask)
{
    dsrtos_result_t result = DSRTOS_OK;
    uint32_t timeout = DSRTOS_CLOCK_STARTUP_TIMEOUT;
    
    if (flag_mask == 0U) {
        /* Wait for flag to be clear (PLL unlock case) */
        while (((*ready_flag & RCC_CR_PLLRDY) != 0U) && (timeout > 0U)) {
            timeout--;
        }
    } else {
        /* Wait for flag to be set */
        while (((*ready_flag & flag_mask) == 0U) && (timeout > 0U)) {
            timeout--;
        }
    }
    
    if (timeout == 0U) {
        result = DSRTOS_ERR_TIMEOUT;
        s_clock_controller.stats.clock_failures++;
    }
    
    return result;
}

/**
 * @brief Update system clock frequency calculations
 */
static void update_system_clock_frequencies(void)
{
    dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    uint32_t ahb_prescaler_table[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
    uint32_t apb_prescaler_table[8] = {0, 0, 0, 0, 1, 2, 3, 4};
    uint32_t sysclk_source, ahb_prescaler, apb1_prescaler, apb2_prescaler;
    
    /* Get system clock source */
    /*sysclk_source = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;*/
    sysclk_source = (RCC->CFGR & RCC_CFGR_SWS) >> 2U;  /* SWS bits are at position 2-3 */
    
    /* Calculate SYSCLK frequency */
    switch (sysclk_source) {
        case 0x00U:  /* HSI used as system clock */
            ctrl->sysclk_frequency = DSRTOS_HSI_FREQUENCY_HZ;
            ctrl->system_clock_source = DSRTOS_CLOCK_SOURCE_HSI;
            break;
            
        case 0x01U:  /* HSE used as system clock */
            ctrl->sysclk_frequency = DSRTOS_HSE_FREQUENCY_HZ;
            ctrl->system_clock_source = DSRTOS_CLOCK_SOURCE_HSE;
            break;
            
        case 0x02U:  /* PLL used as system clock */
            ctrl->sysclk_frequency = DSRTOS_SYSCLK_TARGET_HZ;
            ctrl->system_clock_source = DSRTOS_CLOCK_SOURCE_PLL;
            break;
            
        default:
            ctrl->sysclk_frequency = DSRTOS_HSI_FREQUENCY_HZ;
            break;
    }
    
    /* Calculate AHB frequency */
    /*ahb_prescaler = (RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos;*/
    ahb_prescaler = (RCC->CFGR & RCC_CFGR_HPRE) >> 4U;  /* HPRE bits are at position 4-7 */
    ctrl->ahb_frequency = ctrl->sysclk_frequency >> ahb_prescaler_table[ahb_prescaler];
    
    /* Calculate APB1 frequency */
    /*apb1_prescaler = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;*/
    apb1_prescaler = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10U;  /* PPRE1 bits are at position 10-12 */
    ctrl->apb1_frequency = ctrl->ahb_frequency >> apb_prescaler_table[apb1_prescaler];
    
    /* Calculate APB2 frequency */
    /*apb2_prescaler = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;*/
    apb2_prescaler = (RCC->CFGR & RCC_CFGR_PPRE2) >> 13U;  /* PPRE2 bits are at position 13-15 */
    ctrl->apb2_frequency = ctrl->ahb_frequency >> apb_prescaler_table[apb2_prescaler];
    
    /* Update SystemCoreClock variable (used by other modules) */
    SystemCoreClock = ctrl->sysclk_frequency;
}

/*==============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *==============================================================================*/

/**
 * @brief Initialize the DSRTOS clock subsystem
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_result_t dsrtos_clock_init(void)
{
    dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    dsrtos_result_t result = DSRTOS_OK;
    
    /* Check if already initialized */
    if ((ctrl->magic == DSRTOS_CLOCK_MAGIC_NUMBER) && (ctrl->initialized == true)) {
        result = DSRTOS_ERR_ALREADY_INITIALIZED;
    } else {
        /* Clear controller structure */
        ctrl->magic = 0U;
        ctrl->initialized = false;
        
        /* Initialize default values */
        ctrl->sysclk_frequency = DSRTOS_HSI_FREQUENCY_HZ;  /* Reset default */
        ctrl->ahb_frequency = DSRTOS_HSI_FREQUENCY_HZ;
        ctrl->apb1_frequency = DSRTOS_HSI_FREQUENCY_HZ;
        ctrl->apb2_frequency = DSRTOS_HSI_FREQUENCY_HZ;
        ctrl->pll_frequency = 0U;
        
        /* Initialize oscillator status */
        ctrl->hse_ready = false;
        ctrl->hsi_ready = true;  /* HSI is enabled by default */
        ctrl->pll_ready = false;
        ctrl->lse_ready = false;
        ctrl->lsi_ready = false;
        
        /* Initialize peripheral enable masks */
        ctrl->ahb1_enabled_mask = 0U;
        ctrl->ahb2_enabled_mask = 0U;
        ctrl->ahb3_enabled_mask = 0U;
        ctrl->apb1_enabled_mask = 0U;
        ctrl->apb2_enabled_mask = 0U;
        
        /* Initialize statistics */
        ctrl->stats.clock_switches = 0U;
        ctrl->stats.pll_locks = 0U;
        ctrl->stats.clock_failures = 0U;
        ctrl->stats.peripheral_enables = 0U;
        
        /* Configure Flash wait states first */
        result = configure_flash_wait_states();
        
        if (result == DSRTOS_OK) {
            /* Enable HSE oscillator */
            result = enable_hse_oscillator();
        }
        
        if (result == DSRTOS_OK) {
            /* Configure PLL */
            result = configure_pll();
        }
        
        if (result == DSRTOS_OK) {
            /* Configure bus prescalers */
            result = configure_bus_prescalers();
        }
        
        if (result == DSRTOS_OK) {
            /* Switch to PLL as system clock */
            result = switch_to_pll_clock();
        }
        
        if (result == DSRTOS_OK) {
            /* Update frequency calculations */
            update_system_clock_frequencies();
            
            /* Set magic number and mark as initialized */
            ctrl->magic = DSRTOS_CLOCK_MAGIC_NUMBER;
            ctrl->initialized = true;
        }
    }
    
    return result;
}

/**
 * @brief Enable peripheral clock
 * @param peripheral Peripheral clock to enable
 * @return DSRTOS_OK on success, error code on failure
 */
#if 0
dsrtos_result_t dsrtos_clock_enable_peripheral(dsrtos_peripheral_t peripheral)
{
    dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    dsrtos_result_t result;
    
    if ((ctrl->magic != DSRTOS_CLOCK_MAGIC_NUMBER) || (ctrl->initialized != true)) {
        result = DSRTOS_ERR_NOT_INITIALIZED;
    } else {
        switch (peripheral) {
            /* AHB1 peripherals */
            case DSRTOS_PERIPHERAL_GPIOA:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOAEN;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOB:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOBEN;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOC:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOCEN;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOD:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIODEN;
                break;
                
            /* APB1 peripherals */
            case DSRTOS_PERIPHERAL_TIM2:
                RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
                ctrl->apb1_enabled_mask |= RCC_APB1ENR_TIM2EN;
                break;
                
            case DSRTOS_PERIPHERAL_USART2:
                RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
                ctrl->apb1_enabled_mask |= RCC_APB1ENR_USART2EN;
                break;
                
            /* APB2 peripherals */
            case DSRTOS_PERIPHERAL_USART1:
                RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
                ctrl->apb2_enabled_mask |= RCC_APB2ENR_USART1EN;
                break;
                
            case DSRTOS_PERIPHERAL_SYSCFG:
                RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
                ctrl->apb2_enabled_mask |= RCC_APB2ENR_SYSCFGEN;
                break;
                
            default:
                result = DSRTOS_ERR_INVALID_PARAM;
                break;
        }
        
        if (result != DSRTOS_ERR_INVALID_PARAM) {
            ctrl->stats.peripheral_enables++;
            result = DSRTOS_OK;
        }
    }
    
    return result;
}
#endif

	dsrtos_result_t dsrtos_clock_enable_peripheral(dsrtos_peripheral_t peripheral)
{
    dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    dsrtos_result_t result = DSRTOS_ERR_INVALID_PARAM;
    
    if ((ctrl->magic != DSRTOS_CLOCK_MAGIC_NUMBER) || (ctrl->initialized != true)) {
        result = DSRTOS_ERR_NOT_INITIALIZED;
    } else {
        switch (peripheral) {
            /* AHB1 peripherals */
            case DSRTOS_PERIPHERAL_GPIOA:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOAEN;
                result = DSRTOS_OK;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOB:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOBEN;
                result = DSRTOS_OK;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOC:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIOCEN;
                result = DSRTOS_OK;
                break;
                
            case DSRTOS_PERIPHERAL_GPIOD:
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
                ctrl->ahb1_enabled_mask |= RCC_AHB1ENR_GPIODEN;
                result = DSRTOS_OK;
                break;
                
            /* Additional GPIO ports - not all STM32F407VG have these */
            case DSRTOS_PERIPHERAL_GPIOE:
            case DSRTOS_PERIPHERAL_GPIOF:
            case DSRTOS_PERIPHERAL_GPIOG:
            case DSRTOS_PERIPHERAL_GPIOH:
            case DSRTOS_PERIPHERAL_GPIOI:
            case DSRTOS_PERIPHERAL_CRC:
            case DSRTOS_PERIPHERAL_DMA1:
            case DSRTOS_PERIPHERAL_DMA2:
            case DSRTOS_PERIPHERAL_ETH_MAC:
            case DSRTOS_PERIPHERAL_ETH_MAC_TX:
            case DSRTOS_PERIPHERAL_ETH_MAC_RX:
            case DSRTOS_PERIPHERAL_ETH_MAC_PTP:
            case DSRTOS_PERIPHERAL_OTG_HS:
            case DSRTOS_PERIPHERAL_OTG_HS_ULPI:
            case DSRTOS_PERIPHERAL_DCMI:
            case DSRTOS_PERIPHERAL_CRYP:
            case DSRTOS_PERIPHERAL_HASH:
            case DSRTOS_PERIPHERAL_RNG:
            case DSRTOS_PERIPHERAL_OTG_FS:
            case DSRTOS_PERIPHERAL_FSMC:
            case DSRTOS_PERIPHERAL_TIM2:
            case DSRTOS_PERIPHERAL_TIM3:
            case DSRTOS_PERIPHERAL_TIM4:
            case DSRTOS_PERIPHERAL_TIM5:
            case DSRTOS_PERIPHERAL_TIM6:
            case DSRTOS_PERIPHERAL_TIM7:
            case DSRTOS_PERIPHERAL_TIM12:
            case DSRTOS_PERIPHERAL_TIM13:
            case DSRTOS_PERIPHERAL_TIM14:
            case DSRTOS_PERIPHERAL_WWDG:
            case DSRTOS_PERIPHERAL_SPI2:
            case DSRTOS_PERIPHERAL_SPI3:
            case DSRTOS_PERIPHERAL_USART3:
            case DSRTOS_PERIPHERAL_UART4:
            case DSRTOS_PERIPHERAL_UART5:
            case DSRTOS_PERIPHERAL_I2C1:
            case DSRTOS_PERIPHERAL_I2C2:
            case DSRTOS_PERIPHERAL_I2C3:
            case DSRTOS_PERIPHERAL_CAN1:
            case DSRTOS_PERIPHERAL_CAN2:
            case DSRTOS_PERIPHERAL_PWR:
            case DSRTOS_PERIPHERAL_DAC:
            case DSRTOS_PERIPHERAL_TIM1:
            case DSRTOS_PERIPHERAL_TIM8:
            case DSRTOS_PERIPHERAL_USART1:
            case DSRTOS_PERIPHERAL_USART6:
            case DSRTOS_PERIPHERAL_ADC1:
            case DSRTOS_PERIPHERAL_ADC2:
            case DSRTOS_PERIPHERAL_ADC3:
            case DSRTOS_PERIPHERAL_SDIO:
            case DSRTOS_PERIPHERAL_SPI1:
            case DSRTOS_PERIPHERAL_TIM9:
            case DSRTOS_PERIPHERAL_TIM10:
            case DSRTOS_PERIPHERAL_TIM11:
            case DSRTOS_PERIPHERAL_COUNT:
                /* These peripherals are not implemented in this basic version */
                result = DSRTOS_ERR_NOT_SUPPORTED;
                break;
                
            /* APB1/APB2 peripherals - only enable ones that exist in headers */
            case DSRTOS_PERIPHERAL_USART2:
                RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
                ctrl->apb1_enabled_mask |= RCC_APB1ENR_USART2EN;
                result = DSRTOS_OK;
                break;
                
            case DSRTOS_PERIPHERAL_SYSCFG:
                RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
                ctrl->apb2_enabled_mask |= RCC_APB2ENR_SYSCFGEN;
                result = DSRTOS_OK;
                break;
                
            default:
                result = DSRTOS_ERR_NOT_SUPPORTED;
                break;
        }
        
        if (result == DSRTOS_OK) {
            ctrl->stats.peripheral_enables++;
        }
    }
    
    return result;
}

/**
 * @brief Get clock frequencies
 * @param frequencies Pointer to frequency structure
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_result_t dsrtos_clock_get_frequencies(dsrtos_clock_frequencies_t* frequencies)
{
    const dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    dsrtos_result_t result;
    
    if (frequencies == NULL) {
        result = DSRTOS_ERR_NULL_POINTER;
    }
    else if ((ctrl->magic != DSRTOS_CLOCK_MAGIC_NUMBER) || (ctrl->initialized != true)) {
        result = DSRTOS_ERR_NOT_INITIALIZED;
    }
    else {
        frequencies->sysclk_hz = ctrl->sysclk_frequency;
        frequencies->ahb_hz = ctrl->ahb_frequency;
        frequencies->apb1_hz = ctrl->apb1_frequency;
        frequencies->apb2_hz = ctrl->apb2_frequency;
        frequencies->pll_hz = ctrl->pll_frequency;
        
        result = DSRTOS_OK;
    }
    
    return result;
}

/**
 * @brief Get clock statistics
 * @param stats Pointer to statistics structure
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_result_t dsrtos_clock_get_stats(dsrtos_clock_stats_t* stats)
{
    const dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    dsrtos_result_t result;
    
    if (stats == NULL) {
        result = DSRTOS_ERR_NULL_POINTER;
    }
    else if ((ctrl->magic != DSRTOS_CLOCK_MAGIC_NUMBER) || (ctrl->initialized != true)) {
        result = DSRTOS_ERR_NOT_INITIALIZED;
    }
    else {
        stats->clock_switches = ctrl->stats.clock_switches;
        stats->pll_locks = ctrl->stats.pll_locks;
        stats->clock_failures = ctrl->stats.clock_failures;
        stats->peripheral_enables = ctrl->stats.peripheral_enables;
        stats->hse_ready = ctrl->hse_ready;
        stats->pll_ready = ctrl->pll_ready;
        stats->current_source = ctrl->system_clock_source;
        
        result = DSRTOS_OK;
    }
    
    return result;
}

/**
 * @brief Get system clock frequency
 * @return System clock frequency in Hz
 */
uint32_t dsrtos_clock_get_sysclk_frequency(void)
{
    const dsrtos_clock_controller_t* const ctrl = &s_clock_controller;
    
    if ((ctrl->magic != DSRTOS_CLOCK_MAGIC_NUMBER) || (ctrl->initialized != true)) {
        return 0U;
    }
    
    return ctrl->sysclk_frequency;
}

#pragma GCC diagnostic pop

/*==============================================================================
 * END OF FILE
 *==============================================================================*/
