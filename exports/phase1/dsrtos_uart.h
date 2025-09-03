/**
 * @file dsrtos_uart.h
 * @brief DSRTOS UART Driver Interface - Phase 1
 * 
 * @details Provides UART communication services with interrupt-driven TX/RX,
 *          circular buffers, and callback support for STM32F407VG platform.
 * 
 * @version 1.0.0
 * @date 2025-08-30
 * 
 * @par Features
 * - Up to 6 UART instances (USART1-6, UART4-5)
 * - Interrupt-driven TX/RX with circular buffers
 * - Configurable baud rates (9600 to 3000000 bps)
 * - Error detection and reporting
 * - Performance statistics and monitoring
 * - Callback system for events
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

#ifndef DSRTOS_UART_H
#define DSRTOS_UART_H

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

/** @defgroup DSRTOS_UART_Constants UART Constants
 * @{
 */

/** Maximum UART instances supported */
#define DSRTOS_MAX_UART_INSTANCES            (6U)

/** UART instance IDs */
#define DSRTOS_UART1                         (0U)
#define DSRTOS_UART2                         (1U)
#define DSRTOS_UART3                         (2U)
#define DSRTOS_UART4                         (3U)
#define DSRTOS_UART5                         (4U)
#define DSRTOS_UART6                         (5U)

/** Default buffer sizes */
#define DSRTOS_UART_DEFAULT_TX_BUFFER_SIZE   (512U)
#define DSRTOS_UART_DEFAULT_RX_BUFFER_SIZE   (512U)

/** Maximum buffer sizes */
#define DSRTOS_UART_MAX_BUFFER_SIZE          (4096U)

/** @} */

/** @defgroup DSRTOS_UART_BaudRates Standard Baud Rates
 * @{
 */

#define DSRTOS_UART_BAUD_9600                (9600U)
#define DSRTOS_UART_BAUD_19200               (19200U)
#define DSRTOS_UART_BAUD_38400               (38400U)
#define DSRTOS_UART_BAUD_57600               (57600U)
#define DSRTOS_UART_BAUD_115200              (115200U)
#define DSRTOS_UART_BAUD_230400              (230400U)
#define DSRTOS_UART_BAUD_460800              (460800U)
#define DSRTOS_UART_BAUD_921600              (921600U)
#define DSRTOS_UART_BAUD_1500000             (1500000U)
#define DSRTOS_UART_BAUD_3000000             (3000000U)

/** @} */

/** @defgroup DSRTOS_UART_Errors UART Error Flags
 * @{
 */

/** Overrun error - data lost due to buffer overflow */
#define DSRTOS_UART_ERROR_OVERRUN            (0x01U)

/** Noise error - noise detected on line */
#define DSRTOS_UART_ERROR_NOISE              (0x02U)

/** Framing error - invalid stop bit */
#define DSRTOS_UART_ERROR_FRAMING            (0x04U)

/** Parity error - parity check failed */
#define DSRTOS_UART_ERROR_PARITY             (0x08U)

/** Break condition detected */
#define DSRTOS_UART_ERROR_BREAK              (0x10U)

/** @} */

/*==============================================================================
 * TYPE DEFINITIONS
 *==============================================================================*/

/**
 * @brief UART data bits configuration
 */
typedef enum {
    DSRTOS_UART_DATA_BITS_8 = 0,          /**< 8 data bits */
    DSRTOS_UART_DATA_BITS_9 = 1           /**< 9 data bits */
} dsrtos_uart_data_bits_t;

/**
 * @brief UART parity configuration
 */
typedef enum {
    DSRTOS_UART_PARITY_NONE = 0,          /**< No parity */
    DSRTOS_UART_PARITY_EVEN = 1,          /**< Even parity */
    DSRTOS_UART_PARITY_ODD = 2            /**< Odd parity */
} dsrtos_uart_parity_t;

/**
 * @brief UART stop bits configuration
 */
typedef enum {
    DSRTOS_UART_STOP_BITS_1 = 0,          /**< 1 stop bit */
    DSRTOS_UART_STOP_BITS_2 = 1           /**< 2 stop bits */
} dsrtos_uart_stop_bits_t;

/**
 * @brief UART flow control configuration
 */
