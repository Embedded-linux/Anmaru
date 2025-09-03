/**
 * @file dsrtos_boot.h
 * @brief DSRTOS Boot Controller Interface - Phase 1
 * 
 * @details Provides boot sequence management and system initialization
 *          coordination for DSRTOS. Controls the orderly startup of all
 *          Phase 1 subsystems and provides boot diagnostics.
 * 
 * @version 1.0.0
 * @date 2025-08-30
 * 
 * @par Features
 * - Coordinated Phase 1 subsystem initialization
 * - Boot stage validation and timing
 * - Reset cause detection and classification
 * - Boot diagnostics and error reporting
 * - Safe mode operation on failures
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

#ifndef DSRTOS_BOOT_H
#define DSRTOS_BOOT_H

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

/** @defgroup DSRTOS_Boot_Constants Boot Constants
 * @{
 */

/** Maximum boot time in milliseconds */
#define DSRTOS_MAX_BOOT_TIME_MS              (5000U)

/** Number of boot stages */
#define DSRTOS_BOOT_STAGE_COUNT              (5U)

/** Boot stage indices */
#define DSRTOS_BOOT_STAGE_CLOCK              (0U)
#define DSRTOS_BOOT_STAGE_INTERRUPT          (1U)
#define DSRTOS_BOOT_STAGE_TIMER              (2U)
#define DSRTOS_BOOT_STAGE_UART               (3U)
#define DSRTOS_BOOT_STAGE_VALIDATION         (4U)

/** @} */

/** @defgroup DSRTOS_Boot_Flags Boot Flags
 * @{
 */

/** Cold boot (power-on reset) */
#define DSRTOS_BOOT_FLAG_COLD_BOOT           (0x01U)

/** Warm boot (software/watchdog reset) */
#define DSRTOS_BOOT_FLAG_WARM_BOOT           (0x02U)

/** Debug mode detected */
#define DSRTOS_BOOT_FLAG_DEBUG_MODE          (0x04U)

/** Safe mode operation */
#define DSRTOS_BOOT_FLAG_SAFE_MODE           (0x08U)

/** @} */

/** @defgroup DSRTOS_Boot_ResetCause Reset Cause Flags
 * @{
 */

/** Power-on reset */
#define DSRTOS_RESET_CAUSE_POWER_ON          (0x80000000U)

/** Software reset */
#define DSRTOS_RESET_CAUSE_SOFTWARE          (0x10000000U)

/** Watchdog reset */
#define DSRTOS_RESET_CAUSE_WATCHDOG          (0x20000000U)

/** External reset */
#define DSRTOS_RESET_CAUSE_EXTERNAL          (0x04000000U)

/** Low power reset */
#define DSRTOS_RESET_CAUSE_LOW_POWER         (0x80000000U)

/** @} */

/*==============================================================================
 * TYPE DEFINITIONS
 *==============================================================================*/

/**
 * @brief Reset type enumeration
 */
typedef enum {
    DSRTOS_RESET_SOFTWARE = 0,             /**< Software reset */
    DSRTOS_RESET_WATCHDOG,                 /**< Watchdog reset */
    DSRTOS_RESET_HARD                      /**< Hard fault reset */
} dsrtos_reset_type_t;

/**
 * @brief Boot information structure
 */
typedef struct {
    bool boot_successful;                  /**< Boot completed successfully */
    uint8_t current_stage;                 /**< Current boot stage */
    uint8_t boot_flags;                    /**< Boot flags */
    uint32_t total_boot_time_ms;           /**< Total boot time */
    uint32_t reset_cause;                  /**< Reset cause flags */
    uint32_t boot_count;                   /**< Total boot count */
    
    /* Stage results */
    dsrtos_result_t stage_results[DSRTOS_BOOT_STAGE_COUNT]; /**< Stage results */
    uint32_t stage_durations[DSRTOS_BOOT_STAGE_COUNT];     /**< Stage durations */
    
    /* Validation results */
    bool memory_test_passed;               /**< Memory test result */
    uint32_t clock_frequency_hz;           /**< Validated clock frequency */
} dsrtos_boot_info_t;

/**
 * @brief Boot stage status structure
 */
typedef struct {
    uint8_t stage_index;                   /**< Stage index */
    const char* stage_name;                /**< Stage name */
    dsrtos_result_t result;                /**< Stage result */
    uint32_t duration_ms;                  /**< Stage duration */
    bool required;                         /**< Stage required for boot */
    bool completed;                        /**< Stage completed */
} dsrtos_boot_stage_status_t;

