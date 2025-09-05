/**
 * @file dsrtos_test_main.c
 * @brief DSRTOS Test Main Runner - Comprehensive Test Suite
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file provides the main test runner that integrates all phase test suites
 * following MISRA-C:2012 standards and DO-178C DAL-B requirements.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"
#include "dsrtos_test_phase1.h"
#include "dsrtos_test_phase2.h"
#include "dsrtos_test_phase3.h"
#include "dsrtos_boot.h"
#include "dsrtos_clock.h"
#include "dsrtos_interrupt.h"
#include "dsrtos_timer.h"
#include "dsrtos_uart.h"
#include "dsrtos_memory.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_scheduler.h"
#include "dsrtos_kernel.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test configuration */
#define DSRTOS_TEST_VERBOSE_OUTPUT      true
#define DSRTOS_TEST_STOP_ON_FAILURE     false
#define DSRTOS_TEST_ENABLE_TIMING       true
#define DSRTOS_TEST_ENABLE_MEMORY_CHECK true
#define DSRTOS_TEST_DEFAULT_TIMEOUT_MS  5000U
#define DSRTOS_TEST_MAX_MEMORY_USAGE    (1024U * 1024U) /* 1 MB */

/* Test phase selection */
#define DSRTOS_TEST_PHASE_1_ENABLED     true
#define DSRTOS_TEST_PHASE_2_ENABLED     true
#define DSRTOS_TEST_PHASE_3_ENABLED     true

/*==============================================================================
 *                            STATIC VARIABLES
 *============================================================================*/

/* Test framework configuration */
static dsrtos_test_config_t g_test_config;

/* Test statistics */
static dsrtos_test_stats_t g_test_stats;

/* Test phase status */
static bool g_phase1_initialized = false;
static bool g_phase2_initialized = false;
static bool g_phase3_initialized = false;

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize test configuration
 */
static void init_test_config(void)
{
    g_test_config.verbose_output = DSRTOS_TEST_VERBOSE_OUTPUT;
    g_test_config.stop_on_failure = DSRTOS_TEST_STOP_ON_FAILURE;
    g_test_config.enable_timing = DSRTOS_TEST_ENABLE_TIMING;
    g_test_config.enable_memory_check = DSRTOS_TEST_ENABLE_MEMORY_CHECK;
    g_test_config.default_timeout_us = DSRTOS_TEST_DEFAULT_TIMEOUT_MS * 1000U;
    g_test_config.max_memory_usage = DSRTOS_TEST_MAX_MEMORY_USAGE;
}

