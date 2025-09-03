/**
 * @file dsrtos_clock.h
 * @brief DSRTOS Clock Configuration Interface - Phase 1
 * 
 * @details Provides high-level clock configuration and management interface for
 *          STM32F407VG platform. Handles system clock setup, peripheral clocks,
 *          and clock monitoring.
 * 
 * @version 1.0.0
 * @date 2025-08-30
 * 
 * @par Features
 * - System clock configuration (168MHz from PLL)
 * - Peripheral clock management
 * - Clock source monitoring
 * - Frequency calculation and reporting
 * - Clock failure detection
 * 
 * @par Compliance
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certified
 * - IEC 62304 Class B medical device standard  
 * - IEC 61508 SIL 3 functional safety
 * - ISO 26262 ASIL D automotive safety
 * 
 * @copyright (c) 2025 DSRTOS Development Team
 * @license MIT License - See LICENSE file for details
 */

#ifndef DSRTOS_CLOCK_H
#define DSRTOS_CLOCK_H

/*==============================================================================
 * INCLUDES
 *==============================================================================*/

#include "dsrtos_types.h"
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * COMPILER ATTRIBUTES FOR CERTIFICATION
 *==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * CONSTANTS AND MACROS
 *==============================================================================*/

/** @defgroup DSRTOS_Clock_Constants Clock Constants
 * @{
 */

/** Target system clock frequency in Hz */
#define DSRTOS_SYSCLK_FREQUENCY_HZ           (168000000U)

/** AHB bus frequency in Hz */
#define DSRTOS_AHB_FREQUENCY_HZ              (168000000U)

/** APB1 bus frequency in Hz */
#define DSRTOS_APB1_FREQUENCY_HZ             (42000000U)

/** APB2 bus frequency in Hz */
#define DSRTOS_APB2_FREQUENCY_HZ             (84000000U)

/** HSE crystal frequency in Hz */
#define DSRTOS_HSE_CRYSTAL_HZ                (8000000U)

/** LSE crystal frequency in Hz */
#define DSRTOS_LSE_CRYSTAL_HZ                (32768U)

/** @} */

/*==============================================================================
 * TYPE DEFINITIONS
 *==============================================================================*/

/**
 * @brief Peripheral clock enumeration
 */