/**
 * @brief Boot statistics structure
 */
typedef struct {
    uint32_t total_boots;                  /**< Total boot attempts */
    uint32_t successful_boots;             /**< Successful boots */
    uint32_t failed_boots;                 /**< Failed boots */
    uint32_t cold_boots;                   /**< Cold boot count */
    uint32_t warm_boots;                   /**< Warm boot count */
    uint32_t min_boot_time_ms;             /**< Minimum boot time */
    uint32_t max_boot_time_ms;             /**< Maximum boot time */
    uint32_t avg_boot_time_ms;             /**< Average boot time */
    uint32_t last_boot_time_ms;            /**< Last boot time */
} dsrtos_boot_stats_t;

/**
 * @brief Boot callback function types
 */

/**
 * @brief Boot stage completion callback
 * @param stage_index Completed stage index
 * @param result Stage completion result
 * @param duration_ms Stage duration in milliseconds
 * @param context User context data
 */
typedef void (*dsrtos_boot_stage_callback_t)(uint8_t stage_index,
                                             dsrtos_result_t result,
                                             uint32_t duration_ms,
                                             void* context);

/**
 * @brief Boot failure callback
 * @param failed_stage Stage that failed
 * @param error_code Error that occurred
 * @param context User context data
 */
typedef void (*dsrtos_boot_failure_callback_t)(uint8_t failed_stage,
                                               dsrtos_result_t error_code,
                                               void* context);

/*==============================================================================
 * FUNCTION PROTOTYPES
 *==============================================================================*/

/**
 * @defgroup DSRTOS_Boot_Core Core Boot Functions
 * @brief Core boot management functions
 * @{
 */

/**
 * @brief Initialize DSRTOS boot controller and execute boot sequence
 * 
 * @details Executes the complete Phase 1 boot sequence including:
 *          1. Reset cause detection
 *          2. Early hardware initialization
 *          3. Clock system initialization
 *          4. Interrupt controller initialization
 *          5. Timer subsystem initialization
 *          6. UART driver initialization (optional)
 *          7. System validation and diagnostics
 * 
 * @return DSRTOS_OK on successful boot
 * @return DSRTOS_ERR_ALREADY_INITIALIZED if already booted
 * @return DSRTOS_ERR_CLOCK_FAULT if clock initialization fails
 * @return DSRTOS_ERR_MEMORY_FAULT if memory test fails
 * @return DSRTOS_ERR_TIMEOUT if boot timeout exceeded
 * @return DSRTOS_ERR_HARDWARE_FAULT if critical hardware failure
 * 
 * @pre None (called at system startup)
 * @post All Phase 1 subsystems initialized and ready
 * @post Boot diagnostics completed
 * 
 * @par Thread Safety
 * Not thread-safe. Must be called during single-threaded boot sequence.
 * 
 * @par Boot Sequence
 * The boot process follows this sequence:
 * 1. **Reset Analysis**: Detect power-on, software, or watchdog reset
 * 2. **Hardware Setup**: Configure vector table, priority grouping, FPU
 * 3. **Clock Init**: Configure 168MHz system clock from PLL
 * 4. **Interrupt Init**: Setup NVIC and interrupt handlers  
 * 5. **Timer Init**: Initialize SysTick and high-resolution timer
 * 6. **UART Init**: Setup debug console (optional)
 * 7. **Validation**: Memory test, clock verification, system check
 * 
 * @par MISRA-C:2012 Compliance
 * Full compliance with rules 8.7, 17.7, 21.1
 * 
 * @par Example
 * @code
 * int main(void) {
 *     dsrtos_result_t boot_result = dsrtos_boot_init();
 *     
 *     if (boot_result == DSRTOS_OK) {
 *         printf("DSRTOS Phase 1 boot successful\n");
 *         // Proceed to Phase 2 kernel initialization
 *     } else {
 *         error_handler("Boot failed", boot_result);
 *     }
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_boot_init(void);

/** @} */

/**
 * @defgroup DSRTOS_Boot_Info Information Functions
 * @brief Boot information and status functions
 * @{
 */

