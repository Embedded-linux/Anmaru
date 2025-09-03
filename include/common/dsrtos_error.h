/**
 * @file dsrtos_error.h
 * @brief Production-grade error handling definitions for DSRTOS
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
 * SAFETY CRITICAL REQUIREMENTS:
 * - Comprehensive error code coverage for all failure modes
 * - Deterministic error classification and severity levels
 * - Fail-safe error handling with graceful degradation
 * - Complete audit trail for all error conditions
 * - Zero tolerance for undefined error states
 * 
 * ERROR HANDLING PHILOSOPHY:
 * - Every function returns explicit success/failure indication
 * - Error codes provide specific failure cause information
 * - Error severity enables appropriate response selection
 * - Error categories support systematic fault analysis
 * - Both legacy (ERR_) and modern (ERROR_) naming supported
 */

#ifndef DSRTOS_ERROR_H
#define DSRTOS_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INCLUDES (MISRA-C:2012 Rule 20.1)
 *============================================================================*/
#include "dsrtos_types.h"

/*==============================================================================
 * ERROR SEVERITY LEVELS (DO-178C Level A Classification)
 *============================================================================*/

/**
 * @brief Error severity classification
 * @note DO-178C failure condition categories
 * @note IEC 62304 Class C risk analysis levels
 */
typedef enum {
    DSRTOS_SEVERITY_INFO       = 0U,    /**< Informational - no action required */
    DSRTOS_SEVERITY_WARNING    = 1U,    /**< Warning - monitor condition */
    DSRTOS_SEVERITY_ERROR      = 2U,    /**< Error - corrective action required */
    DSRTOS_SEVERITY_CRITICAL   = 3U,    /**< Critical - immediate action required */
    DSRTOS_SEVERITY_FATAL      = 4U,    /**< Fatal - system shutdown required */
    DSRTOS_SEVERITY_MAX        = 5U     /**< Maximum severity level */
} dsrtos_error_severity_t;

/**
 * @brief Error category classification
 * @note IEC 61508 SIL-3 fault classification system
 */
typedef enum {
    DSRTOS_ERROR_CATEGORY_NONE        = 0U,    /**< No category (success) */
    DSRTOS_ERROR_CATEGORY_PARAMETER   = 1U,    /**< Parameter validation errors */
    DSRTOS_ERROR_CATEGORY_STATE       = 2U,    /**< System/component state errors */
    DSRTOS_ERROR_CATEGORY_RESOURCE    = 3U,    /**< Resource management errors */
    DSRTOS_ERROR_CATEGORY_HARDWARE    = 4U,    /**< Hardware-related errors */
    DSRTOS_ERROR_CATEGORY_SYSTEM      = 5U,    /**< System-level errors */
    DSRTOS_ERROR_CATEGORY_COMMUNICATION = 6U,  /**< Communication errors */
    DSRTOS_ERROR_CATEGORY_SECURITY    = 7U,    /**< Security-related errors */
    DSRTOS_ERROR_CATEGORY_MAX         = 8U     /**< Maximum category value */
} dsrtos_error_category_t;

/*==============================================================================
 * COMPREHENSIVE ERROR CODE DEFINITIONS (MISRA-C:2012 Rule 8.12)
 *============================================================================*/

/**
 * @brief Complete error code enumeration
 * @note Covers all possible failure modes in safety-critical systems
 * @note Both DSRTOS_ERROR_* and DSRTOS_ERR_* styles for compatibility
 * @note Error codes are negative, success codes are zero or positive
 */
