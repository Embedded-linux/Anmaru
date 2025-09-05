/**
 * @file dsrtos_test_phase3.c
 * @brief DSRTOS Phase 3 Test Cases - Task Management, Scheduling, Context Switching
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file contains comprehensive test cases for Phase 3 functionality
 * following MISRA-C:2012 standards and DO-178C DAL-B requirements.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_scheduler.h"
#include "dsrtos_context_switch.h"
#include "dsrtos_stack_manager.h"
#include "dsrtos_kernel.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test timeouts */
#define PHASE3_TEST_TIMEOUT_MS       2000U
#define PHASE3_TASK_STACK_SIZE      512U
#define PHASE3_TASK_PRIORITY_HIGH   10U
#define PHASE3_TASK_PRIORITY_MED    5U
#define PHASE3_TASK_PRIORITY_LOW    1U

/* Test iterations */
#define PHASE3_TASK_TEST_ITER       100U
#define PHASE3_SCHEDULER_TEST_ITER  50U
#define PHASE3_CONTEXT_TEST_ITER    25U

/* Test task names */
#define PHASE3_TASK_NAME_1          "TestTask1"
#define PHASE3_TASK_NAME_2          "TestTask2"
#define PHASE3_TASK_NAME_3          "TestTask3"

/*==============================================================================
 *                            STATIC VARIABLES
 *============================================================================*/

/* Test suite instance */
static dsrtos_test_suite_t g_phase3_test_suite;