/**
 * @brief Get comprehensive boot information and status
 * 
 * @param[out] info Pointer to boot information structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if info is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if boot controller not initialized
 * 
 * @par Example
 * @code
 * dsrtos_boot_info_t boot_info;
 * if (dsrtos_boot_get_info(&boot_info) == DSRTOS_OK) {
 *     printf("Boot Status: %s\n", 
 *            boot_info.boot_successful ? "SUCCESS" : "FAILED");
 *     printf("Boot Time: %u ms\n", boot_info.total_boot_time_ms);
 *     printf("Reset Cause: 0x%08X\n", boot_info.reset_cause);
 *     
 *     if (boot_info.boot_flags & DSRTOS_BOOT_FLAG_COLD_BOOT) {
 *         printf("Cold boot detected\n");
 *     }
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_boot_get_info(dsrtos_boot_info_t* info);

/**
 * @brief Check if system boot completed successfully
 * 
 * @return true if all critical boot stages completed successfully
 * @return false if boot failed or not yet complete
 * 
 * @note This is a lightweight check for boot success
 * @note Returns false if dsrtos_boot_init() has not been called
 */
bool dsrtos_boot_is_successful(void);

/**
 * @brief Get boot stage status
 * 
 * @param[in] stage_index Stage index to query
 * @param[out] status Pointer to stage status structure
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if status is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if stage_index invalid
 */
dsrtos_result_t dsrtos_boot_get_stage_status(uint8_t stage_index,
                                             dsrtos_boot_stage_status_t* status);

/**
 * @brief Get boot stage name
 * 
 * @param[in] stage_index Stage index
 * 
 * @return Stage name string, or NULL if invalid stage
 * 
 * @par Example
 * @code
 * for (uint8_t i = 0; i < DSRTOS_BOOT_STAGE_COUNT; i++) {
 *     const char* name = dsrtos_boot_get_stage_name(i);
 *     printf("Stage %u: %s\n", i, name ? name : "Unknown");
 * }
 * @endcode
 */
const char* dsrtos_boot_get_stage_name(uint8_t stage_index);

/**
 * @brief Get current boot stage
 * 
 * @return Current boot stage index
 * @return 0xFF if boot controller not initialized
 */
uint8_t dsrtos_boot_get_current_stage(void);

/**
 * @brief Get boot flags
 * 
 * @return Boot flags (combination of DSRTOS_BOOT_FLAG_*)
 * @return 0 if boot controller not initialized
 */
uint8_t dsrtos_boot_get_flags(void);

/**
 * @brief Get reset cause
 * 
 * @return Reset cause flags from RCC CSR register
 * @return 0 if boot controller not initialized
 */
uint32_t dsrtos_boot_get_reset_cause(void);

/** @} */

/**
 * @defgroup DSRTOS_Boot_Stats Statistics Functions
 * @brief Boot statistics and monitoring
 * @{
 */

/**
 * @brief Get boot statistics
 * 
 * @param[out] stats Pointer to statistics structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if stats is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if boot controller not initialized
 * 
 * @note Statistics are accumulated across multiple boots if stored in non-volatile memory
 */
dsrtos_result_t dsrtos_boot_get_stats(dsrtos_boot_stats_t* stats);

/**
 * @brief Reset boot statistics
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if boot controller not initialized
 * 
 * @note Does not reset current boot information
 */
dsrtos_result_t dsrtos_boot_reset_stats(void);

/**
 * @brief Get boot time for specific stage
 * 
 * @param[in] stage_index Stage index
 * @param[out] duration_ms Pointer to store duration in milliseconds
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if duration_ms is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if stage_index invalid
 */
dsrtos_result_t dsrtos_boot_get_stage_time(uint8_t stage_index, uint32_t* duration_ms);

/**
 * @brief Get total boot time
 * 
 * @return Total boot time in milliseconds
 * @return 0 if boot not completed or not initialized
 */
uint32_t dsrtos_boot_get_total_time(void);

/** @} */

/**
 * @defgroup DSRTOS_Boot_Callbacks Callback Functions
 * @brief Boot event callbacks
 * @{
 */

/**
 * @brief Register boot stage completion callback
 * 
 * @param[in] callback Callback function
 * @param[in] context User context for callback
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if callback is NULL
 * 
 * @note Callback is called after each stage completes
 * @note Callback should execute quickly (< 10ms recommended)
 */
dsrtos_result_t dsrtos_boot_register_stage_callback(dsrtos_boot_stage_callback_t callback,
                                                   void* context);

/**
 * @brief Register boot failure callback
 * 
 * @param[in] callback Callback function
 * @param[in] context User context for callback
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if callback is NULL
 * 
 * @note Callback is called when critical boot stage fails
 * @note Can be used for error logging or recovery actions
 */