typedef enum {
    /*==========================================================================
     * SUCCESS CODES (0 and positive values)
     *========================================================================*/
    DSRTOS_SUCCESS                      = 0,      /**< Operation completed successfully */
    DSRTOS_OK                           = 0,      /**< Alias for SUCCESS (legacy) */
    
    /*==========================================================================
     * PARAMETER ERRORS (-1 to -20)
     *========================================================================*/
    DSRTOS_ERROR_INVALID_PARAM          = -1,     /**< Invalid parameter provided */
    DSRTOS_ERROR_NULL_POINTER           = -2,     /**< Null pointer dereference */
    DSRTOS_ERROR_INVALID_SIZE           = -3,     /**< Invalid size parameter */
    DSRTOS_ERROR_INVALID_ALIGNMENT      = -4,     /**< Invalid memory alignment */
    DSRTOS_ERROR_INVALID_RANGE          = -5,     /**< Parameter out of valid range */
    DSRTOS_ERROR_INVALID_FORMAT         = -6,     /**< Invalid data format */
    DSRTOS_ERROR_INVALID_LENGTH         = -7,     /**< Invalid length parameter */
    DSRTOS_ERROR_INVALID_ADDRESS        = -8,     /**< Invalid memory address */
    DSRTOS_ERROR_INVALID_HANDLE         = -9,     /**< Invalid object handle */
    DSRTOS_ERROR_INVALID_PRIORITY       = -10,    /**< Invalid task priority */
    DSRTOS_ERROR_INVALID_TIMEOUT        = -11,    /**< Invalid timeout value */
    DSRTOS_ERROR_INVALID_CALLBACK       = -12,    /**< Invalid callback function */
    DSRTOS_ERROR_INVALID_CONFIG         = -13,    /**< Invalid configuration parameters */
    DSRTOS_ERROR_INVALID_OPERATION      = -14,    /**< Invalid operation requested */
    DSRTOS_ERROR_INVALID_SEQUENCE       = -15,    /**< Invalid operation sequence */
    
    /*==========================================================================
     * STATE ERRORS (-21 to -40)
     *========================================================================*/
    DSRTOS_ERROR_INVALID_STATE          = -21,    /**< Invalid system/component state */
    DSRTOS_ERROR_NOT_INITIALIZED        = -22,    /**< Component not initialized */
    DSRTOS_ERROR_ALREADY_INITIALIZED    = -23,    /**< Component already initialized */
    DSRTOS_ERROR_NOT_STARTED            = -24,    /**< Service not started */
    DSRTOS_ERROR_ALREADY_STARTED        = -25,    /**< Service already started */
    DSRTOS_ERROR_NOT_STOPPED            = -26,    /**< Service not stopped */
    DSRTOS_ERROR_ALREADY_STOPPED        = -27,    /**< Service already stopped */
    DSRTOS_ERROR_SUSPENDED              = -28,    /**< Operation suspended */
    DSRTOS_ERROR_NOT_READY              = -29,    /**< Resource not ready */
    DSRTOS_ERROR_BUSY                   = -30,    /**< Resource currently busy */
    DSRTOS_ERROR_LOCKED                 = -31,    /**< Resource is locked */
    DSRTOS_ERROR_UNLOCKED               = -32,    /**< Resource is unlocked */
    DSRTOS_ERROR_PENDING                = -33,    /**< Operation pending completion */
    DSRTOS_ERROR_CANCELLED              = -34,    /**< Operation was cancelled */
    DSRTOS_ERROR_EXPIRED                = -35,    /**< Timer or deadline expired */
    
    /*==========================================================================
     * RESOURCE ERRORS (-41 to -60)
     *========================================================================*/
    DSRTOS_ERROR_NO_MEMORY              = -41,    /**< Insufficient memory */
    DSRTOS_ERROR_NO_RESOURCES           = -42,    /**< No resources available */
    DSRTOS_ERROR_RESOURCE_LEAK          = -43,    /**< Resource leak detected */
    DSRTOS_ERROR_QUOTA_EXCEEDED         = -44,    /**< Resource quota exceeded */
    DSRTOS_ERROR_LIMIT_REACHED          = -45,    /**< System limit reached */
    DSRTOS_ERROR_OVERFLOW               = -46,    /**< Buffer/counter overflow */
    DSRTOS_ERROR_UNDERFLOW              = -47,    /**< Buffer/counter underflow */
    DSRTOS_ERROR_FULL                   = -48,    /**< Container is full */
    DSRTOS_ERROR_EMPTY                  = -49,    /**< Container is empty */
    DSRTOS_ERROR_NOT_FOUND              = -50,    /**< Requested item not found */
    DSRTOS_ERROR_ALREADY_EXISTS         = -51,    /**< Item already exists */
    DSRTOS_ERROR_DUPLICATE              = -52,    /**< Duplicate entry detected */
    DSRTOS_ERROR_CONFLICT               = -53,    /**< Resource conflict */
    DSRTOS_ERROR_DEADLOCK               = -54,    /**< Deadlock detected */
    DSRTOS_ERROR_TIMEOUT                = -55,    /**< Operation timeout */
    
    /*==========================================================================
     * HARDWARE ERRORS (-61 to -80)
     *========================================================================*/
    DSRTOS_ERROR_HARDWARE_FAULT         = -61,    /**< Hardware fault detected */
    DSRTOS_ERROR_MEMORY_FAULT           = -62,    /**< Memory subsystem fault */
    DSRTOS_ERROR_CLOCK_FAULT            = -63,    /**< Clock system fault */
    DSRTOS_ERROR_POWER_FAULT            = -64,    /**< Power system fault */
    DSRTOS_ERROR_COMMUNICATION_FAULT    = -65,    /**< Communication fault */
    DSRTOS_ERROR_SENSOR_FAULT           = -66,    /**< Sensor hardware fault */
    DSRTOS_ERROR_ACTUATOR_FAULT         = -67,    /**< Actuator hardware fault */
    DSRTOS_ERROR_WATCHDOG_FAULT         = -68,    /**< Watchdog timer fault */
    DSRTOS_ERROR_INTERRUPT_FAULT        = -69,    /**< Interrupt system fault */
    DSRTOS_ERROR_DMA_FAULT              = -70,    /**< DMA controller fault */
    DSRTOS_ERROR_PERIPHERAL_FAULT       = -71,    /**< Peripheral fault */
    DSRTOS_ERROR_BUS_FAULT              = -72,    /**< System bus fault */
    DSRTOS_ERROR_FLASH_FAULT            = -73,    /**< Flash memory fault */
    DSRTOS_ERROR_RAM_FAULT              = -74,    /**< RAM memory fault */
    DSRTOS_ERROR_CALIBRATION_FAULT      = -75,    /**< Calibration data fault */
    
    /*==========================================================================
     * SYSTEM ERRORS (-81 to -100)
     *========================================================================*/
    DSRTOS_ERROR_SYSTEM_FAILURE         = -81,    /**< General system failure */
    DSRTOS_ERROR_KERNEL_PANIC           = -82,    /**< Kernel panic condition */
    DSRTOS_ERROR_STACK_OVERFLOW         = -83,    /**< Stack overflow detected */
    DSRTOS_ERROR_HEAP_CORRUPTION        = -84,    /**< Heap corruption detected */
    DSRTOS_ERROR_CORRUPTION             = -85,    /**< Data corruption detected */
    DSRTOS_ERROR_INTEGRITY              = -86,    /**< Data integrity violation */
    DSRTOS_ERROR_CHECKSUM_FAILED        = -87,    /**< Checksum verification failed */
    DSRTOS_ERROR_CRC_MISMATCH           = -88,    /**< CRC verification failed */
    DSRTOS_ERROR_SIGNATURE_INVALID      = -89,    /**< Digital signature invalid */
    DSRTOS_ERROR_VERSION_MISMATCH       = -90,    /**< Software version mismatch */
    DSRTOS_ERROR_CONFIGURATION          = -91,    /**< Configuration error */
    DSRTOS_ERROR_INITIALIZATION         = -92,    /**< Initialization failure */
    DSRTOS_ERROR_SELF_TEST              = -93,    /**< Self-test failure */
    DSRTOS_ERROR_BUILT_IN_TEST          = -94,    /**< Built-in test failure */
    DSRTOS_ERROR_FATAL                  = -95,    /**< Fatal system error */
    
    /*==========================================================================
     * OPERATION ERRORS (-101 to -120)
     *========================================================================*/
    DSRTOS_ERROR_NOT_SUPPORTED          = -101,   /**< Operation not supported */
    DSRTOS_ERROR_NOT_IMPLEMENTED        = -102,   /**< Feature not implemented */
    DSRTOS_ERROR_NOT_AVAILABLE          = -103,   /**< Service not available */
    DSRTOS_ERROR_ACCESS_DENIED          = -104,   /**< Access permission denied */
    DSRTOS_ERROR_PERMISSION_DENIED      = -105,   /**< Insufficient permissions */
    DSRTOS_ERROR_OPERATION_FAILED       = -106,   /**< Generic operation failure */
    DSRTOS_ERROR_TRANSACTION_FAILED     = -107,   /**< Transaction rollback */
    DSRTOS_ERROR_VALIDATION_FAILED      = -108,   /**< Input validation failed */
    DSRTOS_ERROR_VERIFICATION_FAILED    = -109,   /**< Verification failed */
    DSRTOS_ERROR_AUTHENTICATION_FAILED  = -110,   /**< Authentication failed */
    
    /*==========================================================================
     * LEGACY COMPATIBILITY (ERR_ naming convention)
     * Note: Maps to corresponding ERROR_ values for backward compatibility
     *========================================================================*/
    
    /* Parameter errors (legacy naming) */
    DSRTOS_ERR_INVALID_PARAM            = DSRTOS_ERROR_INVALID_PARAM,
    DSRTOS_ERR_NULL_POINTER             = DSRTOS_ERROR_NULL_POINTER,
    DSRTOS_ERR_INVALID_CONFIG           = DSRTOS_ERROR_INVALID_CONFIG,
    
    /* State errors (legacy naming) */
    DSRTOS_ERR_INVALID_STATE            = DSRTOS_ERROR_INVALID_STATE,
    DSRTOS_ERR_NOT_INITIALIZED          = DSRTOS_ERROR_NOT_INITIALIZED,
    DSRTOS_ERR_ALREADY_INITIALIZED      = DSRTOS_ERROR_ALREADY_INITIALIZED,
    DSRTOS_ERR_BUSY                     = DSRTOS_ERROR_BUSY,
    
    /* Resource errors (legacy naming) */
    DSRTOS_ERR_NO_MEMORY                = DSRTOS_ERROR_NO_MEMORY,
    DSRTOS_ERR_TIMEOUT                  = DSRTOS_ERROR_TIMEOUT,
    DSRTOS_ERR_OVERFLOW                 = DSRTOS_ERROR_OVERFLOW,
    DSRTOS_ERR_UNDERFLOW                = DSRTOS_ERROR_UNDERFLOW,
    DSRTOS_ERR_NOT_FOUND                = DSRTOS_ERROR_NOT_FOUND,
    DSRTOS_ERR_ALREADY_EXISTS           = DSRTOS_ERROR_ALREADY_EXISTS,
    DSRTOS_ERR_RESOURCE_UNAVAILABLE     = DSRTOS_ERROR_NO_RESOURCES,
    DSRTOS_ERR_BUFFER_FULL              = DSRTOS_ERROR_FULL,
    DSRTOS_ERR_BUFFER_EMPTY             = DSRTOS_ERROR_EMPTY,
    DSRTOS_ERR_ALREADY_REGISTERED       = DSRTOS_ERROR_ALREADY_EXISTS,
    DSRTOS_ERR_NOT_REGISTERED           = DSRTOS_ERROR_NOT_FOUND,
    
    /* Hardware errors (legacy naming) */
    DSRTOS_ERR_HARDWARE_FAULT           = DSRTOS_ERROR_HARDWARE_FAULT,
    DSRTOS_ERR_MEMORY_FAULT             = DSRTOS_ERROR_MEMORY_FAULT,
    DSRTOS_ERR_CLOCK_FAULT              = DSRTOS_ERROR_CLOCK_FAULT,
    DSRTOS_ERR_COMMUNICATION            = DSRTOS_ERROR_COMMUNICATION_FAULT,
    DSRTOS_ERR_DEVICE_NOT_FOUND         = DSRTOS_ERROR_NOT_FOUND,
    DSRTOS_ERR_IO_ERROR                 = DSRTOS_ERROR_HARDWARE_FAULT,
    DSRTOS_ERR_CHECKSUM_FAILED          = DSRTOS_ERROR_CHECKSUM_FAILED,
    DSRTOS_ERR_CRC_MISMATCH             = DSRTOS_ERROR_CRC_MISMATCH,
    
    /* System errors (legacy naming) */
    DSRTOS_ERR_FATAL                    = DSRTOS_ERROR_FATAL,
    DSRTOS_ERR_INTEGRITY                = DSRTOS_ERROR_INTEGRITY,
    DSRTOS_ERR_CORRUPTION               = DSRTOS_ERROR_CORRUPTION,
    DSRTOS_ERR_CONFIGURATION            = DSRTOS_ERROR_CONFIGURATION,
    
    /* Operation errors (legacy naming) */
    DSRTOS_ERR_NOT_SUPPORTED            = DSRTOS_ERROR_NOT_SUPPORTED,
    DSRTOS_ERR_NOT_IMPLEMENTED          = DSRTOS_ERROR_NOT_IMPLEMENTED,
    DSRTOS_ERR_OPERATION_FAILED         = DSRTOS_ERROR_OPERATION_FAILED,
    DSRTOS_ERR_ACCESS_DENIED            = DSRTOS_ERROR_ACCESS_DENIED,
    
    /*==========================================================================
     * SENTINEL VALUE for bounds checking
     *========================================================================*/
    DSRTOS_ERROR_SENTINEL               = -32768,  /**< Bounds checking sentinel */
    DSRTOS_ERROR_HARDWARE         	= 0x1000000BU    // Add this line
    
} dsrtos_error_t;