typedef enum {
    DSRTOS_UART_FLOW_CONTROL_NONE = 0,    /**< No flow control */
    DSRTOS_UART_FLOW_CONTROL_RTS_CTS = 1  /**< RTS/CTS hardware flow control */
} dsrtos_uart_flow_control_t;

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint32_t baud_rate;                   /**< Baud rate in bps */
    dsrtos_uart_data_bits_t data_bits;    /**< Number of data bits */
    dsrtos_uart_parity_t parity;          /**< Parity setting */
    dsrtos_uart_stop_bits_t stop_bits;    /**< Number of stop bits */
    dsrtos_uart_flow_control_t flow_control; /**< Flow control setting */
    uint32_t tx_buffer_size;              /**< TX buffer size (0 = use default) */
    uint32_t rx_buffer_size;              /**< RX buffer size (0 = use default) */
} dsrtos_uart_config_t;

/**
 * @brief UART statistics structure
 */
typedef struct {
    uint32_t bytes_transmitted;           /**< Total bytes transmitted */
    uint32_t bytes_received;              /**< Total bytes received */
    uint32_t tx_interrupts;               /**< TX interrupt count */
    uint32_t rx_interrupts;               /**< RX interrupt count */
    uint32_t total_errors;                /**< Total error count */
    uint8_t last_error_flags;             /**< Last error flags */
    uint32_t tx_buffer_usage;             /**< Current TX buffer usage */
    uint32_t rx_buffer_usage;             /**< Current RX buffer usage */
    uint32_t tx_buffer_size;              /**< TX buffer total size */
    uint32_t rx_buffer_size;              /**< RX buffer total size */
} dsrtos_uart_stats_t;

/**
 * @brief UART callback function types
 */

/**
 * @brief TX complete callback function type
 * @param uart_id UART instance ID
 * @param context User context data
 */
typedef void (*dsrtos_uart_callback_t)(uint8_t uart_id, void* context);

/**
 * @brief RX data received callback function type  
 * @param data Received byte
 * @param context User context data
 */
typedef void (*dsrtos_uart_rx_callback_t)(uint8_t data, void* context);

/**
 * @brief Error callback function type
 * @param error_flags Error flags (combination of DSRTOS_UART_ERROR_*)
 * @param context User context data
 */
typedef void (*dsrtos_uart_error_callback_t)(uint8_t error_flags, void* context);

/**
 * @brief UART callback structure
 */
typedef struct {
    dsrtos_uart_callback_t tx_complete;      /**< TX complete callback */
    dsrtos_uart_rx_callback_t rx_received;   /**< RX data callback */  
    dsrtos_uart_error_callback_t error_occurred; /**< Error callback */
} dsrtos_uart_callbacks_t;

/**
 * @brief UART capabilities structure
 */
typedef struct {
    uint32_t min_baud_rate;               /**< Minimum supported baud rate */
    uint32_t max_baud_rate;               /**< Maximum supported baud rate */
    uint8_t max_instances;                /**< Maximum UART instances */
    bool hardware_flow_control;          /**< Hardware flow control support */
    bool dma_support;                     /**< DMA transfer support */
    uint32_t max_buffer_size;             /**< Maximum buffer size */
} dsrtos_uart_capabilities_t;

/*==============================================================================
 * FUNCTION PROTOTYPES
 *==============================================================================*/

/**
 * @defgroup DSRTOS_UART_Core Core UART Functions
 * @brief Core UART management functions
 * @{
 */

/**
 * @brief Initialize the DSRTOS UART subsystem
 * 
 * @details Initializes the UART controller, prepares instance structures,
 *          and sets up internal state. Must be called once during system
 *          initialization before any UART instances can be used.
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_ALREADY_INITIALIZED if already initialized
 * 
 * @pre Interrupt controller must be initialized
 * @post UART subsystem ready for instance configuration
 * 
 * @par Thread Safety
 * Not thread-safe. Must be called during single-threaded initialization.
 * 
 * @par MISRA-C:2012 Compliance
 * Full compliance with rules 8.7, 17.7, 21.1
 */
dsrtos_result_t dsrtos_uart_init(void);

