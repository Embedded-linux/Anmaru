/* ============================================================================
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 7: Dynamic Scheduler Switching Test Suite
 * 
 * Comprehensive tests for scheduler switching, task migration, and rollback.
 * ============================================================================ */

#include "dsrtos_test.h"
#include "dsrtos_switch.h"
#include "dsrtos_migration.h"
#include "dsrtos_kernel.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_scheduler.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * TEST CONFIGURATION
 * ============================================================================ */

#define TEST_MAX_TASKS              32U
#define TEST_SWITCH_TIMEOUT_US      1000U
#define TEST_MIGRATION_BATCH_SIZE   8U
#define TEST_PRIORITY_LEVELS        256U

/* Test scheduler IDs */
#define TEST_SCHEDULER_RR          0x0001U
#define TEST_SCHEDULER_PRIORITY    0x0002U
#define TEST_SCHEDULER_EDF         0x0003U
#define TEST_SCHEDULER_CUSTOM      0x0004U

/* ============================================================================
 * TEST DATA STRUCTURES
 * ============================================================================ */

/* Test context */
typedef struct test_context {
    dsrtos_switch_controller_t* controller;
    dsrtos_tcb_t* test_tasks[TEST_MAX_TASKS];
    uint32_t task_count;
    uint32_t switch_count;
    uint32_t migration_count;
    bool test_passed;
} test_context_t;

/* Global test context */
static test_context_t g_test_ctx = {0};

/* Test statistics */
static struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t assertions_made;
} g_test_stats = {0};

/* ============================================================================
 * TEST HELPERS
 * ============================================================================ */

/**
 * @brief Initialize test environment
 */
static dsrtos_status_t test_init(void)
{
    dsrtos_status_t status;
    
    /* Initialize kernel */
    status = dsrtos_kernel_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Initialize switch controller */
    g_test_ctx.controller = &g_switch_controller;
    status = dsrtos_switch_controller_init(g_test_ctx.controller);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Create test tasks */
    for (uint32_t i = 0U; i < TEST_MAX_TASKS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "test_task_%u", i);
        
        g_test_ctx.test_tasks[i] = dsrtos_test_create_task(
            name,
            i,  /* Priority */
            1024U,  /* Stack size */
            NULL,  /* Entry function */
            NULL   /* Parameter */
        );
        
        TEST_ASSERT_NOT_NULL(g_test_ctx.test_tasks[i]);
    }
    
    g_test_ctx.task_count = TEST_MAX_TASKS;
    g_test_ctx.test_passed = true;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Clean up test environment
 */