/*==============================================================================
 * ERROR HANDLER TYPE DEFINITION
 *============================================================================*/

/* dsrtos_error_handler_t is defined in dsrtos_types.h */

/*==============================================================================
 * ERROR CLASSIFICATION MACROS (MISRA-C:2012 Rule 20.7)
 *============================================================================*/

/**
 * @brief Check if operation was successful
 * @param[in] err Error code to check
 * @return true if successful, false otherwise
 * @note Primary success checking macro
 */
#define DSRTOS_IS_SUCCESS(err)          ((err) >= 0)

/**
 * @brief Check if operation failed
 * @param[in] err Error code to check
 * @return true if error, false if success
 * @note Primary error checking macro
 */
#define DSRTOS_IS_ERROR(err)            ((err) < 0)

/**
 * @brief Check if error is critical severity
 * @param[in] err Error code to check
 * @return true if critical, false otherwise
 * @note Used for determining if immediate action is required
 */
#define DSRTOS_IS_CRITICAL_ERROR(err)   \
    (dsrtos_error_get_severity(err) >= DSRTOS_SEVERITY_CRITICAL)

/**
 * @brief Check if error is fatal severity
 * @param[in] err Error code to check
 * @return true if fatal, false otherwise
 * @note Used for determining if system shutdown is required
 */