typedef enum {
    /* AHB1 peripherals */
    DSRTOS_PERIPHERAL_GPIOA = 0,           /**< GPIO Port A */
    DSRTOS_PERIPHERAL_GPIOB,               /**< GPIO Port B */
    DSRTOS_PERIPHERAL_GPIOC,               /**< GPIO Port C */
    DSRTOS_PERIPHERAL_GPIOD,               /**< GPIO Port D */
    DSRTOS_PERIPHERAL_GPIOE,               /**< GPIO Port E */
    DSRTOS_PERIPHERAL_GPIOF,               /**< GPIO Port F */
    DSRTOS_PERIPHERAL_GPIOG,               /**< GPIO Port G */
    DSRTOS_PERIPHERAL_GPIOH,               /**< GPIO Port H */
    DSRTOS_PERIPHERAL_GPIOI,               /**< GPIO Port I */
    DSRTOS_PERIPHERAL_CRC,                 /**< CRC calculation unit */
    DSRTOS_PERIPHERAL_DMA1,                /**< DMA1 controller */
    DSRTOS_PERIPHERAL_DMA2,                /**< DMA2 controller */
    DSRTOS_PERIPHERAL_ETH_MAC,             /**< Ethernet MAC */
    DSRTOS_PERIPHERAL_ETH_MAC_TX,          /**< Ethernet MAC TX */
    DSRTOS_PERIPHERAL_ETH_MAC_RX,          /**< Ethernet MAC RX */
    DSRTOS_PERIPHERAL_ETH_MAC_PTP,         /**< Ethernet MAC PTP */
    DSRTOS_PERIPHERAL_OTG_HS,              /**< USB OTG HS */
    DSRTOS_PERIPHERAL_OTG_HS_ULPI,         /**< USB OTG HS ULPI */
    
    /* AHB2 peripherals */
    DSRTOS_PERIPHERAL_DCMI,                /**< Digital camera interface */
    DSRTOS_PERIPHERAL_CRYP,                /**< Cryptographic processor */
    DSRTOS_PERIPHERAL_HASH,                /**< Hash processor */
    DSRTOS_PERIPHERAL_RNG,                 /**< Random number generator */
    DSRTOS_PERIPHERAL_OTG_FS,              /**< USB OTG FS */
    
    /* AHB3 peripherals */
    DSRTOS_PERIPHERAL_FSMC,                /**< Flexible static memory controller */
    
    /* APB1 peripherals */
    DSRTOS_PERIPHERAL_TIM2,                /**< Timer 2 */
    DSRTOS_PERIPHERAL_TIM3,                /**< Timer 3 */
    DSRTOS_PERIPHERAL_TIM4,                /**< Timer 4 */
    DSRTOS_PERIPHERAL_TIM5,                /**< Timer 5 */
    DSRTOS_PERIPHERAL_TIM6,                /**< Timer 6 */
    DSRTOS_PERIPHERAL_TIM7,                /**< Timer 7 */
    DSRTOS_PERIPHERAL_TIM12,               /**< Timer 12 */
    DSRTOS_PERIPHERAL_TIM13,               /**< Timer 13 */
    DSRTOS_PERIPHERAL_TIM14,               /**< Timer 14 */
    DSRTOS_PERIPHERAL_WWDG,                /**< Window watchdog */
    DSRTOS_PERIPHERAL_SPI2,                /**< SPI2 */
    DSRTOS_PERIPHERAL_SPI3,                /**< SPI3 */
    DSRTOS_PERIPHERAL_USART2,              /**< USART2 */
    DSRTOS_PERIPHERAL_USART3,              /**< USART3 */
    DSRTOS_PERIPHERAL_UART4,               /**< UART4 */
    DSRTOS_PERIPHERAL_UART5,               /**< UART5 */
    DSRTOS_PERIPHERAL_I2C1,                /**< I2C1 */
    DSRTOS_PERIPHERAL_I2C2,                /**< I2C2 */
    DSRTOS_PERIPHERAL_I2C3,                /**< I2C3 */
    DSRTOS_PERIPHERAL_CAN1,                /**< CAN1 */
    DSRTOS_PERIPHERAL_CAN2,                /**< CAN2 */
    DSRTOS_PERIPHERAL_PWR,                 /**< Power interface */
    DSRTOS_PERIPHERAL_DAC,                 /**< DAC */
    
    /* APB2 peripherals */
    DSRTOS_PERIPHERAL_TIM1,                /**< Timer 1 */
    DSRTOS_PERIPHERAL_TIM8,                /**< Timer 8 */
    DSRTOS_PERIPHERAL_USART1,              /**< USART1 */
    DSRTOS_PERIPHERAL_USART6,              /**< USART6 */
    DSRTOS_PERIPHERAL_ADC1,                /**< ADC1 */
    DSRTOS_PERIPHERAL_ADC2,                /**< ADC2 */
    DSRTOS_PERIPHERAL_ADC3,                /**< ADC3 */
    DSRTOS_PERIPHERAL_SDIO,                /**< SDIO */
    DSRTOS_PERIPHERAL_SPI1,                /**< SPI1 */
    DSRTOS_PERIPHERAL_SYSCFG,              /**< System configuration */
    DSRTOS_PERIPHERAL_TIM9,                /**< Timer 9 */
    DSRTOS_PERIPHERAL_TIM10,               /**< Timer 10 */
    DSRTOS_PERIPHERAL_TIM11,               /**< Timer 11 */
    
    DSRTOS_PERIPHERAL_COUNT                /**< Total peripheral count */
} dsrtos_peripheral_t;

