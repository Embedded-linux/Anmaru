/**
 * @file dsrtos_test_minimal.c
 * @brief DSRTOS Minimal Test Suite for Target Board Testing
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file contains minimal test cases for target board validation
 * focusing on core functionality with reduced complexity.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"
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

/* Minimal test configuration */
#define MINIMAL_TEST_TIMEOUT_MS      500U
#define MINIMAL_TASK_STACK_SIZE      256U
#define MINIMAL_TEST_ITERATIONS      10U

/* Test task names */
#define MINIMAL_TASK_NAME_1          "MinTask1"
#define MINIMAL_TASK_NAME_2          "MinTask2"

/*==============================================================================
 *                            STATIC VARIABLES
 *============================================================================*/

/* Test suite instance */
static dsrtos_test_suite_t g_minimal_test_suite;

/* Test case array - Minimal set */
static dsrtos_test_case_t g_minimal_test_cases[] = {
    {"test_system_init", test_system_init, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_clock_basic", test_clock_basic, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_memory_basic", test_memory_basic, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_critical_basic", test_critical_basic, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_create", test_task_create, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_scheduler_basic", test_scheduler_basic, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_kernel_basic", test_kernel_basic, DSRTOS_TEST_TIMEOUT_MS(MINIMAL_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL}
};

/* Test task handles */
static dsrtos_task_handle_t g_minimal_task_1 = NULL;
static dsrtos_task_handle_t g_minimal_task_2 = NULL;

/* Test counters */
static volatile uint32_t g_minimal_task_count = 0U;
static volatile bool g_minimal_test_running = false;

/* Test task stacks */
static uint8_t g_minimal_task_1_stack[MINIMAL_TASK_STACK_SIZE];
static uint8_t g_minimal_task_2_stack[MINIMAL_TASK_STACK_SIZE];

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Minimal test task 1 function
 * @param param Task parameter
 */
static void minimal_task_1_func(void *param)
{
    (void)param;
    
    g_minimal_test_running = true;
    
    while (g_minimal_test_running) {
        g_minimal_task_count++;
        dsrtos_task_yield();
        
        /* Simple delay */
        for (volatile uint32_t i = 0U; i < 1000U; i++) {
            __asm volatile ("nop");
        }
    }
}

/**
 * @brief Minimal test task 2 function
 * @param param Task parameter
 */
static void minimal_task_2_func(void *param)
{
    (void)param;
    
    while (g_minimal_test_running) {
        g_minimal_task_count++;
        dsrtos_task_yield();
        
        /* Simple delay */
        for (volatile uint32_t i = 0U; i < 1000U; i++) {
            __asm volatile ("nop");
        }
    }
}

/*==============================================================================
 *                            TEST FUNCTIONS
 *============================================================================*/

/**
 * @brief Test system initialization
 * @return Test result
 */
dsrtos_test_result_t test_system_init(void)
{
    dsrtos_error_t result;
    
    /* Test boot initialization */
    result = dsrtos_boot_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test clock initialization */
    dsrtos_clock_config_t clock_config = {
        .source = DSRTOS_CLOCK_SOURCE_HSE,
        .frequency = 8000000U,
        .pll_enabled = true,
        .pll_multiplier = 25U,
        .pll_divider = 2U
    };
    result = dsrtos_clock_init(&clock_config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test interrupt initialization */
    result = dsrtos_interrupt_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test memory initialization */
    dsrtos_memory_config_t memory_config = {
        .memory_start = 0x20000000U,
        .memory_size = 64U * 1024U, /* 64 KB */
        .alignment = 4U,
        .heap_enabled = true,
        .stack_enabled = true
    };
    result = dsrtos_memory_init(&memory_config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic clock functionality
 * @return Test result
 */
dsrtos_test_result_t test_clock_basic(void)
{
    uint32_t frequency;
    
    /* Test system clock frequency */
    frequency = dsrtos_clock_get_system_frequency();
    DSRTOS_TEST_ASSERT_NE(frequency, 0U);
    
    /* Test clock source */
    dsrtos_clock_source_t source = dsrtos_clock_get_source();
    DSRTOS_TEST_ASSERT_EQ(source, DSRTOS_CLOCK_SOURCE_PLL);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic memory functionality
 * @return Test result
 */
dsrtos_test_result_t test_memory_basic(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 64U;
    
    /* Test memory allocation */
    result = dsrtos_memory_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    /* Test memory write */
    uint32_t *test_ptr = (uint32_t *)ptr;
    test_ptr[0] = 0x12345678U;
    DSRTOS_TEST_ASSERT_EQ(test_ptr[0], 0x12345678U);
    
    /* Test memory deallocation */
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic critical section functionality
 * @return Test result
 */
dsrtos_test_result_t test_critical_basic(void)
{
    dsrtos_error_t result;
    uint32_t i;
    
    /* Test critical section enter/exit */
    for (i = 0U; i < MINIMAL_TEST_ITERATIONS; i++) {
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Simple operation in critical section */
        volatile uint32_t test_var = 0x12345678U;
        test_var++;
        
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    }
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic task creation
 * @return Test result
 */
dsrtos_test_result_t test_task_create(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    
    /* Initialize task manager */
    dsrtos_task_manager_config_t task_config = {
        .max_tasks = 8U,
        .max_priority = 7U,
        .stack_size = MINIMAL_TASK_STACK_SIZE,
        .enable_statistics = true,
        .enable_stack_check = true
    };
    result = dsrtos_task_manager_init(&task_config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Create test task 1 */
    config.name = MINIMAL_TASK_NAME_1;
    config.function = minimal_task_1_func;
    config.parameter = NULL;
    config.priority = 5U;
    config.stack_size = MINIMAL_TASK_STACK_SIZE;
    config.stack_pointer = g_minimal_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &g_minimal_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(g_minimal_task_1);
    
    /* Create test task 2 */
    config.name = MINIMAL_TASK_NAME_2;
    config.function = minimal_task_2_func;
    config.parameter = NULL;
    config.priority = 3U;
    config.stack_size = MINIMAL_TASK_STACK_SIZE;
    config.stack_pointer = g_minimal_task_2_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &g_minimal_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(g_minimal_task_2);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic scheduler functionality
 * @return Test result
 */
dsrtos_test_result_t test_scheduler_basic(void)
{
    dsrtos_error_t result;
    
    /* Initialize scheduler */
    dsrtos_scheduler_config_t scheduler_config = {
        .type = DSRTOS_SCHEDULER_TYPE_PRIORITY,
        .max_tasks = 8U,
        .time_slice = 1000U, /* 1ms */
        .enable_preemption = true,
        .enable_round_robin = false
    };
    result = dsrtos_scheduler_init(&scheduler_config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start scheduler */
    result = dsrtos_scheduler_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start tasks */
    result = dsrtos_task_start(g_minimal_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_start(g_minimal_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Let tasks run for a short time */
    for (volatile uint32_t i = 0U; i < 10000U; i++) {
        __asm volatile ("nop");
    }
    
    /* Stop tasks */
    result = dsrtos_task_stop(g_minimal_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_stop(g_minimal_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Stop scheduler */
    result = dsrtos_scheduler_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Verify tasks executed */
    DSRTOS_TEST_ASSERT_NE(g_minimal_task_count, 0U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test basic kernel functionality
 * @return Test result
 */
dsrtos_test_result_t test_kernel_basic(void)
{
    dsrtos_error_t result;
    
    /* Initialize kernel */
    result = dsrtos_kernel_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test kernel state */
    dsrtos_kernel_state_t state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_READY);
    
    /* Start kernel */
    result = dsrtos_kernel_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_RUNNING);
    
    /* Let kernel run briefly */
    for (volatile uint32_t i = 0U; i < 5000U; i++) {
        __asm volatile ("nop");
    }
    
    /* Stop kernel */
    result = dsrtos_kernel_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_STOPPED);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize minimal test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_minimal_init(void)
{
    /* Initialize test suite */
    g_minimal_test_suite.name = "Minimal DSRTOS Test Suite";
    g_minimal_test_suite.test_cases = g_minimal_test_cases;
    g_minimal_test_suite.test_count = sizeof(g_minimal_test_cases) / sizeof(g_minimal_test_cases[0]);
    g_minimal_test_suite.passed_count = 0U;
    g_minimal_test_suite.failed_count = 0U;
    g_minimal_test_suite.skipped_count = 0U;
    g_minimal_test_suite.error_count = 0U;
    g_minimal_test_suite.total_time_us = 0U;
    g_minimal_test_suite.enabled = true;
    
    /* Register test suite */
    return dsrtos_test_register_suite(&g_minimal_test_suite);
}

/**
 * @brief Minimal test main entry point
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_minimal_main(void)
{
    dsrtos_test_config_t config;
    dsrtos_error_t result;
    
    /* Initialize test framework */
    config.verbose_output = true;
    config.stop_on_failure = true;  /* Stop on first failure for minimal testing */
    config.enable_timing = true;
    config.enable_memory_check = true;
    config.default_timeout_us = MINIMAL_TEST_TIMEOUT_MS * 1000U;
    config.max_memory_usage = 32U * 1024U; /* 32 KB */
    
    result = dsrtos_test_framework_init(&config);
    if (result != DSRTOS_OK) {
        return result;
    }
    
    /* Initialize minimal test suite */
    result = dsrtos_test_minimal_init();
    if (result != DSRTOS_OK) {
        (void)dsrtos_test_framework_deinit();
        return result;
    }
    
    /* Run minimal tests */
    result = dsrtos_test_run_all();
    
    /* Print results */
    (void)dsrtos_test_print_results();
    
    /* Deinitialize test framework */
    (void)dsrtos_test_framework_deinit();
    
    return result;
}