/**
 * @brief Configure and open a UART instance
 * 
 * @details Configures UART hardware according to the provided configuration,
 *          sets up circular buffers, registers interrupt handlers, and
 *          enables the UART for communication.
 * 
 * @param[in] uart_id UART instance ID (0-5)
 * @param[in] config UART configuration parameters
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART subsystem not initialized
 * @return DSRTOS_ERR_NULL_POINTER if config is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid or config invalid
 * @return DSRTOS_ERR_ALREADY_INITIALIZED if UART already opened
 * @return DSRTOS_ERR_NOT_SUPPORTED if configuration not supported
 * @return DSRTOS_ERR_INTERRUPT_REGISTER if interrupt registration fails
 * 
 * @pre UART subsystem must be initialized
 * @post UART ready for communication
 * 
 * @par Thread Safety
 * Thread-safe with interrupt protection
 * 
 * @par Example
 * @code
 * dsrtos_uart_config_t config = {
 *     .baud_rate = DSRTOS_UART_BAUD_115200,
 *     .data_bits = DSRTOS_UART_DATA_BITS_8,
 *     .parity = DSRTOS_UART_PARITY_NONE,
 *     .stop_bits = DSRTOS_UART_STOP_BITS_1,
 *     .flow_control = DSRTOS_UART_FLOW_CONTROL_NONE,
 *     .tx_buffer_size = 0,  // Use default
 *     .rx_buffer_size = 0   // Use default
 * };
 * 
 * dsrtos_result_t result = dsrtos_uart_open(DSRTOS_UART1, &config);
 * if (result == DSRTOS_OK) {
 *     // UART1 ready for use
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_uart_open(uint8_t uart_id, const dsrtos_uart_config_t* config);

/**
 * @brief Close UART instance
 * 
 * @details Disables UART hardware, unregisters interrupt handlers,
 *          and frees associated resources.
 * 
 * @param[in] uart_id UART instance ID to close
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART not opened
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 * 
 * @post UART instance closed and resources freed
 */
dsrtos_result_t dsrtos_uart_close(uint8_t uart_id);

/** @} */

/**
 * @defgroup DSRTOS_UART_IO Input/Output Functions
 * @brief UART data transmission and reception
 * @{
 */

/**
 * @brief Transmit data via UART
 * 
 * @details Queues data for transmission in the TX circular buffer.
 *          Transmission occurs asynchronously via interrupts. Function
 *          returns immediately after queuing data.
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] data Data buffer to transmit
 * @param[in] length Number of bytes to transmit
 * @param[out] bytes_sent Pointer to store actual bytes queued (may be NULL)
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART not opened
 * @return DSRTOS_ERR_NULL_POINTER if data is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id or length invalid
 * 
 * @note Function is non-blocking - returns immediately
 * @note If TX buffer is full, only partial data may be queued
 * @note Use bytes_sent parameter to determine actual bytes queued
 * 
 * @par Thread Safety
 * Thread-safe with interrupt protection
 * 
 * @par Example
 * @code
 * const char* message = "Hello, World!\r\n";
 * uint32_t bytes_queued;
 * 
 * dsrtos_result_t result = dsrtos_uart_transmit(DSRTOS_UART1,
 *                                              (const uint8_t*)message,
 *                                              strlen(message),
 *                                              &bytes_queued);
 * 
 * if (result == DSRTOS_OK) {
 *     printf("Queued %u bytes for transmission\n", bytes_queued);
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_uart_transmit(uint8_t uart_id, const uint8_t* data, 
                                     uint32_t length, uint32_t* bytes_sent);

/**
 * @brief Receive data from UART
 * 
 * @details Retrieves available data from the RX circular buffer.
 *          Data is received asynchronously via interrupts and stored
 *          in the buffer. Function returns immediately with available data.
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] data Buffer to store received data
 * @param[in] length Maximum bytes to receive
 * @param[out] bytes_received Pointer to store actual bytes read (may be NULL)
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART not opened
 * @return DSRTOS_ERR_NULL_POINTER if data is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id or length invalid
 * 
 * @note Function is non-blocking - returns immediately
 * @note Returns only data currently available in buffer
 * @note Use bytes_received parameter to determine actual bytes read
 * 
 * @par Thread Safety
 * Thread-safe with interrupt protection
 * 
 * @par Example
 * @code
 * uint8_t rx_buffer[256];
 * uint32_t bytes_read;
 * 
 * dsrtos_result_t result = dsrtos_uart_receive(DSRTOS_UART1,
 *                                             rx_buffer,
 *                                             sizeof(rx_buffer),
 *                                             &bytes_read);
 * 
 * if (result == DSRTOS_OK && bytes_read > 0) {
 *     // Process received data
 *     process_received_data(rx_buffer, bytes_read);
 * }
 * @endcode
 */
dsrtos_result_t dsrtos_uart_receive(uint8_t uart_id, uint8_t* data, 
                                    uint32_t length, uint32_t* bytes_received);