/**
 * @brief Clock source enumeration
 */
typedef enum {
    DSRTOS_CLOCK_SOURCE_HSI = 0,           /**< High Speed Internal oscillator */
    DSRTOS_CLOCK_SOURCE_HSE,               /**< High Speed External crystal */
    DSRTOS_CLOCK_SOURCE_PLL,               /**< Phase Locked Loop */
    DSRTOS_CLOCK_SOURCE_LSI,               /**< Low Speed Internal oscillator */
    DSRTOS_CLOCK_SOURCE_LSE                /**< Low Speed External crystal */
} dsrtos_clock_source_t;

/**
 * @brief Clock frequencies structure
 */
typedef struct {
    uint32_t sysclk_hz;                   /**< System clock frequency */
    uint32_t ahb_hz;                      /**< AHB bus frequency */
    uint32_t apb1_hz;                     /**< APB1 bus frequency */
    uint32_t apb2_hz;                     /**< APB2 bus frequency */
    uint32_t pll_hz;                      /**< PLL output frequency */
} dsrtos_clock_frequencies_t;

/**
 * @brief Clock statistics structure
 */
typedef struct {
    uint32_t clock_switches;              /**< Number of clock source switches */
    uint32_t pll_locks;                   /**< Number of PLL lock events */
    uint32_t clock_failures;              /**< Clock failure count */
    uint32_t peripheral_enables;          /**< Peripheral enable operations */
    bool hse_ready;                       /**< HSE oscillator status */
    bool pll_ready;                       /**< PLL ready status */
    dsrtos_clock_source_t current_source; /**< Current system clock source */
} dsrtos_clock_stats_t;

/**
 * @brief Clock configuration structure
 */
typedef struct {
    bool use_hse;                         /**< Use HSE crystal */
    bool use_pll;                         /**< Use PLL for system clock */
    uint32_t target_sysclk_hz;            /**< Target system clock frequency */
    uint32_t hse_frequency_hz;            /**< HSE crystal frequency */
    uint8_t flash_wait_states;            /**< Flash wait states */
} dsrtos_clock_config_t;

/**
 * @brief Clock monitor callback function type
 * @param source Clock source that failed
 * @param context User context data
 */
typedef void (*dsrtos_clock_monitor_callback_t)(dsrtos_clock_source_t source, void* context);

/*==============================================================================
 * FUNCTION PROTOTYPES
 *==============================================================================*/

/**
 * @defgroup DSRTOS_Clock_Core Core Clock Functions
 * @brief Core clock management functions
 * @{
 */

/**
 * @brief Initialize the DSRTOS clock subsystem
 * 
 * @details Configures the system clock to run at 168MHz from PLL using HSE.
 *          Sets up appropriate bus prescalers and Flash wait states.
 *          Must be called early in system initialization.
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_ALREADY_INITIALIZED if already initialized
 * @return DSRTOS_ERR_TIMEOUT if clock startup timeout
 * @return DSRTOS_ERR_HARDWARE_FAULT if clock configuration fails
 * 
 * @pre None (can be called at system startup)
 * @post System clock running at 168MHz from PLL
 * @post Bus clocks configured: AHB=168MHz, APB1=42MHz, APB2=84MHz
 * 
 * @par Thread Safety
 * Not thread-safe. Must be called during single-threaded initialization.
 * 
 * @par Clock Configuration
 * - HSE: 8MHz external crystal
 * - PLL: HSE/8 * 336 / 2 = 168MHz
 * - AHB: SYSCLK/1 = 168MHz
 * - APB1: AHB/4 = 42MHz (max 42MHz)
 * - APB2: AHB/2 = 84MHz (max 84MHz)
 * - Flash: 5 wait states for 168MHz
 * 
 * @par MISRA-C:2012 Compliance
 * Full compliance with rules 8.7, 17.7, 21.1
 */
dsrtos_result_t dsrtos_clock_init(void);

