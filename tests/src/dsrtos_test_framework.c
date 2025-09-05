/**
 * @file dsrtos_test_framework.c
 * @brief DSRTOS Test Framework Implementation - MISRA Compliant
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file implements the DSRTOS test framework following MISRA-C:2012 standards.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test framework magic number */
#define DSRTOS_TEST_MAGIC             0x54455354U  /* "TEST" */

/* Test framework state */
#define DSRTOS_TEST_STATE_UNINIT      0U
#define DSRTOS_TEST_STATE_INIT        1U
#define DSRTOS_TEST_STATE_RUNNING     2U
#define DSRTOS_TEST_STATE_COMPLETE    3U

/*==============================================================================
 *                              GLOBAL VARIABLES
 *============================================================================*/

/* Test framework state */
static uint32_t g_test_state = DSRTOS_TEST_STATE_UNINIT;
static dsrtos_test_config_t g_test_config;
static dsrtos_test_stats_t g_test_stats;
static dsrtos_test_suite_t *g_test_suites[DSRTOS_TEST_SUITES_MAX];
static uint32_t g_suite_count = 0U;
static dsrtos_test_suite_t *g_current_suite = NULL;

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Get current time in microseconds
 * @return Current time in microseconds
 */
static uint32_t get_time_us(void)
{
    /* This would be implemented using the system timer */
    /* For now, return a simple counter */
    static uint32_t time_counter = 0U;
    return ++time_counter;
}

/**
 * @brief Print test output
 * @param format Print format string
 * @param ... Variable arguments
 */
static void test_print(const char *format, ...)
{
    /* This would be implemented using UART or debug output */
    /* For now, it's a stub */
    (void)format;
}

/**
 * @brief Validate test framework state
 * @param expected_state Expected state
 * @return true if state is valid, false otherwise
 */
static bool validate_state(uint32_t expected_state)
{
    return (g_test_state == expected_state);
}

/**
 * @brief Find test suite by name
 * @param suite_name Suite name
 * @return Pointer to test suite or NULL if not found
 */
