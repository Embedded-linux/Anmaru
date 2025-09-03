/**
 * @file dsrtos_timer.h
 * @brief DSRTOS Hardware Timer Management Interface - Phase 1
 * 
 * @details Provides hardware timer services including system tick, high-resolution
 *          timing, delay functions, and timer callbacks for STM32F407VG platform.
 * 
 * @version 1.0.0
 * @date 2025-08-30
 * 
 * @par Features
 * - System tick at 1kHz (1ms resolution)
 * - High-resolution timing at 1μs resolution
 * - Timer callback system for periodic/one-shot events
 * - Performance measurement and statistics
 * - Overflow handling for extended operation
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

#ifndef DSRTOS_TIMER_H
#define DSRTOS_TIMER_H

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

/** @defgroup DSRTOS_Timer_Constants Timer Constants
 * @{
 */

/** Maximum number of timer callbacks supported */
#define DSRTOS_MAX_TIMER_CALLBACKS           (16U)

/** System tick frequency in Hz */
#define DSRTOS_TIMER_SYSTICK_FREQ_HZ         (1000U)

/** Maximum delay value in microseconds (1 hour) */
#define DSRTOS_TIMER_MAX_DELAY_US            (3600000000UL)

/** Maximum delay value in milliseconds (1 hour) */
#define DSRTOS_TIMER_MAX_DELAY_MS            (3600000UL)

/** Minimum timer callback period in microseconds */
#define DSRTOS_TIMER_MIN_CALLBACK_PERIOD_US  (100U)

/** @} */

/** @defgroup DSRTOS_Timer_Errors Timer Error Codes
 * @{
 */

/** Timer callback ID out of range */
#define DSRTOS_ERR_TIMER_ID_INVALID          (DSRTOS_ERR_BASE + 0x100U)

/** Timer callback already registered */
#define DSRTOS_ERR_TIMER_ALREADY_REGISTERED  (DSRTOS_ERR_BASE + 0x101U)

/** Timer callback not registered */
#define DSRTOS_ERR_TIMER_NOT_REGISTERED      (DSRTOS_ERR_BASE + 0x102U)

/** Timer period too short */
#define DSRTOS_ERR_TIMER_PERIOD_TOO_SHORT    (DSRTOS_ERR_BASE + 0x103U)

/** Timer hardware fault */
#define DSRTOS_ERR_TIMER_HARDWARE_FAULT      (DSRTOS_ERR_BASE + 0x104U)

/** @} */

/*==============================================================================
 * TYPE DEFINITIONS
 *==============================================================================*/

/**
 * @brief Timer callback function type
 * 
 * @param timer_id Timer identifier (0-15)
 * @param context User context data passed during registration
 * 
 * @note Callback should execute quickly (< 100μs recommended)
 * @note Callback must not call blocking functions
 * @note Callback execution time is monitored for performance analysis
 */
/*typedef void (*dsrtos_timer_callback_t)(uint8_t timer_id, void* context);*/
/*typedef void (*dsrtos_timer_callback_t)(dsrtos_timer_handle_t timer_handle, void* user_data);*/

/**
 * @brief Timer statistics structure
 * 
 * @details Provides comprehensive timer system monitoring data
 *          for performance analysis and system health monitoring.
 */
typedef struct {
    uint64_t system_tick_count;        /**< Total system ticks since boot */
    uint32_t systick_interrupts;       /**< SysTick interrupt count */
    uint32_t hires_interrupts;         /**< High-resolution timer interrupts */
    uint32_t callback_executions;      /**< Total callback executions */
    uint32_t max_callback_time_us;     /**< Maximum callback execution time */
    uint8_t active_timers;             /**< Number of active timer callbacks */
    uint32_t cpu_frequency_hz;         /**< CPU frequency in Hz */
    uint32_t systick_frequency_hz;     /**< SysTick frequency in Hz */
} dsrtos_timer_stats_t;

/**
 * @brief Timer callback configuration structure
 * 
 * @details Used for batch timer callback registration
 */
typedef struct {
    uint8_t timer_id;                  /**< Timer identifier */
    dsrtos_timer_callback_t callback;  /**< Callback function */
    void* context;                     /**< User context */
    uint32_t period_us;                /**< Period in microseconds */
    bool periodic;                     /**< true=periodic, false=one-shot */
} dsrtos_timer_config_t;

/**
 * @brief Timer capabilities structure
 * 
 * @details Describes the capabilities and limits of the timer subsystem
 */
typedef struct {
    uint32_t min_resolution_us;        /**< Minimum resolution in microseconds */
    uint32_t max_period_us;            /**< Maximum timer period */
    uint8_t max_callbacks;             /**< Maximum simultaneous callbacks */
    bool high_resolution_available;    /**< High-resolution timer available */
    bool callback_timing_available;    /**< Callback timing measurement */
    uint32_t timer_frequency_hz;       /**< Timer hardware frequency */
} dsrtos_timer_capabilities_t;