static dsrtos_status_t test_cleanup(void)
{
    /* Delete test tasks */
    for (uint32_t i = 0U; i < g_test_ctx.task_count; i++) {
        if (g_test_ctx.test_tasks[i] != NULL) {
            dsrtos_task_delete(g_test_ctx.test_tasks[i]);
            g_test_ctx.test_tasks[i] = NULL;
        }
    }
    
    /* Deinitialize controller */
    (void)dsrtos_switch_controller_deinit(g_test_ctx.controller);
    
    /* Reset context */
    (void)memset(&g_test_ctx, 0, sizeof(g_test_ctx));
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Progress callback for migration tests
 */
static void test_migration_progress(uint32_t completed, uint32_t total)
{
    g_test_ctx.migration_count = completed;
    
    /* Verify progress is monotonic */
    static uint32_t last_completed = 0U;
    TEST_ASSERT(completed >= last_completed);
    TEST_ASSERT(completed <= total);
    last_completed = completed;
}

/**
 * @brief Validation callback for switch tests
 */
static bool test_switch_validator(const dsrtos_switch_request_t* request)
{
    /* Custom validation logic */
    if (request == NULL) {
        return false;
    }
    
    /* Don't allow switches to same scheduler */
    if (request->from_scheduler_id == request->to_scheduler_id) {
        return false;
    }
    
    /* Validate deadline */
    if (request->deadline_us == 0U) {
        return false;
    }
    
    return true;
}

/* ============================================================================
 * BASIC SWITCH TESTS
 * ============================================================================ */

/**
 * @brief Test controller initialization
 */
static void test_controller_init(void)
{
    dsrtos_status_t status;
    dsrtos_switch_controller_t controller = {0};
    
    TEST_START("Controller Initialization");
    
    /* Test NULL parameter */
    status = dsrtos_switch_controller_init(NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_INVALID_PARAM);
    
    /* Test valid initialization */
    status = dsrtos_switch_controller_init(&controller);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(controller.initialized);
    TEST_ASSERT_EQUAL(controller.switch_in_progress, false);
    
    /* Test double initialization */
    status = dsrtos_switch_controller_init(&controller);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test deinitialization */
    status = dsrtos_switch_controller_deinit(&controller);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(!controller.initialized);
    
    TEST_PASS();
}

/**
 * @brief Test basic scheduler switch
 */
static void test_basic_switch(void)
{
    dsrtos_status_t status;
    
    TEST_START("Basic Scheduler Switch");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform switch from RR to Priority */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR, 
                                    TEST_SCHEDULER_PRIORITY, 
                                    false);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify switch completed */
    TEST_ASSERT_EQUAL(dsrtos_switch_get_phase(), SWITCH_IDLE);
    TEST_ASSERT(!dsrtos_switch_in_progress());
    
    /* Check statistics */
    dsrtos_switch_stats_t stats;
    status = dsrtos_switch_get_stats(&stats);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT_EQUAL(stats.total_switches, 1U);
    TEST_ASSERT_EQUAL(stats.successful_switches, 1U);
    TEST_ASSERT_EQUAL(stats.failed_switches, 0U);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test switch with invalid parameters
 */
static void test_invalid_switch(void)
{
    dsrtos_status_t status;
    
    TEST_START("Invalid Switch Parameters");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test same scheduler switch */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR, 
                                    TEST_SCHEDULER_RR, 
                                    false);
    TEST_ASSERT_NOT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test NULL request */
    status = dsrtos_switch_scheduler_ex(NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_INVALID_PARAM);
    
    /* Test invalid scheduler IDs */
    status = dsrtos_switch_scheduler(0xFFFFU, 0xFFFEU, false);
    TEST_ASSERT_NOT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * MIGRATION STRATEGY TESTS
 * ============================================================================ */

/**
 * @brief Test preserve order migration strategy
 */
static void test_migration_preserve_order(void)
{
    dsrtos_status_t status;
    
    TEST_START("Migration - Preserve Order Strategy");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform migration with preserve order */
    status = dsrtos_migrate_tasks_strategic(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        TEST_SCHEDULER_RR,
        TEST_SCHEDULER_PRIORITY,
        MIGRATE_PRESERVE_ORDER,
        NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify task order preserved */
    /* Tasks should maintain relative ordering */
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test priority-based migration strategy
 */
static void test_migration_priority_based(void)
{
    dsrtos_status_t status;
    
    TEST_START("Migration - Priority Based Strategy");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Set different priorities for tasks */
    for (uint32_t i = 0U; i < g_test_ctx.task_count; i++) {
        g_test_ctx.test_tasks[i]->priority = (i * 8U) % TEST_PRIORITY_LEVELS;
    }
    
    /* Perform migration with priority strategy */
    status = dsrtos_migrate_tasks_strategic(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        TEST_SCHEDULER_RR,
        TEST_SCHEDULER_PRIORITY,
        MIGRATE_PRIORITY_BASED,
        NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify tasks sorted by priority */
    for (uint32_t i = 1U; i < g_test_ctx.task_count; i++) {
        TEST_ASSERT(g_test_ctx.test_tasks[i-1]->priority <= 
                   g_test_ctx.test_tasks[i]->priority);
    }
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test deadline-based migration strategy
 */
static void test_migration_deadline_based(void)
{
    dsrtos_status_t status;
    
    TEST_START("Migration - Deadline Based Strategy");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Set deadlines for tasks (using runtime_stats as placeholder) */
    for (uint32_t i = 0U; i < g_test_ctx.task_count; i++) {
        g_test_ctx.test_tasks[i]->runtime_stats.total_runtime = 
            (g_test_ctx.task_count - i) * 100U;
    }
    
    /* Perform migration with deadline strategy */
    status = dsrtos_migrate_tasks_strategic(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        TEST_SCHEDULER_PRIORITY,
        TEST_SCHEDULER_EDF,
        MIGRATE_DEADLINE_BASED,
        NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test batch migration with progress tracking
 */
static void test_batch_migration(void)
{
    dsrtos_status_t status;
    
    TEST_START("Batch Migration with Progress");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Reset migration count */
    g_test_ctx.migration_count = 0U;
    
    /* Perform batch migration */
    status = dsrtos_migrate_batch_with_progress(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        TEST_SCHEDULER_RR,
        TEST_SCHEDULER_PRIORITY,
        MIGRATE_PRESERVE_ORDER,
        test_migration_progress);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify all tasks migrated */
    TEST_ASSERT_EQUAL(g_test_ctx.migration_count, g_test_ctx.task_count);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * ROLLBACK TESTS
 * ============================================================================ */

/**
 * @brief Test switch rollback on failure
 */
static void test_switch_rollback(void)
{
    dsrtos_status_t status;
    dsrtos_switch_context_t context = {0};
    
    TEST_START("Switch Rollback on Failure");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Prepare switch context */
    context.request.from_scheduler_id = TEST_SCHEDULER_RR;
    context.request.to_scheduler_id = TEST_SCHEDULER_PRIORITY;
    context.request.forced = false;
    context.rollback.rollback_possible = true;
    
    /* Simulate failed switch by setting phase */
    context.phase = SWITCH_MIGRATING_TASKS;
    
    /* Perform rollback */
    status = dsrtos_switch_rollback(&context);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify rollback completed */
    TEST_ASSERT_EQUAL(context.phase, SWITCH_IDLE);
    TEST_ASSERT(!context.rollback.rollback_possible);
    
    /* Check rollback statistics */
    dsrtos_switch_stats_t stats;
    status = dsrtos_switch_get_stats(&stats);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(stats.rollback_count > 0U);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test state preservation during switch
 */
static void test_state_preservation(void)
{
    dsrtos_status_t status;
    dsrtos_switch_context_t context = {0};
    uint8_t state_buffer[1024] = {0};
    
    TEST_START("State Preservation During Switch");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Set up context with state buffer */
    context.preservation.state_buffer = state_buffer;
    context.preservation.buffer_size = sizeof(state_buffer);
    
    /* Save state */
    status = dsrtos_switch_save_state(&context, 
                                     g_test_ctx.controller,
                                     TEST_SCHEDULER_RR);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(context.preservation.used_size > 0U);
    
    /* Modify controller state */
    g_test_ctx.controller->switch_count = 999U;
    
    /* Restore state */
    status = dsrtos_switch_restore_state(&context,
                                        g_test_ctx.controller,
                                        TEST_SCHEDULER_RR);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * VALIDATION TESTS
 * ============================================================================ */

/**
 * @brief Test switch validation
 */
static void test_switch_validation(void)
{
    dsrtos_status_t status;
    dsrtos_switch_request_t request = {0};
    
    TEST_START("Switch Request Validation");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Register validation callback */
    status = dsrtos_switch_register_validator(test_switch_validator);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test valid request */
    request.from_scheduler_id = TEST_SCHEDULER_RR;
    request.to_scheduler_id = TEST_SCHEDULER_PRIORITY;
    request.deadline_us = TEST_SWITCH_TIMEOUT_US;
    request.strategy = MIGRATE_PRESERVE_ORDER;
    
    status = dsrtos_switch_validate_request(&request);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test invalid request - same scheduler */
    request.from_scheduler_id = TEST_SCHEDULER_RR;
    request.to_scheduler_id = TEST_SCHEDULER_RR;
    
    status = dsrtos_switch_validate_request(&request);
    TEST_ASSERT_NOT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Test invalid strategy */
    request.from_scheduler_id = TEST_SCHEDULER_RR;
    request.to_scheduler_id = TEST_SCHEDULER_PRIORITY;
    request.strategy = MIGRATE_STRATEGY_COUNT;  /* Invalid */
    
    status = dsrtos_switch_validate_request(&request);
    TEST_ASSERT_NOT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test migration validation
 */
static void test_migration_validation(void)
{
    bool can_migrate;
    
    TEST_START("Migration Validation");
    
    /* Initialize test environment */
    (void)test_init();
    
    /* Test valid task migration */
    can_migrate = dsrtos_can_migrate_task(g_test_ctx.test_tasks[0],
                                         TEST_SCHEDULER_PRIORITY);
    TEST_ASSERT(can_migrate);
    
    /* Test NULL task */
    can_migrate = dsrtos_can_migrate_task(NULL, TEST_SCHEDULER_PRIORITY);
    TEST_ASSERT(!can_migrate);
    
    /* Test deleted task */
    g_test_ctx.test_tasks[0]->state = TASK_STATE_DELETED;
    can_migrate = dsrtos_can_migrate_task(g_test_ctx.test_tasks[0],
                                         TEST_SCHEDULER_PRIORITY);
    TEST_ASSERT(!can_migrate);
    
    /* Restore task state */
    g_test_ctx.test_tasks[0]->state = TASK_STATE_READY;
    
    /* Clean up */
    (void)test_cleanup();
    
    TEST_PASS();
}

/* ============================================================================
 * PERFORMANCE TESTS
 * ============================================================================ */

/**
 * @brief Test switch timing constraints
 */
static void test_switch_timing(void)
{
    dsrtos_status_t status;
    uint32_t start_time, end_time, duration;
    
    TEST_START("Switch Timing Constraints");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Measure switch time */
    start_time = dsrtos_kernel_get_tick_count();
    
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR,
                                    TEST_SCHEDULER_PRIORITY,
                                    false);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    end_time = dsrtos_kernel_get_tick_count();
    duration = (end_time - start_time) * 1000U;  /* Convert to microseconds */
    
    /* Verify timing constraint */
    TEST_ASSERT(duration <= DSRTOS_MAX_SWITCH_TIME_US);
    
    /* Get actual switch duration */
    dsrtos_switch_stats_t stats;
    status = dsrtos_switch_get_stats(&stats);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(stats.last_switch_time_us <= DSRTOS_MAX_SWITCH_TIME_US);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test critical section timing
 */
static void test_critical_timing(void)
{
    dsrtos_status_t status;
    dsrtos_switch_stats_t stats;
    
    TEST_START("Critical Section Timing");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform switch */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR,
                                    TEST_SCHEDULER_PRIORITY,
                                    false);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Check critical section time */
    status = dsrtos_switch_get_stats(&stats);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(stats.max_critical_time_us <= DSRTOS_SWITCH_CRITICAL_TIME_US);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test migration performance
 */
static void test_migration_performance(void)
{
    dsrtos_status_t status;
    uint32_t estimated_time;
    uint32_t actual_time;
    
    TEST_START("Migration Performance");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Estimate migration time */
    estimated_time = dsrtos_estimate_migration_time(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        MIGRATE_PRESERVE_ORDER);
    
    /* Perform migration and measure time */
    uint32_t start = dsrtos_kernel_get_tick_count();
    
    status = dsrtos_migrate_tasks_strategic(
        g_test_ctx.test_tasks,
        g_test_ctx.task_count,
        TEST_SCHEDULER_RR,
        TEST_SCHEDULER_PRIORITY,
        MIGRATE_PRESERVE_ORDER,
        NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    uint32_t end = dsrtos_kernel_get_tick_count();
    actual_time = (end - start) * 1000U;  /* Convert to microseconds */
    
    /* Verify estimation accuracy (within 50%) */
    TEST_ASSERT(actual_time <= (estimated_time * 3U / 2U));
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * POLICY TESTS
 * ============================================================================ */

/**
 * @brief Test switch policies
 */
static void test_switch_policies(void)
{
    dsrtos_status_t status;
    
    TEST_START("Switch Policy Configuration");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Set restrictive policies */
    status = dsrtos_switch_set_policy(
        1000U,  /* Min interval: 1 second */
        100U,   /* Max duration: 100us */
        false   /* No forced switches */
    );
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Try rapid switches - should fail due to min interval */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR,
                                    TEST_SCHEDULER_PRIORITY,
                                    false);
    /* First switch might succeed */
    
    /* Immediate second switch should fail */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_PRIORITY,
                                    TEST_SCHEDULER_EDF,
                                    false);
    TEST_ASSERT_NOT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Reset policies to defaults */
    status = dsrtos_switch_set_policy(
        DSRTOS_MIN_SWITCH_INTERVAL_MS,
        DSRTOS_MAX_SWITCH_TIME_US,
        true
    );
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * HISTORY AND STATISTICS TESTS
 * ============================================================================ */

/**
 * @brief Test switch history tracking
 */
static void test_switch_history(void)
{
    dsrtos_status_t status;
    dsrtos_switch_history_t history[DSRTOS_SWITCH_HISTORY_SIZE];
    uint32_t count;
    
    TEST_START("Switch History Tracking");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Clear history */
    status = dsrtos_switch_clear_history();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform multiple switches */
    for (uint32_t i = 0U; i < 3U; i++) {
        uint16_t from = (i % 2U) ? TEST_SCHEDULER_RR : TEST_SCHEDULER_PRIORITY;
        uint16_t to = (i % 2U) ? TEST_SCHEDULER_PRIORITY : TEST_SCHEDULER_RR;
        
        status = dsrtos_switch_scheduler(from, to, false);
        /* Ignore status - focus on history */
        (void)status;
    }
    
    /* Get history */
    count = DSRTOS_SWITCH_HISTORY_SIZE;
    status = dsrtos_switch_get_history(history, &count);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT(count > 0U);
    
    /* Verify history records */
    for (uint32_t i = 0U; i < count; i++) {
        TEST_ASSERT(history[i].timestamp > 0U);
        TEST_ASSERT(history[i].from_id != history[i].to_id);
    }
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test statistics collection
 */
static void test_statistics(void)
{
    dsrtos_status_t status;
    dsrtos_switch_stats_t stats;
    
    TEST_START("Statistics Collection");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Reset controller for clean stats */
    status = dsrtos_switch_controller_reset(g_test_ctx.controller);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform successful switch */
    status = dsrtos_switch_scheduler(TEST_SCHEDULER_RR,
                                    TEST_SCHEDULER_PRIORITY,
                                    false);
    
    /* Get statistics */
    status = dsrtos_switch_get_stats(&stats);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify statistics */
    TEST_ASSERT_EQUAL(stats.total_switches, 1U);
    TEST_ASSERT(stats.last_switch_time_us > 0U);
    TEST_ASSERT(stats.min_switch_time_us != UINT32_MAX);
    TEST_ASSERT(stats.max_switch_time_us > 0U);
    TEST_ASSERT(stats.avg_switch_time_us > 0U);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * DEBUGGING TESTS
 * ============================================================================ */

/**
 * @brief Test phase tracking
 */
static void test_phase_tracking(void)
{
    dsrtos_switch_phase_t phase;
    const char* phase_name;
    
    TEST_START("Switch Phase Tracking");
    
    /* Initialize test environment */
    (void)test_init();
    
    /* Get idle phase */
    phase = dsrtos_switch_get_phase();
    TEST_ASSERT_EQUAL(phase, SWITCH_IDLE);
    
    /* Get phase name */
    phase_name = dsrtos_switch_get_phase_name(SWITCH_IDLE);
    TEST_ASSERT_NOT_NULL(phase_name);
    TEST_ASSERT_STRING_EQUAL(phase_name, "IDLE");
    
    /* Test all phase names */
    for (int i = 0; i <= SWITCH_ROLLING_BACK; i++) {
        phase_name = dsrtos_switch_get_phase_name((dsrtos_switch_phase_t)i);
        TEST_ASSERT_NOT_NULL(phase_name);
    }
    
    /* Clean up */
    (void)test_cleanup();
    
    TEST_PASS();
}

/**
 * @brief Test error reporting
 */
static void test_error_reporting(void)
{
    dsrtos_status_t status;
    uint32_t error_code;
    const char* error_msg;
    
    TEST_START("Error Reporting");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Get error info when no error */
    status = dsrtos_switch_get_last_error(&error_code, &error_msg);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    TEST_ASSERT_EQUAL(error_code, 0U);
    TEST_ASSERT_NOT_NULL(error_msg);
    
    /* Test NULL parameters */
    status = dsrtos_switch_get_last_error(NULL, &error_msg);
    TEST_ASSERT_EQUAL(status, DSRTOS_INVALID_PARAM);
    
    status = dsrtos_switch_get_last_error(&error_code, NULL);
    TEST_ASSERT_EQUAL(status, DSRTOS_INVALID_PARAM);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

/**
 * @brief Stress test with rapid switches
 */
static void test_rapid_switches(void)
{
    dsrtos_status_t status;
    uint32_t successful_switches = 0U;
    
    TEST_START("Rapid Switch Stress Test");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Set minimal interval for rapid switching */
    status = dsrtos_switch_set_policy(1U, DSRTOS_MAX_SWITCH_TIME_US, true);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Perform rapid switches */
    for (uint32_t i = 0U; i < 10U; i++) {
        uint16_t from = (i % 2U) ? TEST_SCHEDULER_RR : TEST_SCHEDULER_PRIORITY;
        uint16_t to = (i % 2U) ? TEST_SCHEDULER_PRIORITY : TEST_SCHEDULER_RR;
        
        status = dsrtos_switch_scheduler(from, to, true);
        if (status == DSRTOS_SUCCESS) {
            successful_switches++;
        }
        
        /* Small delay between switches */
        for (volatile uint32_t j = 0U; j < 1000U; j++);
    }
    
    /* Verify some switches succeeded */
    TEST_ASSERT(successful_switches > 0U);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/**
 * @brief Test large task migration
 */
static void test_large_migration(void)
{
    dsrtos_status_t status;
    
    TEST_START("Large Task Migration");
    
    /* Initialize test environment */
    status = test_init();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Migrate all test tasks */
    status = dsrtos_migrate_batch_with_progress(
        g_test_ctx.test_tasks,
        TEST_MAX_TASKS,
        TEST_SCHEDULER_RR,
        TEST_SCHEDULER_PRIORITY,
        MIGRATE_PRIORITY_BASED,
        test_migration_progress);
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    /* Verify all tasks migrated */
    TEST_ASSERT_EQUAL(g_test_ctx.migration_count, TEST_MAX_TASKS);
    
    /* Clean up */
    status = test_cleanup();
    TEST_ASSERT_EQUAL(status, DSRTOS_SUCCESS);
    
    TEST_PASS();
}

/* ============================================================================
 * TEST SUITE RUNNER
 * ============================================================================ */

/**
 * @brief Run all switch tests
 */
void dsrtos_test_switch_suite(void)
{
    printf("\n==================================================\n");
    printf("DSRTOS Phase 7: Dynamic Scheduler Switching Tests\n");
    printf("==================================================\n\n");
    
    /* Reset test statistics */
    g_test_stats.tests_run = 0U;
    g_test_stats.tests_passed = 0U;
    g_test_stats.tests_failed = 0U;
    
    /* Basic tests */
    test_controller_init();
    test_basic_switch();
    test_invalid_switch();
    
    /* Migration strategy tests */
    test_migration_preserve_order();
    test_migration_priority_based();
    test_migration_deadline_based();
    test_batch_migration();
    
    /* Rollback tests */
    test_switch_rollback();
    test_state_preservation();
    
    /* Validation tests */
    test_switch_validation();
    test_migration_validation();
    
    /* Performance tests */
    test_switch_timing();
    test_critical_timing();
    test_migration_performance();
    
    /* Policy tests */
    test_switch_policies();
    
    /* History and statistics tests */
    test_switch_history();
    test_statistics();
    
    /* Debugging tests */
    test_phase_tracking();
    test_error_reporting();
    
    /* Stress tests */
    test_rapid_switches();
    test_large_migration();
    
    /* Print summary */
    printf("\n==================================================\n");
    printf("Test Summary:\n");
    printf("  Tests Run:    %u\n", g_test_stats.tests_run);
    printf("  Tests Passed: %u\n", g_test_stats.tests_passed);
    printf("  Tests Failed: %u\n", g_test_stats.tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           (g_test_stats.tests_run > 0) ?
           (100.0f * g_test_stats.tests_passed / g_test_stats.tests_run) : 0.0f);
    printf("==================================================\n\n");
}

/**
 * @brief Main test entry point
 */
int main(void)
{
    /* Initialize hardware if needed */
    dsrtos_test_init_hardware();
    
    /* Run test suite */
    dsrtos_test_switch_suite();
    
    /* Return based on test results */
    return (g_test_stats.tests_failed == 0U) ? 0 : -1;
}