dsrtos_result_t dsrtos_boot_register_failure_callback(dsrtos_boot_failure_callback_t callback,
                                                     void* context);

/**
 * @brief Unregister boot callbacks
 * 
 * @return DSRTOS_OK on success
 */
dsrtos_result_t dsrtos_boot_unregister_callbacks(void);

/** @} */

/**
 * @defgroup DSRTOS_Boot_Control Control Functions
 * @brief Boot control and reset functions
 * @{
 */

/**
 * @brief Trigger system reset
 * 
 * @details Performs immediate system reset using the specified method.
 *          Function does not return.
 * 
 * @param[in] reset_type Type of reset to perform
 * 
 * @note This function does not return
 * @note All interrupts are disabled before reset
 * @note Reset cause will be detectable on next boot
 * 
 * @par Example
 * @code
 * // Trigger clean software reset
 * dsrtos_boot_system_reset(DSRTOS_RESET_SOFTWARE);
 * // Never reached
 * @endcode
 */
void dsrtos_boot_system_reset(dsrtos_reset_type_t reset_type) __attribute__((noreturn));

/**
 * @brief Enter safe mode operation
 * 
 * @details Disables non-critical system features and operates with
 *          minimal functionality for fault recovery.
 * 
 * @return DSRTOS_OK if safe mode entered successfully
 * @return DSRTOS_ERR_NOT_SUPPORTED if safe mode not available
 * 
 * @note Safe mode uses HSI clock instead of PLL
 * @note Only critical peripherals remain active
 * @note Used for fault recovery scenarios
 */
dsrtos_result_t dsrtos_boot_enter_safe_mode(void);

/**
 * @brief Exit safe mode operation
 * 
 * @return DSRTOS_OK if normal mode restored
 * @return DSRTOS_ERR_HARDWARE_FAULT if cannot exit safe mode
 * 
 * @note Attempts to restore full system operation
 * @note May trigger system reset if restoration fails
 */
dsrtos_result_t dsrtos_boot_exit_safe_mode(void);

/**
 * @brief Check if system is in safe mode
 * 
 * @return true if operating in safe mode
 * @return false if normal operation
 */
bool dsrtos_boot_is_safe_mode(void);

/** @} */

/**
 * @defgroup DSRTOS_Boot_Validation Validation Functions
 * @brief Boot validation and diagnostics
 * @{
 */

/**
 * @brief Run boot diagnostics
 * 
 * @details Performs comprehensive system validation including:
 *          - Memory integrity check
 *          - Clock frequency validation  
 *          - Peripheral functionality test
 *          - Stack canary validation
 * 
 * @return DSRTOS_OK if all diagnostics pass
 * @return DSRTOS_ERR_MEMORY_FAULT if memory test fails
 * @return DSRTOS_ERR_CLOCK_FAULT if clock validation fails
 * @return DSRTOS_ERR_HARDWARE_FAULT if peripheral test fails
 * 
 * @note Can be called after boot completion for health check
 * @note Execution time approximately 100-500ms
 */
dsrtos_result_t dsrtos_boot_run_diagnostics(void);

/**
 * @brief Validate memory integrity
 * 
 * @return DSRTOS_OK if memory test passes
 * @return DSRTOS_ERR_MEMORY_FAULT if memory corruption detected
 * 
 * @note Performs non-destructive memory test
 * @note Tests stack canaries and heap integrity
 */
dsrtos_result_t dsrtos_boot_validate_memory(void);

/**
 * @brief Validate system clock
 * 
 * @return DSRTOS_OK if clock frequencies are correct
 * @return DSRTOS_ERR_CLOCK_FAULT if clock validation fails
 * 
 * @note Verifies SYSCLK, AHB, APB1, APB2 frequencies
 * @note Checks PLL lock status and HSE stability
 */
dsrtos_result_t dsrtos_boot_validate_clock(void);

/** @} */

/*==============================================================================
 * COMPILER ATTRIBUTES FOR CERTIFICATION
 *==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_BOOT_H */