/**
 * @brief Initialize clock with custom configuration
 * 
 * @param[in] config Clock configuration parameters
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if config is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if configuration invalid
 * @return Other error codes same as dsrtos_clock_init
 */
dsrtos_result_t dsrtos_clock_init_with_config(const dsrtos_clock_config_t* config);

/**
 * @brief Deinitialize clock subsystem
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if not initialized
 */
dsrtos_result_t dsrtos_clock_deinit(void);

/** @} */

/**
 * @defgroup DSRTOS_Clock_Peripheral Peripheral Clock Functions
 * @brief Peripheral clock control functions
 * @{
 */

/**
 * @brief Enable peripheral clock
 * 
 * @details Enables the clock for the specified peripheral. Peripheral
 *          will be ready for use after this call returns successfully.
 * 
 * @param[in] peripheral Peripheral to enable
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if peripheral invalid
 * 
 * @note Some peripherals may require additional GPIO and pin configuration
 * @note Clock enable is persistent until dsrtos_clock_disable_peripheral called
 * 
 * @par Thread Safety
 * Thread-safe with atomic register operations
 * 
 * @par Example
 * @code
 * // Enable GPIOA and USART1 clocks
 * dsrtos_clock_enable_peripheral(DSRTOS_PERIPHERAL_GPIOA);
 * dsrtos_clock_enable_peripheral(DSRTOS_PERIPHERAL_USART1);
 * @endcode
 */
dsrtos_result_t dsrtos_clock_enable_peripheral(dsrtos_peripheral_t peripheral);

/**
 * @brief Disable peripheral clock
 * 
 * @param[in] peripheral Peripheral to disable
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if peripheral invalid
 * 
 * @warning Disabling clock while peripheral is in use may cause system issues
 */
dsrtos_result_t dsrtos_clock_disable_peripheral(dsrtos_peripheral_t peripheral);

/**
 * @brief Check if peripheral clock is enabled
 * 
 * @param[in] peripheral Peripheral to check
 * @param[out] is_enabled Pointer to store enable status
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if is_enabled is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if peripheral invalid
 */
dsrtos_result_t dsrtos_clock_is_peripheral_enabled(dsrtos_peripheral_t peripheral, 
                                                   bool* is_enabled);

/**
 * @brief Enable multiple peripheral clocks
 * 
 * @param[in] peripherals Array of peripherals to enable
 * @param[in] count Number of peripherals in array
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if peripherals is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if count is 0
 */
dsrtos_result_t dsrtos_clock_enable_peripherals(const dsrtos_peripheral_t* peripherals,
                                                uint32_t count);

/** @} */

/**
 * @defgroup DSRTOS_Clock_Info Information Functions
 * @brief Clock information and status functions
 * @{
 */

/**
 * @brief Get current clock frequencies
 * 
 * @param[out] frequencies Pointer to frequency structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if frequencies is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 * 
 * @par Example
 * @code
 * dsrtos_clock_frequencies_t clocks;
 * if (dsrtos_clock_get_frequencies(&clocks) == DSRTOS_OK) {
 *     printf("SYSCLK: %u MHz\n", clocks.sysclk_hz / 1000000);
 *     printf("AHB: %u MHz\n", clocks.ahb_hz / 1000000);
 *     printf("APB1: %u MHz\n", clocks.apb1_hz / 1000000);
 *     printf("APB2: %u MHz\n", clocks.apb2_hz / 1000000);
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_clock_get_frequencies(dsrtos_clock_frequencies_t* frequencies);

/**
 * @brief Get system clock frequency
 * 
 * @return System clock frequency in Hz, or 0 if not initialized
 */
uint32_t dsrtos_clock_get_sysclk_frequency(void);

/**
 * @brief Get AHB clock frequency
 * 
 * @return AHB clock frequency in Hz, or 0 if not initialized
 */
uint32_t dsrtos_clock_get_ahb_frequency(void);

/**
 * @brief Get APB1 clock frequency
 * 
 * @return APB1 clock frequency in Hz, or 0 if not initialized
 */