/**
 * @brief Transmit single byte
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] byte Byte to transmit
 * 
 * @return DSRTOS_OK if byte queued successfully
 * @return DSRTOS_ERR_BUFFER_FULL if TX buffer is full
 * @return Other error codes same as dsrtos_uart_transmit
 */
dsrtos_result_t dsrtos_uart_put_byte(uint8_t uart_id, uint8_t byte);

/**
 * @brief Receive single byte
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] byte Pointer to store received byte
 * 
 * @return DSRTOS_OK if byte received
 * @return DSRTOS_ERR_NO_DATA if RX buffer is empty
 * @return Other error codes same as dsrtos_uart_receive
 */
dsrtos_result_t dsrtos_uart_get_byte(uint8_t uart_id, uint8_t* byte);

/**
 * @brief Transmit string
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] str Null-terminated string to transmit
 * 
 * @return DSRTOS_OK on success
 * @return Error codes same as dsrtos_uart_transmit
 * 
 * @note Convenience function for string transmission
 */
dsrtos_result_t dsrtos_uart_puts(uint8_t uart_id, const char* str);

/** @} */

/**
 * @defgroup DSRTOS_UART_Callbacks Callback Management
 * @brief UART event callback functions
 * @{
 */

/**
 * @brief Register UART event callbacks
 * 
 * @details Registers callback functions for UART events including
 *          TX completion, RX data received, and error conditions.
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] callbacks Callback function structure
 * @param[in] context User context passed to callbacks (may be NULL)
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART not opened
 * @return DSRTOS_ERR_NULL_POINTER if callbacks is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 * 
 * @note Callbacks are called from interrupt context
 * @note Callbacks should execute quickly (< 50μs recommended)
 * @note Any callback pointer may be NULL to disable that callback
 * 
 * @par Example
 * @code
 * void uart_rx_callback(uint8_t data, void* context) {
 *     // Handle received byte - keep it fast!
 *     received_byte = data;
 *     data_ready_flag = true;
 * }
 * 
 * void uart_error_callback(uint8_t error_flags, void* context) {
 *     if (error_flags & DSRTOS_UART_ERROR_OVERRUN) {
 *         overrun_count++;
 *     }
 * }
 * 
 * dsrtos_uart_callbacks_t callbacks = {
 *     .tx_complete = NULL,              // Not interested in TX complete
 *     .rx_received = uart_rx_callback,
 *     .error_occurred = uart_error_callback
 * };
 * 
 * dsrtos_uart_register_callbacks(DSRTOS_UART1, &callbacks, NULL);
 * @endcode
 */
dsrtos_result_t dsrtos_uart_register_callbacks(uint8_t uart_id,
                                               const dsrtos_uart_callbacks_t* callbacks,
                                               void* context);

/**
 * @brief Unregister UART callbacks
 * 
 * @param[in] uart_id UART instance ID
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_unregister_callbacks(uint8_t uart_id);

/** @} */

/**
 * @defgroup DSRTOS_UART_Status Status and Information
 * @brief UART status and monitoring functions
 * @{
 */

/**
 * @brief Get UART statistics
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] stats Pointer to statistics structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if stats is NULL
 * @return DSRTOS_ERR_NOT_INITIALIZED if UART not opened
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_get_stats(uint8_t uart_id, dsrtos_uart_stats_t* stats);

/**
 * @brief Reset UART statistics
 * 
 * @param[in] uart_id UART instance ID
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_reset_stats(uint8_t uart_id);

/**
 * @brief Check TX buffer available space
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] available_bytes Pointer to store available space
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if available_bytes is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_get_tx_space(uint8_t uart_id, uint32_t* available_bytes);

/**
 * @brief Check RX buffer data count
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] available_bytes Pointer to store available data count
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if available_bytes is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_get_rx_count(uint8_t uart_id, uint32_t* available_bytes);

/**
 * @brief Check if UART is busy transmitting
 * 
 * @param[in] uart_id UART instance ID
 * @param[out] is_busy Pointer to store busy status
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if is_busy is NULL
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_is_tx_busy(uint8_t uart_id, bool* is_busy);

/**
 * @brief Get UART capabilities
 * 
 * @param[out] caps Pointer to capabilities structure to fill
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_NULL_POINTER if caps is NULL
 */
dsrtos_result_t dsrtos_uart_get_capabilities(dsrtos_uart_capabilities_t* caps);

/** @} */

/**
 * @defgroup DSRTOS_UART_Control Control Functions
 * @brief UART control and configuration
 * @{
 */