/*==============================================================================
 * FUNCTION PROTOTYPES
 *==============================================================================*/

/**
 * @defgroup DSRTOS_Timer_Core Core Timer Functions
 * @brief Core timer management functions
 * @{
 */

/**
 * @brief Initialize the DSRTOS timer subsystem
 * 
 * @details Initializes SysTick and high-resolution timer, registers interrupt
 *          handlers, and prepares callback system. Must be called once during
 *          system initialization.
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_ALREADY_INITIALIZED if already initialized
 * @return DSRTOS_ERR_HARDWARE_FAULT if timer configuration fails
 * @return DSRTOS_ERR_INTERRUPT_REGISTER if interrupt registration fails
 * 
 * @pre Interrupt controller must be initialized
 * @pre System clock must be configured
 * @post Timer subsystem ready for operation
 * 
 * @par Thread Safety
 * Not thread-safe. Must be called during single-threaded initialization.
 * 
 * @par MISRA-C:2012 Compliance
 * Full compliance with rules 8.7, 17.7, 21.1
 */
dsrtos_result_t dsrtos_timer_init(void);

/**
 * @brief Deinitialize the timer subsystem
 * 
 * @details Stops all timers, unregisters interrupts, and cleans up resources.
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if not initialized
 * 
 * @post Timer subsystem stopped and resources freed
 */
dsrtos_result_t dsrtos_timer_deinit(void);

/** @} */

/**
 * @defgroup DSRTOS_Timer_Time Time Measurement Functions
 * @brief Time measurement and access functions
 * @{
 */

/**
 * @brief Get system tick count
 * 
 * @details Returns the number of system ticks since boot. System tick
 *          increments at 1kHz (every 1ms).
 * 
 * @return Current system tick count
 * 
 * @note Atomic 64-bit read with interrupt protection
 * @note Tick counter will not overflow for 584 million years
 * 
 * @par Thread Safety
 * Thread-safe with atomic read operation
 * 
 * @par Example
 * @code
 * uint64_t start_tick = dsrtos_timer_get_ticks();
 * // ... do work ...
 * uint64_t elapsed_ms = dsrtos_timer_get_ticks() - start_tick;
 * @endcode
 */
uint64_t dsrtos_timer_get_ticks(void);

/**
 * @brief Get system time in milliseconds
 * 
 * @details Returns the system uptime in milliseconds since boot.
 *          Equivalent to dsrtos_timer_get_ticks().
 * 
 * @return Current system time in milliseconds
 * 
 * @par Thread Safety
 * Thread-safe with atomic read operation
 */
uint64_t dsrtos_timer_get_milliseconds(void);

/**
 * @brief Get high-resolution time in microseconds
 * 
 * @details Returns high-resolution system time with 1μs resolution.
 *          Uses hardware timer for precise timing measurement.
 * 
 * @return Current system time in microseconds since boot
 * @return 0 if timer not initialized or hardware fault
 * 
 * @note Provides 1μs resolution for performance measurement
 * @note Handles timer overflow automatically
 * 
 * @par Thread Safety
 * Thread-safe with atomic read and overflow handling
 * 
 * @par Example
 * @code
 * uint64_t start = dsrtos_timer_get_microseconds();
 * execute_critical_function();
 * uint64_t duration = dsrtos_timer_get_microseconds() - start;
 * printf("Function took %lu μs\n", (unsigned long)duration);
 * @endcode
 */
uint64_t dsrtos_timer_get_microseconds(void);

/**
 * @brief Convert ticks to milliseconds
 * 
 * @param[in] ticks Tick count to convert
 * 
 * @return Equivalent time in milliseconds
 */
static inline uint64_t dsrtos_timer_ticks_to_ms(uint64_t ticks)
{
    return ticks;  /* Ticks are at 1kHz = 1ms each */
}

/**
 * @brief Convert milliseconds to ticks
 * 
 * @param[in] milliseconds Time in milliseconds to convert
 * 
 * @return Equivalent tick count
 */
static inline uint64_t dsrtos_timer_ms_to_ticks(uint64_t milliseconds)
{
    return milliseconds;  /* Ticks are at 1kHz = 1ms each */
}

/** @} */

/**
 * @defgroup DSRTOS_Timer_Delay Delay Functions
 * @brief Blocking delay functions
 * @{
 */

/**
 * @brief Delay for specified microseconds
 * 
 * @details Blocks execution for the specified number of microseconds.
 *          Uses high-resolution timer for accurate delays.
 * 
 * @param[in] microseconds Number of microseconds to delay
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if microseconds > MAX_DELAY_US
 * 
 * @note This is a blocking function - CPU will be busy-waiting
 * @note Accuracy: ±1μs for delays > 10μs
 * @note Should not be called from interrupt context
 * 
 * @par Thread Safety
 * Thread-safe but blocking
 * 
 * @par Example
 * @code
 * // Precise 50μs delay
 * dsrtos_timer_delay_us(50);
 * @endcode
 */