static dsrtos_test_suite_t *find_suite(const char *suite_name)
{
    uint32_t i;
    
    if (suite_name == NULL) {
        return NULL;
    }
    
    for (i = 0U; i < g_suite_count; i++) {
        if (g_test_suites[i] != NULL && 
            strcmp(g_test_suites[i]->name, suite_name) == 0) {
            return g_test_suites[i];
        }
    }
    
    return NULL;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

dsrtos_error_t dsrtos_test_framework_init(const dsrtos_test_config_t *config)
{
    if (config == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    if (!validate_state(DSRTOS_TEST_STATE_UNINIT)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Initialize configuration */
    (void)memcpy(&g_test_config, config, sizeof(dsrtos_test_config_t));
    
    /* Initialize statistics */
    (void)memset(&g_test_stats, 0U, sizeof(dsrtos_test_stats_t));
    
    /* Initialize suite array */
    (void)memset(g_test_suites, 0U, sizeof(g_test_suites));
    g_suite_count = 0U;
    g_current_suite = NULL;
    
    /* Set state */
    g_test_state = DSRTOS_TEST_STATE_INIT;
    
    test_print("DSRTOS Test Framework Initialized\n");
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_framework_deinit(void)
{
    if (!validate_state(DSRTOS_TEST_STATE_INIT)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Clear all data */
    (void)memset(&g_test_config, 0U, sizeof(dsrtos_test_config_t));
    (void)memset(&g_test_stats, 0U, sizeof(dsrtos_test_stats_t));
    (void)memset(g_test_suites, 0U, sizeof(g_test_suites));
    g_suite_count = 0U;
    g_current_suite = NULL;
    
    /* Set state */
    g_test_state = DSRTOS_TEST_STATE_UNINIT;
    
    test_print("DSRTOS Test Framework Deinitialized\n");
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_register_suite(dsrtos_test_suite_t *suite)
{
    if (suite == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    if (!validate_state(DSRTOS_TEST_STATE_INIT)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    if (g_suite_count >= DSRTOS_TEST_SUITES_MAX) {
        return DSRTOS_ERROR_LIMIT_EXCEEDED;
    }
    
    /* Add suite to array */
    g_test_suites[g_suite_count] = suite;
    g_suite_count++;
    
    test_print("Registered test suite: %s\n", suite->name);
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_run_all(void)
{
    uint32_t i;
    uint32_t j;
    dsrtos_test_suite_t *suite;
    dsrtos_test_case_t *test_case;
    uint32_t start_time;
    uint32_t end_time;
    
    if (!validate_state(DSRTOS_TEST_STATE_INIT)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    g_test_state = DSRTOS_TEST_STATE_RUNNING;
    
    test_print("Starting DSRTOS Test Suite Execution\n");
    test_print("=====================================\n");
    
    /* Reset statistics */
    (void)memset(&g_test_stats, 0U, sizeof(dsrtos_test_stats_t));
    
    start_time = get_time_us();
    
    /* Run all test suites */
    for (i = 0U; i < g_suite_count; i++) {
        suite = g_test_suites[i];
        if (suite == NULL || !suite->enabled) {
            continue;
        }
        
        test_print("\nRunning Test Suite: %s\n", suite->name);
        test_print("------------------------\n");
        
        /* Reset suite statistics */
        suite->passed_count = 0U;
        suite->failed_count = 0U;
        suite->skipped_count = 0U;
        suite->error_count = 0U;
        suite->total_time_us = 0U;
        
        uint32_t suite_start_time = get_time_us();
        
        /* Run all test cases in the suite */
        for (j = 0U; j < suite->test_count; j++) {
            test_case = &suite->test_cases[j];
            if (test_case == NULL || !test_case->enabled) {
                continue;
            }
            
            test_print("  Running: %s ... ", test_case->name);
            
            uint32_t test_start_time = get_time_us();
            dsrtos_test_result_t result = test_case->func();
            uint32_t test_end_time = get_time_us();
            
            test_case->result = result;
            test_case->execution_time_us = test_end_time - test_start_time;
            
            /* Update statistics */
            g_test_stats.total_tests++;
            suite->total_time_us += test_case->execution_time_us;
            
            switch (result) {
                case DSRTOS_TEST_RESULT_PASS:
                    test_print("PASS\n");
                    g_test_stats.passed_tests++;
                    suite->passed_count++;
                    break;
                    
                case DSRTOS_TEST_RESULT_FAIL:
                    test_print("FAIL\n");
                    g_test_stats.failed_tests++;
                    suite->failed_count++;
                    if (g_test_config.stop_on_failure) {
                        goto test_complete;
                    }
                    break;
                    
                case DSRTOS_TEST_RESULT_SKIP:
                    test_print("SKIP\n");
                    g_test_stats.skipped_tests++;
                    suite->skipped_count++;
                    break;
                    
                case DSRTOS_TEST_RESULT_ERROR:
                default:
                    test_print("ERROR\n");
                    g_test_stats.error_tests++;
                    suite->error_count++;
                    if (g_test_config.stop_on_failure) {
                        goto test_complete;
                    }
                    break;
            }
        }
        
        uint32_t suite_end_time = get_time_us();
        suite->total_time_us = suite_end_time - suite_start_time;
        
        test_print("Suite %s completed: %u passed, %u failed, %u skipped, %u errors\n",
                  suite->name, suite->passed_count, suite->failed_count, 
                  suite->skipped_count, suite->error_count);
    }
    
test_complete:
    end_time = get_time_us();
    g_test_stats.total_time_us = end_time - start_time;
    g_test_stats.total_suites = g_suite_count;
    
    g_test_state = DSRTOS_TEST_STATE_COMPLETE;
    
    /* Print final results */
    (void)dsrtos_test_print_results();
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_run_suite(const char *suite_name)
{
    dsrtos_test_suite_t *suite;
    uint32_t i;
    dsrtos_test_case_t *test_case;
    
    if (suite_name == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    if (!validate_state(DSRTOS_TEST_STATE_INIT)) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    suite = find_suite(suite_name);
    if (suite == NULL) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    if (!suite->enabled) {
        return DSRTOS_ERROR_NOT_ENABLED;
    }
    
    g_test_state = DSRTOS_TEST_STATE_RUNNING;
    g_current_suite = suite;
    
    test_print("Running Test Suite: %s\n", suite_name);
    test_print("------------------------\n");
    
    /* Reset suite statistics */
    suite->passed_count = 0U;
    suite->failed_count = 0U;
    suite->skipped_count = 0U;
    suite->error_count = 0U;
    suite->total_time_us = 0U;
    
    uint32_t suite_start_time = get_time_us();
    
    /* Run all test cases in the suite */
    for (i = 0U; i < suite->test_count; i++) {
        test_case = &suite->test_cases[i];
        if (test_case == NULL || !test_case->enabled) {
            continue;
        }
        
        test_print("  Running: %s ... ", test_case->name);
        
        uint32_t test_start_time = get_time_us();
        dsrtos_test_result_t result = test_case->func();
        uint32_t test_end_time = get_time_us();
        
        test_case->result = result;
        test_case->execution_time_us = test_end_time - test_start_time;
        
        /* Update statistics */
        suite->total_time_us += test_case->execution_time_us;
        
        switch (result) {
            case DSRTOS_TEST_RESULT_PASS:
                test_print("PASS\n");
                suite->passed_count++;
                break;
                
            case DSRTOS_TEST_RESULT_FAIL:
                test_print("FAIL\n");
                suite->failed_count++;
                if (g_test_config.stop_on_failure) {
                    goto suite_complete;
                }
                break;
                
            case DSRTOS_TEST_RESULT_SKIP:
                test_print("SKIP\n");
                suite->skipped_count++;
                break;
                
            case DSRTOS_TEST_RESULT_ERROR:
            default:
                test_print("ERROR\n");
                suite->error_count++;
                if (g_test_config.stop_on_failure) {
                    goto suite_complete;
                }
                break;
        }
    }
    
suite_complete:
    uint32_t suite_end_time = get_time_us();
    suite->total_time_us = suite_end_time - suite_start_time;
    
    test_print("Suite %s completed: %u passed, %u failed, %u skipped, %u errors\n",
              suite_name, suite->passed_count, suite->failed_count, 
              suite->skipped_count, suite->error_count);
    
    g_current_suite = NULL;
    g_test_state = DSRTOS_TEST_STATE_INIT;
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_get_stats(dsrtos_test_stats_t *stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    (void)memcpy(stats, &g_test_stats, sizeof(dsrtos_test_stats_t));
    
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_test_print_results(void)
{
    uint32_t i;
    dsrtos_test_suite_t *suite;
    
    test_print("\n");
    test_print("DSRTOS Test Results Summary\n");
    test_print("===========================\n");
    test_print("Total Suites: %u\n", g_test_stats.total_suites);
    test_print("Total Tests:  %u\n", g_test_stats.total_tests);
    test_print("Passed:       %u\n", g_test_stats.passed_tests);
    test_print("Failed:       %u\n", g_test_stats.failed_tests);
    test_print("Skipped:      %u\n", g_test_stats.skipped_tests);
    test_print("Errors:       %u\n", g_test_stats.error_tests);
    test_print("Total Time:   %u us\n", g_test_stats.total_time_us);
    test_print("\n");
    
    /* Print per-suite results */
    for (i = 0U; i < g_suite_count; i++) {
        suite = g_test_suites[i];
        if (suite == NULL) {
            continue;
        }
        
        test_print("Suite: %s\n", suite->name);
        test_print("  Tests: %u, Passed: %u, Failed: %u, Skipped: %u, Errors: %u\n",
                  suite->test_count, suite->passed_count, suite->failed_count,
                  suite->skipped_count, suite->error_count);
        test_print("  Time: %u us\n", suite->total_time_us);
        test_print("\n");
    }
    
    return DSRTOS_OK;
}

void dsrtos_test_suite_start(const char *suite_name)
{
    if (suite_name != NULL) {
        test_print("Starting Test Suite: %s\n", suite_name);
    }
}

void dsrtos_test_suite_end(void)
{
    test_print("Test Suite Completed\n");
}

void dsrtos_test_record_result(const char *test_name, dsrtos_test_result_t result)
{
    if (test_name != NULL && g_current_suite != NULL) {
        test_print("Test %s: %s\n", test_name, 
                  (result == DSRTOS_TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }
}

void dsrtos_test_fail(const char *file, uint32_t line, const char *condition)
{
    test_print("TEST FAILED: %s:%u - %s\n", file, line, condition);
}

void dsrtos_test_fail_eq(const char *file, uint32_t line, const char *actual_name, 
                        int32_t actual, int32_t expected)
{
    test_print("TEST FAILED: %s:%u - %s == %d, expected %d\n", 
              file, line, actual_name, actual, expected);
}

void dsrtos_test_fail_ne(const char *file, uint32_t line, const char *actual_name, 
                        int32_t actual, int32_t not_expected)
{
    test_print("TEST FAILED: %s:%u - %s != %d, but got %d\n", 
              file, line, actual_name, not_expected, actual);
}

void dsrtos_test_fail_null(const char *file, uint32_t line, const char *ptr_name, 
                          const void *ptr)
{
    test_print("TEST FAILED: %s:%u - %s is not NULL (0x%p)\n", 
              file, line, ptr_name, ptr);
}

void dsrtos_test_fail_not_null(const char *file, uint32_t line, const char *ptr_name)
{
    test_print("TEST FAILED: %s:%u - %s is NULL\n", file, line, ptr_name);
}

void dsrtos_test_fail_range(const char *file, uint32_t line, const char *value_name, 
                           int32_t value, int32_t min_val, int32_t max_val)
{
    test_print("TEST FAILED: %s:%u - %s = %d, not in range [%d, %d]\n", 
              file, line, value_name, value, min_val, max_val);
}

uint32_t dsrtos_test_get_time_us(void)
{
    return get_time_us();
}

void dsrtos_test_fill_pattern(void *ptr, size_t size, uint32_t pattern)
{
    uint32_t *word_ptr;
    uint8_t *byte_ptr;
    uint32_t i;
    
    if (ptr == NULL || size == 0U) {
        return;
    }
    
    /* Fill with pattern */
    word_ptr = (uint32_t *)ptr;
    for (i = 0U; i < (size / sizeof(uint32_t)); i++) {
        word_ptr[i] = pattern;
    }
    
    /* Fill remaining bytes */
    byte_ptr = (uint8_t *)ptr + (size - (size % sizeof(uint32_t)));
    for (i = 0U; i < (size % sizeof(uint32_t)); i++) {
        byte_ptr[i] = (uint8_t)(pattern >> (i * 8U));
    }
}

bool dsrtos_test_verify_pattern(const void *ptr, size_t size, uint32_t pattern)
{
    const uint32_t *word_ptr;
    const uint8_t *byte_ptr;
    uint32_t i;
    
    if (ptr == NULL || size == 0U) {
        return false;
    }
    
    /* Check word pattern */
    word_ptr = (const uint32_t *)ptr;
    for (i = 0U; i < (size / sizeof(uint32_t)); i++) {
        if (word_ptr[i] != pattern) {
            return false;
        }
    }
    
    /* Check remaining bytes */
    byte_ptr = (const uint8_t *)ptr + (size - (size % sizeof(uint32_t)));
    for (i = 0U; i < (size % sizeof(uint32_t)); i++) {
        if (byte_ptr[i] != (uint8_t)(pattern >> (i * 8U))) {
            return false;
        }
    }
    
    return true;
}

dsrtos_error_t dsrtos_test_main(void)
{
    dsrtos_test_config_t config;
    dsrtos_error_t result;
    
    /* Initialize test framework */
    config.verbose_output = true;
    config.stop_on_failure = false;
    config.enable_timing = true;
    config.enable_memory_check = true;
    config.default_timeout_us = 1000000U; /* 1 second */
    config.max_memory_usage = 1024U * 1024U; /* 1 MB */
    
    result = dsrtos_test_framework_init(&config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Run all tests */
    result = dsrtos_test_run_all();
    
    /* Deinitialize test framework */
    (void)dsrtos_test_framework_deinit();
    
    return result;
}