/**
 * @brief Flush TX buffer
 * 
 * @details Waits for all pending TX data to be transmitted
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] timeout_ms Maximum time to wait in milliseconds
 * 
 * @return DSRTOS_OK if all data transmitted
 * @return DSRTOS_ERR_TIMEOUT if timeout occurred
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_flush_tx(uint8_t uart_id, uint32_t timeout_ms);

/**
 * @brief Clear RX buffer
 * 
 * @param[in] uart_id UART instance ID
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_clear_rx(uint8_t uart_id);

/**
 * @brief Enable/disable UART
 * 
 * @param[in] uart_id UART instance ID
 * @param[in] enable true to enable, false to disable
 * 
 * @return DSRTOS_OK on success
 * @return DSRTOS_ERR_INVALID_PARAM if uart_id invalid
 */
dsrtos_result_t dsrtos_uart_enable(uint8_t uart_id, bool enable);

/** @} */

/*==============================================================================
 * COMPILER ATTRIBUTES FOR CERTIFICATION
 *==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_UART_H */

/**
 * @defgroup DSRTOS_UART_Examples Usage Examples
 * @brief Practical usage examples
 * @{
 * 
 * @par Basic UART Communication
 * @code
 * // Initialize and configure UART1 for debug console
 * void setup_debug_console(void) {
 *     dsrtos_uart_config_t config = {
 *         .baud_rate = DSRTOS_UART_BAUD_115200,
 *         .data_bits = DSRTOS_UART_DATA_BITS_8,
 *         .parity = DSRTOS_UART_PARITY_NONE,
 *         .stop_bits = DSRTOS_UART_STOP_BITS_1,
 *         .flow_control = DSRTOS_UART_FLOW_CONTROL_NONE,
 *         .tx_buffer_size = 1024,
 *         .rx_buffer_size = 512
 *     };
 * 
 *     if (dsrtos_uart_open(DSRTOS_UART1, &config) == DSRTOS_OK) {
 *         dsrtos_uart_puts(DSRTOS_UART1, "Debug console ready\r\n");
 *     }
 * }
 * @endcode
 * 
 * @par Interrupt-Driven Data Processing
 * @code
 * static volatile bool command_ready = false;
 * static char command_buffer[128];
 * static uint8_t command_length = 0;
 * 
 * void uart_rx_handler(uint8_t data, void* context) {
 *     if (data == '\r' || data == '\n') {
 *         command_buffer[command_length] = '\0';
 *         command_ready = true;
 *     } else if (command_length < sizeof(command_buffer) - 1) {
 *         command_buffer[command_length++] = data;
 *     }
 * }
 * 
 * void setup_command_interface(void) {
 *     dsrtos_uart_callbacks_t callbacks = {
 *         .rx_received = uart_rx_handler,
 *         .tx_complete = NULL,
 *         .error_occurred = NULL
 *     };
 *     
 *     dsrtos_uart_register_callbacks(DSRTOS_UART2, &callbacks, NULL);
 * }
 * 
 * void process_commands(void) {
 *     if (command_ready) {
 *         handle_command(command_buffer);
 *         command_ready = false;
 *         command_length = 0;
 *     }
 * }
 * @endcode
 * 
 * @par Error Monitoring and Recovery
 * @code
 * void uart_error_handler(uint8_t error_flags, void* context) {
 *     if (error_flags & DSRTOS_UART_ERROR_OVERRUN) {
 *         // Clear RX buffer to recover from overrun
 *         dsrtos_uart_clear_rx(DSRTOS_UART1);
 *         error_log("UART overrun - data may be lost");
 *     }
 *     
 *     if (error_flags & DSRTOS_UART_ERROR_FRAMING) {
 *         error_log("UART framing error - check baud rate");
 *     }
 * }
 * 
 * void monitor_uart_health(void) {
 *     dsrtos_uart_stats_t stats;
 *     
 *     if (dsrtos_uart_get_stats(DSRTOS_UART1, &stats) == DSRTOS_OK) {
 *         // Check for high error rate
 *         if (stats.total_errors > 0) {
 *             float error_rate = (float)stats.total_errors / 
 *                               (stats.bytes_received + stats.bytes_transmitted);
 *             
 *             if (error_rate > 0.01f) {  // > 1% error rate
 *                 warning_log("High UART error rate: %.2f%%", error_rate * 100);
 *             }
 *         }
 *         
 *         // Check buffer usage
 *         if (stats.rx_buffer_usage > (stats.rx_buffer_size * 0.8f)) {
 *             warning_log("RX buffer nearly full - increase processing rate");
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