/* Test case array */
static dsrtos_test_case_t g_phase3_test_cases[] = {
    {"test_task_manager_initialization", test_task_manager_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_creation", test_task_creation, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_deletion", test_task_deletion, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_suspend_resume", test_task_suspend_resume, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_priority", test_task_priority, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_state_transitions", test_task_state_transitions, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_scheduler_initialization", test_scheduler_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_scheduler_start_stop", test_scheduler_start_stop, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_scheduler_yield", test_scheduler_yield, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_scheduler_priority", test_scheduler_priority, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_context_switch", test_context_switch, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_context_save_restore", test_context_save_restore, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_stack_management", test_stack_management, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_queue_operations", test_task_queue_operations, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_task_statistics", test_task_statistics, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_kernel_operations", test_kernel_operations, DSRTOS_TEST_TIMEOUT_MS(PHASE3_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL}
};

/* Test task handles */
static dsrtos_task_handle_t g_test_task_1 = NULL;
static dsrtos_task_handle_t g_test_task_2 = NULL;
static dsrtos_task_handle_t g_test_task_3 = NULL;

/* Test counters */
static volatile uint32_t g_task_1_count = 0U;
static volatile uint32_t g_task_2_count = 0U;
static volatile uint32_t g_task_3_count = 0U;
static volatile uint32_t g_context_switch_count = 0U;
static volatile bool g_task_1_running = false;
static volatile bool g_task_2_running = false;
static volatile bool g_task_3_running = false;

/* Test task stacks */
static uint8_t g_task_1_stack[PHASE3_TASK_STACK_SIZE];
static uint8_t g_task_2_stack[PHASE3_TASK_STACK_SIZE];
static uint8_t g_task_3_stack[PHASE3_TASK_STACK_SIZE];

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Test task 1 function
 * @param param Task parameter
 */
static void test_task_1_func(void *param)
{
    (void)param;
    
    g_task_1_running = true;
    
    while (g_task_1_running) {
        g_task_1_count++;
        dsrtos_task_yield();
    }
}

/**
 * @brief Test task 2 function
 * @param param Task parameter
 */
static void test_task_2_func(void *param)
{
    (void)param;
    
    g_task_2_running = true;
    
    while (g_task_2_running) {
        g_task_2_count++;
        dsrtos_task_yield();
    }
}

/**
 * @brief Test task 3 function
 * @param param Task parameter
 */
static void test_task_3_func(void *param)
{
    (void)param;
    
    g_task_3_running = true;
    
    while (g_task_3_running) {
        g_task_3_count++;
        dsrtos_task_yield();
    }
}

/**
 * @brief Test context switch callback
 * @param from_task Source task handle
 * @param to_task Destination task handle
 */
static void test_context_switch_callback(dsrtos_task_handle_t from_task, dsrtos_task_handle_t to_task)
{
    (void)from_task;
    (void)to_task;
    
    g_context_switch_count++;
}

/*==============================================================================
 *                            TEST FUNCTIONS
 *============================================================================*/

/**
 * @brief Test task manager initialization
 * @return Test result
 */
dsrtos_test_result_t test_task_manager_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_task_manager_config_t config;
    
    /* Configure task manager */
    config.max_tasks = 16U;
    config.max_priority = 15U;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.enable_statistics = true;
    config.enable_stack_check = true;
    
    /* Test task manager initialization */
    result = dsrtos_task_manager_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task manager state */
    dsrtos_task_manager_state_t state = dsrtos_task_manager_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_MANAGER_STATE_READY);
    
    /* Test task manager statistics */
    dsrtos_task_manager_stats_t stats;
    result = dsrtos_task_manager_get_stats(&stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(stats.max_tasks, 16U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task creation
 * @return Test result
 */
dsrtos_test_result_t test_task_creation(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    
    /* Configure task 1 */
    config.name = PHASE3_TASK_NAME_1;
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_HIGH;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    /* Test task creation */
    result = dsrtos_task_create(&config, &g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(g_test_task_1);
    
    /* Test task state */
    dsrtos_task_state_t state = dsrtos_task_get_state(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_READY);
    
    /* Test task priority */
    uint32_t priority = dsrtos_task_get_priority(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(priority, PHASE3_TASK_PRIORITY_HIGH);
    
    /* Test task name */
    const char *name = dsrtos_task_get_name(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(strcmp(name, PHASE3_TASK_NAME_1), 0);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task deletion
 * @return Test result
 */
dsrtos_test_result_t test_task_deletion(void)
{
    dsrtos_error_t result;
    
    /* Test task deletion */
    result = dsrtos_task_delete(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task state after deletion */
    dsrtos_task_state_t state = dsrtos_task_get_state(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_DELETED);
    
    /* Test deletion of already deleted task */
    result = dsrtos_task_delete(g_test_task_1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_STATE);
    
    /* Test deletion of NULL task */
    result = dsrtos_task_delete(NULL);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task suspend/resume
 * @return Test result
 */
dsrtos_test_result_t test_task_suspend_resume(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    
    /* Create test task */
    config.name = PHASE3_TASK_NAME_2;
    config.function = test_task_2_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_MED;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_2_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task suspend */
    result = dsrtos_task_suspend(g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task state after suspend */
    dsrtos_task_state_t state = dsrtos_task_get_state(g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_SUSPENDED);
    
    /* Test task resume */
    result = dsrtos_task_resume(g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task state after resume */
    state = dsrtos_task_get_state(g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_READY);
    
    /* Clean up */
    result = dsrtos_task_delete(g_test_task_2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task priority
 * @return Test result
 */
dsrtos_test_result_t test_task_priority(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    uint32_t priority;
    
    /* Create test task */
    config.name = PHASE3_TASK_NAME_3;
    config.function = test_task_3_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_LOW;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_3_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &g_test_task_3);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test initial priority */
    priority = dsrtos_task_get_priority(g_test_task_3);
    DSRTOS_TEST_ASSERT_EQ(priority, PHASE3_TASK_PRIORITY_LOW);
    
    /* Test priority change */
    result = dsrtos_task_set_priority(g_test_task_3, PHASE3_TASK_PRIORITY_HIGH);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    priority = dsrtos_task_get_priority(g_test_task_3);
    DSRTOS_TEST_ASSERT_EQ(priority, PHASE3_TASK_PRIORITY_HIGH);
    
    /* Test invalid priority */
    result = dsrtos_task_set_priority(g_test_task_3, 16U);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    /* Clean up */
    result = dsrtos_task_delete(g_test_task_3);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task state transitions
 * @return Test result
 */
dsrtos_test_result_t test_task_state_transitions(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    dsrtos_task_handle_t task;
    dsrtos_task_state_t state;
    
    /* Create test task */
    config.name = "StateTestTask";
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_MED;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test initial state */
    state = dsrtos_task_get_state(task);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_READY);
    
    /* Test suspend state */
    result = dsrtos_task_suspend(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    state = dsrtos_task_get_state(task);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_SUSPENDED);
    
    /* Test resume state */
    result = dsrtos_task_resume(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    state = dsrtos_task_get_state(task);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_READY);
    
    /* Test running state */
    result = dsrtos_task_start(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    state = dsrtos_task_get_state(task);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_RUNNING);
    
    /* Test stop state */
    result = dsrtos_task_stop(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    state = dsrtos_task_get_state(task);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TASK_STATE_READY);
    
    /* Clean up */
    result = dsrtos_task_delete(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test scheduler initialization
 * @return Test result
 */
dsrtos_test_result_t test_scheduler_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_scheduler_config_t config;
    
    /* Configure scheduler */
    config.type = DSRTOS_SCHEDULER_TYPE_PRIORITY;
    config.max_tasks = 16U;
    config.time_slice = 1000U; /* 1ms */
    config.enable_preemption = true;
    config.enable_round_robin = false;
    
    /* Test scheduler initialization */
    result = dsrtos_scheduler_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test scheduler state */
    dsrtos_scheduler_state_t state = dsrtos_scheduler_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_SCHEDULER_STATE_READY);
    
    /* Test scheduler type */
    dsrtos_scheduler_type_t type = dsrtos_scheduler_get_type();
    DSRTOS_TEST_ASSERT_EQ(type, DSRTOS_SCHEDULER_TYPE_PRIORITY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test scheduler start/stop
 * @return Test result
 */
dsrtos_test_result_t test_scheduler_start_stop(void)
{
    dsrtos_error_t result;
    
    /* Test scheduler start */
    result = dsrtos_scheduler_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test scheduler state */
    dsrtos_scheduler_state_t state = dsrtos_scheduler_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_SCHEDULER_STATE_RUNNING);
    
    /* Test scheduler stop */
    result = dsrtos_scheduler_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test scheduler state */
    state = dsrtos_scheduler_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_SCHEDULER_STATE_STOPPED);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test scheduler yield
 * @return Test result
 */
dsrtos_test_result_t test_scheduler_yield(void)
{
    dsrtos_error_t result;
    uint32_t i;
    
    /* Start scheduler */
    result = dsrtos_scheduler_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test scheduler yield */
    for (i = 0U; i < PHASE3_SCHEDULER_TEST_ITER; i++) {
        result = dsrtos_scheduler_yield();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    }
    
    /* Stop scheduler */
    result = dsrtos_scheduler_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test scheduler priority
 * @return Test result
 */
dsrtos_test_result_t test_scheduler_priority(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    dsrtos_task_handle_t high_task, low_task;
    
    /* Create high priority task */
    config.name = "HighPriorityTask";
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_HIGH;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &high_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Create low priority task */
    config.name = "LowPriorityTask";
    config.function = test_task_2_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_LOW;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_2_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &low_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start scheduler */
    result = dsrtos_scheduler_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start tasks */
    result = dsrtos_task_start(high_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_start(low_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Wait for some execution */
    for (uint32_t i = 0U; i < 1000U; i++) {
        __asm volatile ("nop");
    }
    
    /* Stop tasks */
    result = dsrtos_task_stop(high_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_stop(low_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Stop scheduler */
    result = dsrtos_scheduler_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Clean up */
    result = dsrtos_task_delete(high_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_delete(low_task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test context switch
 * @return Test result
 */
dsrtos_test_result_t test_context_switch(void)
{
    dsrtos_error_t result;
    dsrtos_task_config_t config;
    dsrtos_task_handle_t task1, task2;
    uint32_t initial_count;
    
    /* Reset counter */
    g_context_switch_count = 0U;
    
    /* Create test tasks */
    config.name = "ContextTask1";
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_HIGH;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &task1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    config.name = "ContextTask2";
    config.function = test_task_2_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_MED;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_2_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &task2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Register context switch callback */
    result = dsrtos_context_switch_register_callback(test_context_switch_callback);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start scheduler */
    result = dsrtos_scheduler_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Start tasks */
    result = dsrtos_task_start(task1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_start(task2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Wait for context switches */
    initial_count = g_context_switch_count;
    for (uint32_t i = 0U; i < 1000U; i++) {
        if (g_context_switch_count > initial_count) {
            break;
        }
        __asm volatile ("nop");
    }
    
    /* Verify context switches occurred */
    DSRTOS_TEST_ASSERT_NE(g_context_switch_count, initial_count);
    
    /* Stop tasks */
    result = dsrtos_task_stop(task1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_stop(task2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Stop scheduler */
    result = dsrtos_scheduler_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Clean up */
    result = dsrtos_task_delete(task1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_task_delete(task2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test context save/restore
 * @return Test result
 */
dsrtos_test_result_t test_context_save_restore(void)
{
    dsrtos_error_t result;
    dsrtos_cpu_context_t context;
    uint32_t test_value = 0x12345678U;
    uint32_t *test_ptr = &test_value;
    
    /* Test context save */
    result = dsrtos_context_save(&context);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Modify some registers */
    test_value = 0x87654321U;
    
    /* Test context restore */
    result = dsrtos_context_restore(&context);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Verify context was restored */
    DSRTOS_TEST_ASSERT_EQ(test_value, 0x12345678U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test stack management
 * @return Test result
 */
dsrtos_test_result_t test_stack_management(void)
{
    dsrtos_error_t result;
    dsrtos_stack_config_t config;
    dsrtos_stack_handle_t stack;
    
    /* Configure stack */
    config.size = PHASE3_TASK_STACK_SIZE;
    config.alignment = 4U;
    config.overflow_protection = true;
    config.underflow_protection = true;
    
    /* Test stack creation */
    result = dsrtos_stack_create(&config, &stack);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(stack);
    
    /* Test stack size */
    uint32_t size = dsrtos_stack_get_size(stack);
    DSRTOS_TEST_ASSERT_EQ(size, PHASE3_TASK_STACK_SIZE);
    
    /* Test stack usage */
    uint32_t usage = dsrtos_stack_get_usage(stack);
    DSRTOS_TEST_ASSERT_EQ(usage, 0U);
    
    /* Test stack overflow check */
    bool overflow = dsrtos_stack_check_overflow(stack);
    DSRTOS_TEST_ASSERT_EQ(overflow, false);
    
    /* Test stack underflow check */
    bool underflow = dsrtos_stack_check_underflow(stack);
    DSRTOS_TEST_ASSERT_EQ(underflow, false);
    
    /* Test stack destruction */
    result = dsrtos_stack_destroy(stack);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task queue operations
 * @return Test result
 */
dsrtos_test_result_t test_task_queue_operations(void)
{
    dsrtos_error_t result;
    dsrtos_task_handle_t task;
    dsrtos_task_config_t config;
    
    /* Create test task */
    config.name = "QueueTestTask";
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_MED;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task queue add */
    result = dsrtos_task_queue_add(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task queue remove */
    result = dsrtos_task_queue_remove(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task queue add again */
    result = dsrtos_task_queue_add(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task queue get next */
    dsrtos_task_handle_t next_task = dsrtos_task_queue_get_next();
    DSRTOS_TEST_ASSERT_EQ(next_task, task);
    
    /* Clean up */
    result = dsrtos_task_delete(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test task statistics
 * @return Test result
 */
dsrtos_test_result_t test_task_statistics(void)
{
    dsrtos_error_t result;
    dsrtos_task_handle_t task;
    dsrtos_task_config_t config;
    dsrtos_task_stats_t stats;
    
    /* Create test task */
    config.name = "StatsTestTask";
    config.function = test_task_1_func;
    config.parameter = NULL;
    config.priority = PHASE3_TASK_PRIORITY_MED;
    config.stack_size = PHASE3_TASK_STACK_SIZE;
    config.stack_pointer = g_task_1_stack;
    config.flags = DSRTOS_TASK_FLAG_NONE;
    
    result = dsrtos_task_create(&config, &task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test task statistics get */
    result = dsrtos_task_get_stats(task, &stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test initial statistics */
    DSRTOS_TEST_ASSERT_EQ(stats.execution_count, 0U);
    DSRTOS_TEST_ASSERT_EQ(stats.total_execution_time, 0U);
    
    /* Start task */
    result = dsrtos_task_start(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Wait for some execution */
    for (uint32_t i = 0U; i < 1000U; i++) {
        __asm volatile ("nop");
    }
    
    /* Stop task */
    result = dsrtos_task_stop(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test updated statistics */
    result = dsrtos_task_get_stats(task, &stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Clean up */
    result = dsrtos_task_delete(task);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test kernel operations
 * @return Test result
 */
dsrtos_test_result_t test_kernel_operations(void)
{
    dsrtos_error_t result;
    
    /* Test kernel initialization */
    result = dsrtos_kernel_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test kernel state */
    dsrtos_kernel_state_t state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_READY);
    
    /* Test kernel start */
    result = dsrtos_kernel_start();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_RUNNING);
    
    /* Test kernel stop */
    result = dsrtos_kernel_stop();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    state = dsrtos_kernel_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_KERNEL_STATE_STOPPED);
    
    /* Test kernel deinitialization */
    result = dsrtos_kernel_deinit();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize Phase 3 test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_phase3_init(void)
{
    /* Initialize test suite */
    g_phase3_test_suite.name = "Phase 3 - Task Management, Scheduling, Context Switching";
    g_phase3_test_suite.test_cases = g_phase3_test_cases;
    g_phase3_test_suite.test_count = sizeof(g_phase3_test_cases) / sizeof(g_phase3_test_cases[0]);
    g_phase3_test_suite.passed_count = 0U;
    g_phase3_test_suite.failed_count = 0U;
    g_phase3_test_suite.skipped_count = 0U;
    g_phase3_test_suite.error_count = 0U;
    g_phase3_test_suite.total_time_us = 0U;
    g_phase3_test_suite.enabled = true;
    
    /* Register test suite */
    return dsrtos_test_register_suite(&g_phase3_test_suite);
}