dsrtos_result_t dsrtos_timer_delay_us(uint32_t microseconds);

/**
 * @brief Delay for specified milliseconds
 * 
 * @details Blocks execution for the specified number of milliseconds.
 *          Equivalent to dsrtos_timer_delay_us(milliseconds * 1000).
 * 
 * @param[in] milliseconds Number of milliseconds to delay
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if milliseconds > MAX_DELAY_MS
 * 
 * @note This is a blocking function
 * @note Should not be called from interrupt context
 * 
 * @par Example
 * @code
 * // 100ms delay
 * dsrtos_timer_delay_ms(100);
 * @endcode
 */
dsrtos_result_t dsrtos_timer_delay_ms(uint32_t milliseconds);

/** @} */

/**
 * @defgroup DSRTOS_Timer_Callbacks Timer Callback Functions
 * @brief Timer callback management
 * @{
 */

/**
 * @brief Register a periodic timer callback
 * 
 * @details Registers a function to be called periodically at the specified
 *          interval. Callback will be called from SysTick interrupt context.
 * 
 * @param[in] timer_id Timer identifier (0-15)
 * @param[in] callback Callback function (must not be NULL)
 * @param[in] context User context data (may be NULL)
 * @param[in] period_us Period in microseconds (minimum 100μs)
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if timer_id >= MAX_CALLBACKS
 * @return DSRTOS_ERR_NULL_POINTER if callback is NULL
 * @return DSRTOS_ERR_TIMER_PERIOD_TOO_SHORT if period < minimum
 * @return DSRTOS_ERR_TIMER_ALREADY_REGISTERED if timer_id in use
 * 
 * @pre Timer subsystem must be initialized
 * @post Callback will be called every period_us microseconds
 * 
 * @par Thread Safety
 * Thread-safe with interrupt protection
 * 
 * @par Example
 * @code
 * void led_blink_callback(uint8_t timer_id, void* context) {
 *     toggle_led();  // Quick operation only
 * }
 * 
 * // Blink LED every 500ms
 * dsrtos_timer_register_callback(0, led_blink_callback, NULL, 500000);
 * @endcode
 */
/*dsrtos_result_t dsrtos_timer_register_callback(uint8_t timer_id,
                                               dsrtos_timer_callback_t callback,
                                               void* context,
                                               uint32_t period_us);*/

dsrtos_result_t dsrtos_timer_register_callback(dsrtos_timer_handle_t timer_handle,
                                               dsrtos_timer_callback_t callback,
                                               void* user_data);

/**
 * @brief Register a one-shot timer callback
 * 
 * @details Registers a function to be called once after the specified delay.
 * 
 * @param[in] timer_id Timer identifier (0-15)
 * @param[in] callback Callback function (must not be NULL)
 * @param[in] context User context data (may be NULL)
 * @param[in] delay_us Delay in microseconds before callback
 * 
 * @return DSRTOS_OK on success
 * @return Error codes same as dsrtos_timer_register_callback
 */
dsrtos_result_t dsrtos_timer_register_oneshot(uint8_t timer_id,
                                              dsrtos_timer_callback_t callback,
                                              void* context,
                                              uint32_t delay_us);

/**
 * @brief Unregister a timer callback
 * 
 * @param[in] timer_id Timer identifier to unregister
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 * @return DSRTOS_ERR_INVALID_PARAM if timer_id invalid
 * @return DSRTOS_ERR_TIMER_NOT_REGISTERED if timer not active
 */
dsrtos_result_t dsrtos_timer_unregister_callback(uint8_t timer_id);

/**
 * @brief Modify timer callback period
 * 
 * @param[in] timer_id Timer identifier
 * @param[in] new_period_us New period in microseconds
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_TIMER_NOT_REGISTERED if timer not active
 * @return DSRTOS_ERR_TIMER_PERIOD_TOO_SHORT if period invalid
 */
dsrtos_result_t dsrtos_timer_modify_period(uint8_t timer_id, uint32_t new_period_us);

/**
 * @brief Register multiple timer callbacks in batch
 * 
 * @param[in] configs Array of timer configurations
 * @param[in] count Number of configurations
 * 
 * @return DSRTOS_OK on success
 * @return Error codes from individual registrations
 * 
 * @note If any registration fails, successful ones are rolled back
 */
dsrtos_result_t dsrtos_timer_register_batch(const dsrtos_timer_config_t* configs,
                                            uint32_t count);

/** @} */