/**
 * @brief Initialize DSRTOS system for testing
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_error_t init_dsrtos_system(void)
{
    dsrtos_error_t result;
    
    /* Initialize boot system */
    result = dsrtos_boot_init();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize clock system */
    dsrtos_clock_config_t clock_config = {
        .source = DSRTOS_CLOCK_SOURCE_HSE,
        .frequency = 8000000U, /* 8 MHz */
        .pll_enabled = true,
        .pll_multiplier = 25U,
        .pll_divider = 2U
    };
    result = dsrtos_clock_init(&clock_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize interrupt system */
    result = dsrtos_interrupt_init();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize timer system */
    dsrtos_timer_config_t timer_config = {
        .timer_id = DSRTOS_TIMER_SYSTICK,
        .period_us = 1000U, /* 1ms */
        .callback = NULL,
        .auto_reload = true,
        .enabled = false
    };
    result = dsrtos_timer_init(&timer_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize UART system */
    dsrtos_uart_config_t uart_config = {
        .uart_id = DSRTOS_UART_1,
        .baud_rate = 115200U,
        .data_bits = DSRTOS_UART_DATA_BITS_8,
        .stop_bits = DSRTOS_UART_STOP_BITS_1,
        .parity = DSRTOS_UART_PARITY_NONE,
        .flow_control = DSRTOS_UART_FLOW_CONTROL_NONE,
        .tx_callback = NULL,
        .rx_callback = NULL
    };
    result = dsrtos_uart_init(&uart_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize memory system */
    dsrtos_memory_config_t memory_config = {
        .memory_start = 0x20000000U, /* SRAM start */
        .memory_size = 128U * 1024U, /* 128 KB */
        .alignment = 4U,
        .heap_enabled = true,
        .stack_enabled = true
    };
    result = dsrtos_memory_init(&memory_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize critical section system */
    result = dsrtos_critical_init();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize assertion system */
    result = dsrtos_assert_init();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize task manager */
    dsrtos_task_manager_config_t task_config = {
        .max_tasks = 16U,
        .max_priority = 15U,
        .stack_size = 512U,
        .enable_statistics = true,
        .enable_stack_check = true
    };
    result = dsrtos_task_manager_init(&task_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize scheduler */
    dsrtos_scheduler_config_t scheduler_config = {
        .type = DSRTOS_SCHEDULER_TYPE_PRIORITY,
        .max_tasks = 16U,
        .time_slice = 1000U, /* 1ms */
        .enable_preemption = true,
        .enable_round_robin = false
    };
    result = dsrtos_scheduler_init(&scheduler_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize kernel */
    result = dsrtos_kernel_init();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    return DSRTOS_OK;
}

/**
 * @brief Deinitialize DSRTOS system
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_error_t deinit_dsrtos_system(void)
{
    dsrtos_error_t result;
    
    /* Deinitialize kernel */
    result = dsrtos_kernel_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize scheduler */
    result = dsrtos_scheduler_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize task manager */
    result = dsrtos_task_manager_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize assertion system */
    result = dsrtos_assert_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize critical section system */
    result = dsrtos_critical_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize memory system */
    result = dsrtos_memory_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize UART system */
    result = dsrtos_uart_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize timer system */
    result = dsrtos_timer_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize interrupt system */
    result = dsrtos_interrupt_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize clock system */
    result = dsrtos_clock_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Deinitialize boot system */
    result = dsrtos_boot_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    return DSRTOS_OK;
}

/**
 * @brief Initialize test phases
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_error_t init_test_phases(void)
{
    dsrtos_error_t result;
    
    /* Initialize Phase 1 tests */
    if (DSRTOS_TEST_PHASE_1_ENABLED) {
        result = dsrtos_test_phase1_init();
        if (result != DSRTOS_OK) {
            return result;
        }
        g_phase1_initialized = true;
    }
    
    /* Initialize Phase 2 tests */
    if (DSRTOS_TEST_PHASE_2_ENABLED) {
        result = dsrtos_test_phase2_init();
        if (result != DSRTOS_OK) {
            return result;
        }
        g_phase2_initialized = true;
    }
    
    /* Initialize Phase 3 tests */
    if (DSRTOS_TEST_PHASE_3_ENABLED) {
        result = dsrtos_test_phase3_init();
        if (result != DSRTOS_OK) {
            return result;
        }
        g_phase3_initialized = true;
    }
    
    return DSRTOS_OK;
}

/**
 * @brief Print test banner
 */
static void print_test_banner(void)
{
    /* This would be implemented using UART or debug output */
    /* For now, it's a stub */
}

/**
 * @brief Print test results summary
 */
static void print_test_summary(void)
{
    /* This would be implemented using UART or debug output */
    /* For now, it's a stub */
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Main test entry point
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_main(void)
{
    dsrtos_error_t result;
    
    /* Print test banner */
    print_test_banner();
    
    /* Initialize test configuration */
    init_test_config();
    
    /* Initialize test framework */
    result = dsrtos_test_framework_init(&g_test_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize DSRTOS system */
    result = init_dsrtos_system();
    if (result != DSRTOS_OK) {
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Initialize test phases */
    result = init_test_phases();
    if (result != DSRTOS_OK) {
        (void)deinit_dsrtos_system();
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Run all test suites */
    result = dsrtos_test_run_all();
    if (result != DSRTOS_OK) {
        (void)deinit_dsrtos_system();
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Get test statistics */
    result = dsrtos_test_get_stats(&g_test_stats);
    if (result != DSRTOS_OK) {
        (void)deinit_dsrtos_system();
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Print test results */
    (void)dsrtos_test_print_results();
    print_test_summary();
    
    /* Deinitialize DSRTOS system */
    (void)deinit_dsrtos_system();
    
    /* Deinitialize test framework */
    (void)dsrtos_test_framework_deinit();
    
    /* Return success if all tests passed */
    if (g_test_stats.failed_tests == 0U && g_test_stats.error_tests == 0U) {
        return DSRTOS_OK;
    } else {
        return DSRTOS_ERROR_TEST_FAILED;
    }
}

/**
 * @brief Run specific phase tests
 * @param phase Phase number (1, 2, or 3)
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_run_phase(uint32_t phase)
{
    dsrtos_error_t result;
    const char *phase_name;
    
    /* Initialize test configuration */
    init_test_config();
    
    /* Initialize test framework */
    result = dsrtos_test_framework_init(&g_test_config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize DSRTOS system */
    result = init_dsrtos_system();
    if (result != DSRTOS_OK) {
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Initialize specific phase */
    switch (phase) {
        case 1U:
            if (DSRTOS_TEST_PHASE_1_ENABLED) {
                result = dsrtos_test_phase1_init();
                if (result != DSRTOS_OK) {
                    (void)deinit_dsrtos_system();
                    (void)dsrtos_test_framework_deinit();
                    return result;
                }
                phase_name = "Phase 1 - Boot, Clock, Interrupt, Timer, UART";
            } else {
                (void)deinit_dsrtos_system();
                (void)dsrtos_test_framework_deinit();
                return DSRTOS_ERROR_NOT_ENABLED;
            }
            break;
            
        case 2U:
            if (DSRTOS_TEST_PHASE_2_ENABLED) {
                result = dsrtos_test_phase2_init();
                if (result != DSRTOS_OK) {
                    (void)deinit_dsrtos_system();
                    (void)dsrtos_test_framework_deinit();
                    return result;
                }
                phase_name = "Phase 2 - Memory Management, Critical Sections, Assertions";
            } else {
                (void)deinit_dsrtos_system();
                (void)dsrtos_test_framework_deinit();
                return DSRTOS_ERROR_NOT_ENABLED;
            }
            break;
            
        case 3U:
            if (DSRTOS_TEST_PHASE_3_ENABLED) {
                result = dsrtos_test_phase3_init();
                if (result != DSRTOS_OK) {
                    (void)deinit_dsrtos_system();
                    (void)dsrtos_test_framework_deinit();
                    return result;
                }
                phase_name = "Phase 3 - Task Management, Scheduling, Context Switching";
            } else {
                (void)deinit_dsrtos_system();
                (void)dsrtos_test_framework_deinit();
                return DSRTOS_ERROR_NOT_ENABLED;
            }
            break;
            
        default:
            (void)deinit_dsrtos_system();
            (void)dsrtos_test_framework_deinit();
            return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    /* Run specific phase tests */
    result = dsrtos_test_run_suite(phase_name);
    if (result != DSRTOS_OK) {
        (void)deinit_dsrtos_system();
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Print test results */
    (void)dsrtos_test_print_results();
    
    /* Deinitialize DSRTOS system */
    (void)deinit_dsrtos_system();
    
    /* Deinitialize test framework */
    (void)dsrtos_test_framework_deinit();
    
    return DSRTOS_OK;
}

/**
 * @brief Get test statistics
 * @param stats Pointer to statistics structure
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_get_statistics(dsrtos_test_stats_t *stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    (void)memcpy(stats, &g_test_stats, sizeof(dsrtos_test_stats_t));
    
    return DSRTOS_OK;
}

/**
 * @brief Check if all tests passed
 * @return true if all tests passed, false otherwise
 */
bool dsrtos_test_all_passed(void)
{
    return (g_test_stats.failed_tests == 0U && g_test_stats.error_tests == 0U);
}

/**
 * @brief Get test pass rate
 * @return Test pass rate as percentage (0-100)
 */
uint32_t dsrtos_test_get_pass_rate(void)
{
    if (g_test_stats.total_tests == 0U) {
        return 0U;
    }
    
    return (g_test_stats.passed_tests * 100U) / g_test_stats.total_tests;
}