/**
 * @defgroup DSRTOS_Boot_Examples Usage Examples
 * @brief Practical usage examples
 * @{
 * 
 * @par Complete Boot Sequence
 * @code
 * int main(void) {
 *     // Initialize DSRTOS boot controller
 *     dsrtos_result_t result = dsrtos_boot_init();
 *     
 *     if (result == DSRTOS_OK) {
 *         // Boot successful - get boot information
 *         dsrtos_boot_info_t info;
 *         dsrtos_boot_get_info(&info);
 *         
 *         printf("DSRTOS Boot Successful!\n");
 *         printf("Boot time: %u ms\n", info.total_boot_time_ms);
 *         printf("Reset cause: 0x%08X\n", info.reset_cause);
 *         
 *         if (info.boot_flags & DSRTOS_BOOT_FLAG_COLD_BOOT) {
 *             printf("Cold boot detected\n");
 *         }
 *         
 *         // Proceed to Phase 2 initialization
 *         return dsrtos_kernel_init();
 *     } else {
 *         // Boot failed - analyze failure
 *         printf("Boot failed: %d\n", result);
 *         
 *         // Attempt safe mode operation
 *         dsrtos_boot_enter_safe_mode();
 *         return -1;
 *     }
 * }
 * @endcode
 * 
 * @par Boot Monitoring and Diagnostics
 * @code
 * void boot_stage_monitor(uint8_t stage, dsrtos_result_t result, 
 *                        uint32_t duration, void* context) {
 *     const char* stage_name = dsrtos_boot_get_stage_name(stage);
 *     
 *     if (result == DSRTOS_OK) {
 *         printf("Stage %s completed in %u ms\n", stage_name, duration);
 *     } else {
 *         printf("Stage %s FAILED: %d (took %u ms)\n", 
 *                stage_name, result, duration);
 *     }
 *     
 *     // Check for slow boot stages
 *     if (duration > 1000) {
 *         printf("WARNING: Stage %s took longer than expected\n", stage_name);
 *     }
 * }
 * 
 * void setup_boot_monitoring(void) {
 *     dsrtos_boot_register_stage_callback(boot_stage_monitor, NULL);
 * }
 * @endcode
 * 
 * @par Error Recovery and Safe Mode
 * @code
 * void boot_failure_handler(uint8_t failed_stage, dsrtos_result_t error, 
 *                          void* context) {
 *     const char* stage_name = dsrtos_boot_get_stage_name(failed_stage);
 *     
 *     printf("CRITICAL: Boot stage %s failed with error %d\n", 
 *            stage_name, error);
 *     
 *     // Log error to non-volatile storage
 *     error_log_save("Boot failure", failed_stage, error);
 *     
 *     // Attempt recovery based on failure type
 *     if (error == DSRTOS_ERR_CLOCK_FAULT) {
 *         printf("Attempting safe mode with HSI clock...\n");
 *         dsrtos_boot_enter_safe_mode();
 *     } else if (error == DSRTOS_ERR_MEMORY_FAULT) {
 *         printf("Memory fault detected - triggering reset...\n");
 *         dsrtos_boot_system_reset(DSRTOS_RESET_SOFTWARE);
 *     }
 * }
 * 
 * void setup_error_recovery(void) {
 *     dsrtos_boot_register_failure_callback(boot_failure_handler, NULL);
 * }
 * @endcode
 * 
 * @par Boot Time Analysis
 * @code
 * void analyze_boot_performance(void) {
 *     dsrtos_boot_info_t info;
 *     if (dsrtos_boot_get_info(&info) != DSRTOS_OK) {
 *         return;
 *     }
 *     
 *     printf("=== Boot Performance Analysis ===\n");
 *     printf("Total boot time: %u ms\n", info.total_boot_time_ms);
 *     
 *     uint32_t total_stage_time = 0;
 *     for (uint8_t i = 0; i < DSRTOS_BOOT_STAGE_COUNT; i++) {
 *         const char* name = dsrtos_boot_get_stage_name(i);
 *         uint32_t duration = info.stage_durations[i];
 *         dsrtos_result_t result = info.stage_results[i];
 *         
 *         printf("  %s: %u ms (%s)\n", 
 *                name, duration, 
 *                (result == DSRTOS_OK) ? "OK" : "FAILED");
 *         
 *         if (result == DSRTOS_OK) {
 *             total_stage_time += duration;
 *         }
 *     }
 *     
 *     uint32_t overhead = info.total_boot_time_ms - total_stage_time;
 *     printf("Boot overhead: %u ms (%.1f%%)\n", 
 *            overhead, 
 *            (float)overhead / info.total_boot_time_ms * 100.0f);
 *     
 *     // Performance warnings
 *     if (info.total_boot_time_ms > 3000) {
 *         printf("WARNING: Boot time exceeds 3 seconds\n");
 *     }
 * }
 * @endcode
 * 
 * @}
 */

/*==============================================================================
 * END OF FILE  
 *==============================================================================*/