uint32_t dsrtos_clock_get_apb1_frequency(void);

/**
 * @brief Get APB2 clock frequency
 * 
 * @return APB2 clock frequency in Hz, or 0 if not initialized
 */
uint32_t dsrtos_clock_get_apb2_frequency(void);

/**
 * @brief Get current system clock source
 * 
 * @return Current clock source, or DSRTOS_CLOCK_SOURCE_HSI if not initialized
 */
dsrtos_clock_source_t dsrtos_clock_get_source(void);

/** @} */

/**
 * @defgroup DSRTOS_Clock_Status Status and Monitoring
 * @brief Clock status and monitoring functions
 * @{
 */

/**
 * @brief Get clock statistics
 * 
 * @param[out] stats Pointer to statistics structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if stats is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 */
dsrtos_result_t dsrtos_clock_get_stats(dsrtos_clock_stats_t* stats);

/**
 * @brief Reset clock statistics
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 * 
 * @note Does not reset current frequency values
 */
dsrtos_result_t dsrtos_clock_reset_stats(void);

/**
 * @brief Check if HSE is ready
 * 
 * @return true if HSE oscillator is ready and stable
 */
bool dsrtos_clock_is_hse_ready(void);

/**
 * @brief Check if PLL is ready
 * 
 * @return true if PLL is locked and ready
 */
bool dsrtos_clock_is_pll_ready(void);

/**
 * @brief Register clock monitor callback
 * 
 * @param[in] callback Callback function for clock events
 * @param[in] context User context for callback
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if callback is NULL
 */
dsrtos_result_t dsrtos_clock_register_monitor(dsrtos_clock_monitor_callback_t callback,
                                              void* context);

/** @} */

/**
 * @defgroup DSRTOS_Clock_Control Control Functions
 * @brief Clock control and switching functions
 * @{
 */

/**
 * @brief Switch system clock source
 * 
 * @param[in] source New clock source
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_INVALID_PARAM if source invalid
 * @return DSRTOS_ERR_NOT_READY if source not ready
 * @return DSRTOS_ERR_TIMEOUT if switch timeout
 * 
 * @warning Switching clock source affects all system timing
 * @note Function will ensure target clock is ready before switching
 */
dsrtos_result_t dsrtos_clock_switch_source(dsrtos_clock_source_t source);

/**
 * @brief Enable clock security system
 * 
 * @details Enables hardware clock security system (CSS) to detect HSE failure
 *          and automatically switch to HSI as backup.
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 * 
 * @note CSS generates NMI interrupt on HSE failure
 */
dsrtos_result_t dsrtos_clock_enable_security_system(void);

/**
 * @brief Disable clock security system
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if clock system not initialized
 */
dsrtos_result_t dsrtos_clock_disable_security_system(void);

/** @} */

/**
 * @defgroup DSRTOS_Clock_Utility Utility Functions
 * @brief Clock utility and helper functions
 * @{
 */

/**
 * @brief Convert frequency to human readable string
 * 
 * @param[in] frequency_hz Frequency in Hz
 * @param[out] str_buffer Buffer to store string (min 16 bytes)
 * @param[in] buffer_size Size of string buffer
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if str_buffer is NULL
 * @return DSRTOS_ERR_BUFFER_TOO_SMALL if buffer too small
 * 
 * @par Example
 * @code
 * char freq_str[32];
 * dsrtos_clock_frequency_to_string(168000000, freq_str, sizeof(freq_str));
 * // Result: "168.0 MHz"
 * @endcode
 */
dsrtos_result_t dsrtos_clock_frequency_to_string(uint32_t frequency_hz,
                                                 char* str_buffer,
                                                 uint32_t buffer_size);

/**
 * @brief Calculate timer prescaler for target frequency
 * 
 * @param[in] peripheral_freq_hz Peripheral clock frequency
 * @param[in] target_freq_hz Target timer frequency
 * @param[out] prescaler Pointer to store calculated prescaler
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if prescaler is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if frequencies invalid
 * 
 * @note Useful for configuring timer prescalers based on current clock settings
 */