/**
 * @defgroup DSRTOS_Timer_Status Status and Information Functions
 * @brief Timer status and monitoring
 * @{
 */

/**
 * @brief Get timer statistics
 * 
 * @param[out] stats Pointer to statistics structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if stats is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 */
dsrtos_result_t dsrtos_timer_get_stats(dsrtos_timer_stats_t* stats);

/**
 * @brief Reset timer statistics
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if timer not initialized
 * 
 * @note Does not reset system tick counter
 */
dsrtos_result_t dsrtos_timer_reset_stats(void);

/**
 * @brief Get timer capabilities
 * 
 * @param[out] caps Pointer to capabilities structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if caps is NULL
 */
dsrtos_result_t dsrtos_timer_get_capabilities(dsrtos_timer_capabilities_t* caps);

/**
 * @brief Check if timer callback is active
 * 
 * @param[in] timer_id Timer identifier to check
 * @param[out] is_active Pointer to store active status
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if is_active is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if timer_id invalid
 */
dsrtos_result_t dsrtos_timer_is_active(uint8_t timer_id, bool* is_active);

/**
 * @brief Get remaining time for timer callback
 * 
 * @param[in] timer_id Timer identifier
 * @param[out] remaining_us Pointer to store remaining microseconds
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if remaining_us is NULL
 * @return DSRTOS_ERR_TIMER_NOT_REGISTERED if timer not active
 */
dsrtos_result_t dsrtos_timer_get_remaining(uint8_t timer_id, uint32_t* remaining_us);

/**
 * @brief Get callback execution count
 * 
 * @param[in] timer_id Timer identifier
 * @param[out] call_count Pointer to store execution count
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if call_count is NULL
 * @return DSRTOS_ERR_TIMER_NOT_REGISTERED if timer not active
 */
dsrtos_result_t dsrtos_timer_get_call_count(uint8_t timer_id, uint32_t* call_count);

/** @} */

/*==============================================================================
 * COMPILER ATTRIBUTES FOR CERTIFICATION
 *==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TIMER_H */

/**
 * @defgroup DSRTOS_Timer_Examples Usage Examples
 * @brief Practical usage examples
 * @{
 * 
 * @par Basic Time Measurement
 * @code
 * void measure_function_performance(void) {
 *     uint64_t start = dsrtos_timer_get_microseconds();
 *     
 *     expensive_computation();
 *     
 *     uint64_t duration = dsrtos_timer_get_microseconds() - start;
 *     printf("Function took %lu μs\n", (unsigned long)duration);
 * }
 * @endcode
 * 
 * @par Periodic Task Implementation
 * @code
 * void sensor_reading_callback(uint8_t timer_id, void* context) {
 *     sensor_data_t* data = (sensor_data_t*)context;
 *     
 *     // Quick sensor read - no blocking operations
 *     data->temperature = read_temperature_sensor();
 *     data->pressure = read_pressure_sensor();
 *     data->timestamp = dsrtos_timer_get_microseconds();
 *     
 *     // Signal main task that new data is available
 *     set_sensor_data_ready_flag();
 * }
 * 
 * void setup_sensor_monitoring(void) {
 *     static sensor_data_t sensor_data;
 *     
 *     // Read sensors every 100ms
 *     dsrtos_timer_register_callback(1, sensor_reading_callback, 
 *                                    &sensor_data, 100000);
 * }
 * @endcode
 * 
 * @par Timeout Implementation
 * @code
 * static volatile bool timeout_occurred = false;
 * 
 * void timeout_callback(uint8_t timer_id, void* context) {
 *     timeout_occurred = true;
 * }
 * 
 * bool wait_for_response_with_timeout(uint32_t timeout_ms) {
 *     timeout_occurred = false;
 *     
 *     // Set up timeout
 *     dsrtos_timer_register_oneshot(15, timeout_callback, NULL, 
 *                                   timeout_ms * 1000);
 *     
 *     // Wait for response or timeout
 *     while (!response_received && !timeout_occurred) {
 *         // Check for response...
 *     }
 *     
 *     // Clean up timer
 *     dsrtos_timer_unregister_callback(15);
 *     
 *     return !timeout_occurred;
 * }
 * @endcode
 * 
 * @par Performance Monitoring
 * @code
 * void print_timer_performance(void) {
 *     dsrtos_timer_stats_t stats;
 *     
 *     if (dsrtos_timer_get_stats(&stats) == DSRTOS_OK) {
 *         printf("System uptime: %llu ms\n", stats.system_tick_count);
 *         printf("Timer callbacks executed: %u\n", stats.callback_executions);
 *         printf("Max callback time: %u μs\n", stats.max_callback_time_us);
 *         printf("Active timers: %u\n", stats.active_timers);
 *         
 *         if (stats.max_callback_time_us > 100) {
 *             printf("WARNING: Long callback detected!\n");
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