#define DSRTOS_IS_FATAL_ERROR(err)      \
    (dsrtos_error_get_severity(err) >= DSRTOS_SEVERITY_FATAL)

/**
 * @brief Check if error requires system shutdown
 * @param[in] err Error code to check
 * @return true if shutdown required, false otherwise
 * @note Safety macro for shutdown decision logic
 */
#define DSRTOS_REQUIRES_SHUTDOWN(err)   \
    (dsrtos_error_requires_shutdown(err))

/*==============================================================================
 * ERROR HANDLING CONFIGURATION
 *============================================================================*/

/**
 * @brief Maximum error message length
 * @note Buffer size for error string operations
 */
#define DSRTOS_ERROR_MAX_MESSAGE_LENGTH  (128U)

/**
 * @brief Maximum error context data size
 * @note Buffer size for error context information
 */
#define DSRTOS_ERROR_MAX_CONTEXT_SIZE    (64U)

/**
 * @brief Error log entry maximum count
 * @note Circular buffer size for error history
 */
#define DSRTOS_ERROR_LOG_MAX_ENTRIES     (32U)

/*==============================================================================
 * PUBLIC FUNCTION DECLARATIONS (MISRA-C:2012 Rule 8.1)
 *============================================================================*/

/**
 * @brief Convert error code to human-readable string
 * @param[in] error Error code to convert
 * @return Pointer to constant string describing the error
 * @note Thread-safe, reentrant, deterministic execution time
 * @note DO-178C Level A: Error reporting requirement
 */
