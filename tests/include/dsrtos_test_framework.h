/**
 * @file dsrtos_test_framework.h
 * @brief DSRTOS Test Framework - MISRA Compliant Test Infrastructure
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file provides a comprehensive test framework for DSRTOS kernel
 * following MISRA-C:2012 standards and DO-178C DAL-B requirements.
 */

#ifndef DSRTOS_TEST_FRAMEWORK_H
#define DSRTOS_TEST_FRAMEWORK_H

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dsrtos_types.h"
#include "dsrtos_error.h"
#include "dsrtos_config.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test framework version */
#define DSRTOS_TEST_VERSION_MAJOR    1U
#define DSRTOS_TEST_VERSION_MINOR    0U
#define DSRTOS_TEST_VERSION_PATCH    0U

/* Test result codes */
#define DSRTOS_TEST_PASS             0U
#define DSRTOS_TEST_FAIL             1U
#define DSRTOS_TEST_SKIP             2U
#define DSRTOS_TEST_ERROR            3U

/* Test status macros */
#define DSRTOS_TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            dsrtos_test_fail(__FILE__, __LINE__, #condition); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

#define DSRTOS_TEST_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            dsrtos_test_fail_eq(__FILE__, __LINE__, #actual, (actual), (expected)); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

#define DSRTOS_TEST_ASSERT_NE(actual, not_expected) \
    do { \
        if ((actual) == (not_expected)) { \
            dsrtos_test_fail_ne(__FILE__, __LINE__, #actual, (actual), (not_expected)); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

#define DSRTOS_TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            dsrtos_test_fail_null(__FILE__, __LINE__, #ptr, (ptr)); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

#define DSRTOS_TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            dsrtos_test_fail_not_null(__FILE__, __LINE__, #ptr); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

#define DSRTOS_TEST_ASSERT_RANGE(value, min_val, max_val) \
    do { \
        if ((value) < (min_val) || (value) > (max_val)) { \
            dsrtos_test_fail_range(__FILE__, __LINE__, #value, (value), (min_val), (max_val)); \
            return DSRTOS_TEST_FAIL; \
        } \
    } while (0)

/* Test execution macros */
#define DSRTOS_TEST_RUN(test_func) \
    do { \
        dsrtos_test_result_t result = (test_func)(); \
        dsrtos_test_record_result(#test_func, result); \
    } while (0)

#define DSRTOS_TEST_SUITE_START(suite_name) \
    do { \
        dsrtos_test_suite_start(suite_name); \
    } while (0)

#define DSRTOS_TEST_SUITE_END() \
    do { \
        dsrtos_test_suite_end(); \
    } while (0)

/* Test timing macros */
#define DSRTOS_TEST_TIMEOUT_MS(timeout_ms) \
    ((timeout_ms) * 1000U)  /* Convert to microseconds */

/* Test memory patterns */
#define DSRTOS_TEST_PATTERN_1        0xAAAAAAAAU
#define DSRTOS_TEST_PATTERN_2        0x55555555U
#define DSRTOS_TEST_PATTERN_3        0x12345678U
#define DSRTOS_TEST_PATTERN_4        0x87654321U

/* Maximum test name length */
#define DSRTOS_TEST_NAME_MAX         64U

/* Maximum number of test cases per suite */
#define DSRTOS_TEST_CASES_MAX        256U

/* Maximum number of test suites */
#define DSRTOS_TEST_SUITES_MAX       16U

/*==============================================================================
 *                                 TYPEDEFS
 *============================================================================*/

/**
 * @brief Test result enumeration
 */
typedef enum {
    DSRTOS_TEST_RESULT_PASS = DSRTOS_TEST_PASS,
    DSRTOS_TEST_RESULT_FAIL = DSRTOS_TEST_FAIL,
    DSRTOS_TEST_RESULT_SKIP = DSRTOS_TEST_SKIP,
    DSRTOS_TEST_RESULT_ERROR = DSRTOS_TEST_ERROR
} dsrtos_test_result_t;

/**
 * @brief Test case function pointer type
 */
typedef dsrtos_test_result_t (*dsrtos_test_case_func_t)(void);

/**
 * @brief Test case structure
 */
typedef struct {
    const char *name;                           /* Test case name */
    dsrtos_test_case_func_t func;              /* Test function */
    uint32_t timeout_us;                       /* Timeout in microseconds */
    bool enabled;                              /* Test enabled flag */
    dsrtos_test_result_t result;               /* Test result */
    uint32_t execution_time_us;                /* Execution time */
    uint32_t line_number;                      /* Source line number */
    const char *file_name;                     /* Source file name */
} dsrtos_test_case_t;

/**
 * @brief Test suite structure
 */
typedef struct {
    const char *name;                           /* Suite name */
    dsrtos_test_case_t *test_cases;            /* Test cases array */
    uint32_t test_count;                       /* Number of test cases */
    uint32_t passed_count;                     /* Passed test count */
    uint32_t failed_count;                     /* Failed test count */
    uint32_t skipped_count;                    /* Skipped test count */
    uint32_t error_count;                      /* Error test count */
    uint32_t total_time_us;                    /* Total execution time */
    bool enabled;                              /* Suite enabled flag */
} dsrtos_test_suite_t;

/**
 * @brief Test framework statistics
 */
typedef struct {
    uint32_t total_suites;                     /* Total test suites */
    uint32_t total_tests;                      /* Total test cases */
    uint32_t passed_tests;                     /* Passed test cases */
    uint32_t failed_tests;                     /* Failed test cases */
    uint32_t skipped_tests;                    /* Skipped test cases */
    uint32_t error_tests;                      /* Error test cases */
    uint32_t total_time_us;                    /* Total execution time */
    uint32_t memory_used;                      /* Memory used by tests */
    uint32_t peak_memory;                      /* Peak memory usage */
} dsrtos_test_stats_t;

/**
 * @brief Test framework configuration
 */
typedef struct {
    bool verbose_output;                       /* Verbose output flag */
    bool stop_on_failure;                      /* Stop on first failure */
    bool enable_timing;                        /* Enable timing measurements */
    bool enable_memory_check;                  /* Enable memory checking */
    uint32_t default_timeout_us;               /* Default timeout */
    uint32_t max_memory_usage;                 /* Maximum memory usage */
} dsrtos_test_config_t;

/*==============================================================================
 *                            FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize the test framework
 * @param config Test framework configuration
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_framework_init(const dsrtos_test_config_t *config);

/**
 * @brief Deinitialize the test framework
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_framework_deinit(void);

/**
 * @brief Register a test suite
 * @param suite Test suite to register
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_register_suite(dsrtos_test_suite_t *suite);

/**
 * @brief Run all registered test suites
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_run_all(void);

/**
 * @brief Run a specific test suite
 * @param suite_name Name of the test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_run_suite(const char *suite_name);

/**
 * @brief Run a specific test case
 * @param suite_name Name of the test suite
 * @param test_name Name of the test case
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_run_case(const char *suite_name, const char *test_name);

/**
 * @brief Get test framework statistics
 * @param stats Pointer to statistics structure
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_get_stats(dsrtos_test_stats_t *stats);

/**
 * @brief Print test results
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_print_results(void);

/**
 * @brief Start a test suite
 * @param suite_name Name of the test suite
 */
void dsrtos_test_suite_start(const char *suite_name);

/**
 * @brief End a test suite
 */
void dsrtos_test_suite_end(void);

/**
 * @brief Record test result
 * @param test_name Name of the test case
 * @param result Test result
 */
void dsrtos_test_record_result(const char *test_name, dsrtos_test_result_t result);

/**
 * @brief Test failure handler
 * @param file Source file name
 * @param line Line number
 * @param condition Failed condition
 */
void dsrtos_test_fail(const char *file, uint32_t line, const char *condition);

/**
 * @brief Test failure handler for equality
 * @param file Source file name
 * @param line Line number
 * @param actual_name Actual value name
 * @param actual Actual value
 * @param expected Expected value
 */
void dsrtos_test_fail_eq(const char *file, uint32_t line, const char *actual_name, 
                        int32_t actual, int32_t expected);

/**
 * @brief Test failure handler for inequality
 * @param file Source file name
 * @param line Line number
 * @param actual_name Actual value name
 * @param actual Actual value
 * @param not_expected Not expected value
 */
void dsrtos_test_fail_ne(const char *file, uint32_t line, const char *actual_name, 
                        int32_t actual, int32_t not_expected);

/**
 * @brief Test failure handler for null pointer
 * @param file Source file name
 * @param line Line number
 * @param ptr_name Pointer name
 * @param ptr Pointer value
 */
void dsrtos_test_fail_null(const char *file, uint32_t line, const char *ptr_name, 
                          const void *ptr);

/**
 * @brief Test failure handler for not null pointer
 * @param file Source file name
 * @param line Line number
 * @param ptr_name Pointer name
 */
void dsrtos_test_fail_not_null(const char *file, uint32_t line, const char *ptr_name);

/**
 * @brief Test failure handler for range check
 * @param file Source file name
 * @param line Line number
 * @param value_name Value name
 * @param value Actual value
 * @param min_val Minimum value
 * @param max_val Maximum value
 */
void dsrtos_test_fail_range(const char *file, uint32_t line, const char *value_name, 
                           int32_t value, int32_t min_val, int32_t max_val);

/**
 * @brief Get current test execution time
 * @return Current time in microseconds
 */
uint32_t dsrtos_test_get_time_us(void);

/**
 * @brief Memory pattern fill
 * @param ptr Pointer to memory
 * @param size Size in bytes
 * @param pattern Pattern to fill
 */
void dsrtos_test_fill_pattern(void *ptr, size_t size, uint32_t pattern);

/**
 * @brief Memory pattern verify
 * @param ptr Pointer to memory
 * @param size Size in bytes
 * @param pattern Expected pattern
 * @return true if pattern matches, false otherwise
 */
bool dsrtos_test_verify_pattern(const void *ptr, size_t size, uint32_t pattern);

/**
 * @brief Test framework main entry point
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_main(void);

#endif /* DSRTOS_TEST_FRAMEWORK_H */