dsrtos_result_t dsrtos_clock_calculate_prescaler(uint32_t peripheral_freq_hz,
                                                 uint32_t target_freq_hz,
                                                 uint32_t* prescaler);

/** @} */

/*==============================================================================
 * COMPILER ATTRIBUTES FOR CERTIFICATION
 *==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CLOCK_H */

/**
 * @defgroup DSRTOS_Clock_Examples Usage Examples
 * @brief Practical usage examples
 * @{
 * 
 * @par Basic Clock Initialization
 * @code
 * void system_init(void) {
 *     // Initialize clocks first
 *     dsrtos_result_t result = dsrtos_clock_init();
 *     if (result != DSRTOS_OK) {
 *         // Handle clock initialization failure
 *         system_error_handler(result);
 *     }
 *     
 *     // Verify clock frequency
 *     uint32_t sysclk = dsrtos_clock_get_sysclk_frequency();
 *     if (sysclk != DSRTOS_SYSCLK_FREQUENCY_HZ) {
 *         printf("Warning: SYSCLK is %u Hz, expected %u Hz\n", 
 *                sysclk, DSRTOS_SYSCLK_FREQUENCY_HZ);
 *     }
 * }
 * @endcode
 * 
 * @par Peripheral Clock Management
 * @code
 * void setup_uart_gpio(void) {
 *     // Enable clocks for UART1 and GPIO ports
 *     dsrtos_peripheral_t peripherals[] = {
 *         DSRTOS_PERIPHERAL_GPIOA,    // UART1 pins on GPIOA
 *         DSRTOS_PERIPHERAL_USART1    // UART1 peripheral
 *     };
 *     
 *     dsrtos_clock_enable_peripherals(peripherals, 2);
 *     
 *     // Now configure GPIO and UART...
 * }
 * @endcode
 * 
 * @par Clock Monitoring
 * @code
 * void clock_failure_handler(dsrtos_clock_source_t source, void* context) {
 *     switch (source) {
 *         case DSRTOS_CLOCK_SOURCE_HSE:
 *             error_log("HSE oscillator failure detected");
 *             // System automatically switched to HSI backup
 *             break;
 *             
 *         case DSRTOS_CLOCK_SOURCE_PLL:
 *             error_log("PLL unlock detected");
 *             break;
 *             
 *         default:
 *             error_log("Unknown clock failure");
 *             break;
 *     }
 * }
 * 
 * void setup_clock_monitoring(void) {
 *     dsrtos_clock_register_monitor(clock_failure_handler, NULL);
 *     dsrtos_clock_enable_security_system();
 * }
 * @endcode
 * 
 * @par Performance Analysis
 * @code
 * void analyze_clock_performance(void) {
 *     dsrtos_clock_stats_t stats;
 *     dsrtos_clock_frequencies_t freqs;
 *     
 *     if (dsrtos_clock_get_stats(&stats) == DSRTOS_OK &&
 *         dsrtos_clock_get_frequencies(&freqs) == DSRTOS_OK) {
 *         
 *         printf("Clock Performance Report:\n");
 *         printf("- System Clock: %u MHz\n", freqs.sysclk_hz / 1000000);
 *         printf("- Clock Switches: %u\n", stats.clock_switches);
 *         printf("- PLL Locks: %u\n", stats.pll_locks);
 *         printf("- Clock Failures: %u\n", stats.clock_failures);
 *         printf("- HSE Status: %s\n", stats.hse_ready ? "Ready" : "Not Ready");
 *         
 *         if (stats.clock_failures > 0) {
 *             warning_log("Clock failures detected - check crystal");
 *         }
 *     }
 * }
 * @endcode
 * 
 * @}
 */

/*==============================================================================
 * END OF FILE  
 *==============================================================================*/