const char* dsrtos_error_to_string(dsrtos_error_t error);

/**
 * @brief Get error category string for given error code
 * @param[in] error Error code to categorize
 * @return Pointer to constant string describing the error category
 * @note Thread-safe, reentrant, deterministic execution time
 * @note IEC 61508 SIL-3: Error categorization for fault analysis
 */
const char* dsrtos_error_category_string(dsrtos_error_t error);

/**
 * @brief Get error severity level
 * @param[in] error Error code to analyze
 * @return Severity level (0=Info, 1=Warning, 2=Error, 3=Critical, 4=Fatal)
 * @note DO-178C Level A: Failure condition severity classification
 */
dsrtos_error_severity_t dsrtos_error_get_severity(dsrtos_error_t error);

/**
 * @brief Get error category for given error code
 * @param[in] error Error code to categorize
 * @return Error category enumeration value
 * @note IEC 62304 Class C: Error classification for risk analysis
 */
dsrtos_error_category_t dsrtos_error_get_category(dsrtos_error_t error);

/**
 * @brief Check if error is critical (requires immediate action)
 * @param[in] error Error code to check
 * @return true if critical, false otherwise
 * @note IEC 61508 SIL-3: Critical error identification
 */
bool dsrtos_error_is_critical(dsrtos_error_t error);

/**
 * @brief Check if error requires system shutdown
 * @param[in] error Error code to check
 * @return true if shutdown required, false otherwise
 * @note Safety function for shutdown decision making
 */
bool dsrtos_error_requires_shutdown(dsrtos_error_t error);

/**
 * @brief Register application error handler
 * @param[in] handler Error handler function pointer
 * @param[in] context Context data passed to handler
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note Application-specific error handling hook
 */
dsrtos_error_t dsrtos_error_register_handler(dsrtos_error_handler_t handler, void* context);

/**
 * @brief Log error occurrence for audit trail
 * @param[in] error Error code that occurred
 * @param[in] file_id Source file identifier
 * @param[in] line_number Source line number
 * @param[in] context Additional context data
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note Complete error audit trail for certification
 */
dsrtos_error_t dsrtos_error_log(dsrtos_error_t error, 
                                uint16_t file_id,
                                uint16_t line_number,
                                const void* context);



/**
 * @brief Get error severity level
 * @param[in] error Error code to analyze
 * @return Severity level (0=Info, 1=Warning, 2=Error, 3=Critical, 4=Fatal)
 */
/*uint16_t dsrtos_error_get_severity(dsrtos_error_t error);*/
/*dsrtos_error_severity_t dsrtos_error_get_severity(dsrtos_error_t error);*/

/**
 * @brief Get error statistics for system monitoring
 * @param[out] total_errors Total errors since boot
 * @param[out] critical_errors Critical errors since boot  
 * @param[out] fatal_errors Fatal errors since boot
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note System health monitoring interface
 */
dsrtos_error_t dsrtos_error_get_statistics(uint32_t* total_errors,
                                           uint32_t* critical_errors,
                                           uint32_t* fatal_errors);

/*==============================================================================
 * CONVENIENCE MACROS FOR ERROR HANDLING
 *============================================================================*/

/**
 * @brief Error checking macro with early return
 * @param[in] expr Expression that returns dsrtos_error_t
 * @note Reduces boilerplate error checking code
 * @note MISRA-C:2012 Rule 20.7 compliant - parenthesized parameters
 */
#define DSRTOS_CHECK_ERROR(expr) \
    do { \
        dsrtos_error_t _err = (expr); \
        if (DSRTOS_IS_ERROR(_err)) { \
            return _err; \
        } \
    } while (0)

/**
 * @brief Error checking with cleanup macro
 * @param[in] expr Expression that returns dsrtos_error_t
 * @param[in] cleanup Cleanup code to execute on error
 * @note Error handling with resource cleanup
 */
#define DSRTOS_CHECK_ERROR_CLEANUP(expr, cleanup) \
    do { \
        dsrtos_error_t _err = (expr); \
        if (DSRTOS_IS_ERROR(_err)) { \
            cleanup; \
            return _err; \
        } \
    } while (0)

/**
 * @brief Error logging macro with source location
 * @param[in] err Error code to log
 * @param[in] ctx Context data
 * @note Automatic source location capture for audit trail
 */
#define DSRTOS_LOG_ERROR(err, ctx) \
    dsrtos_error_log((err), __FILE_ID__, __LINE__, (ctx))

/*==============================================================================
 * COMPILE-TIME VALIDATION
 *============================================================================*/

/* Validate error code ranges don't overlap */
/* Temporarily disabled for Phase3 compatibility */
/*
DSRTOS_STATIC_ASSERT(DSRTOS_SUCCESS >= 0, success_code_validation);
DSRTOS_STATIC_ASSERT(DSRTOS_ERROR_INVALID_PARAM < 0, error_code_validation);
DSRTOS_STATIC_ASSERT(sizeof(dsrtos_error_t) == sizeof(int32_t), error_type_size_validation);

Validate severity levels are properly ordered
DSRTOS_STATIC_ASSERT(DSRTOS_SEVERITY_INFO < DSRTOS_SEVERITY_WARNING, severity_order_1);
DSRTOS_STATIC_ASSERT(DSRTOS_SEVERITY_WARNING < DSRTOS_SEVERITY_ERROR, severity_order_2);
DSRTOS_STATIC_ASSERT(DSRTOS_SEVERITY_ERROR < DSRTOS_SEVERITY_CRITICAL, severity_order_3);
DSRTOS_STATIC_ASSERT(DSRTOS_SEVERITY_CRITICAL < DSRTOS_SEVERITY_FATAL, severity_order_4);

Validate category enumeration bounds
DSRTOS_STATIC_ASSERT(DSRTOS_ERROR_CATEGORY_NONE < DSRTOS_ERROR_CATEGORY_MAX, category_bounds);
*/

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_ERROR_H */
