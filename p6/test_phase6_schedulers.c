/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Development Team
 * 
 * File: test_phase6_schedulers.c
 * Description: Comprehensive test suite for Round Robin and Priority schedulers
 * Phase: 6 - Concrete Scheduler Tests
 * 
 * Test Coverage:
 * - Round Robin scheduling
 * - Priority scheduling with O(1) operations
 * - Priority inheritance
 * - Starvation prevention
 * - Performance benchmarks
 */

#include "dsrtos_test.h"
#include "dsrtos_scheduler_rr.h"
#include "dsrtos_scheduler_priority.h"
#include "dsrtos_kernel.h"
#include "dsrtos_task.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * TEST CONFIGURATION
 * ============================================================================ */

#define TEST_NUM_TASKS          16
#define TEST_STACK_SIZE         2048
#define TEST_TIME_SLICE_MS      10
#define TEST_ITERATIONS         1000

/* Test task priorities */
#define TEST_PRIO_HIGH          10
#define TEST_PRIO_MEDIUM        128
#define TEST_PRIO_LOW           200

/* Performance thresholds */
#define MAX_RR_SCHEDULE_US      5
#define MAX_PRIO_SCHEDULE_US    3
#define MAX_ENQUEUE_US          2

/* ============================================================================
 * TEST DATA STRUCTURES
 * ============================================================================ */

typedef struct {
    dsrtos_tcb_t tcb;
    uint8_t stack[TEST_STACK_SIZE];
    uint32_t execution_count;
    uint32_t last_run_time;
    uint8_t priority;
} test_task_t;

typedef struct {
    dsrtos_rr_scheduler_t rr_scheduler;
    dsrtos_priority_scheduler_t prio_scheduler;
    test_task_t tasks[TEST_NUM_TASKS];
    uint32_t test_counter;
    bool test_passed;
} test_context_t;

static test_context_t g_test_ctx;

/* ============================================================================
 * ROUND ROBIN SCHEDULER TESTS
 * ============================================================================ */

/**
 * @brief Test Round Robin initialization
 */
static bool test_rr_init(void)
{
    dsrtos_status_t status;
    dsrtos_rr_scheduler_t* rr = &g_test_ctx.rr_scheduler;
    
    TEST_PRINT("Testing Round Robin initialization...");
    
    /* Test valid initialization */
    status = dsrtos_rr_init(rr, TEST_TIME_SLICE_MS);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR init should succeed");
    TEST_ASSERT(rr->base.magic == SCHEDULER_MAGIC, "Magic number should be set");
    TEST_ASSERT(rr->time_slice_ms == TEST_TIME_SLICE_MS, "Time slice should match");
    TEST_ASSERT(rr->ready_queue.count == 0, "Queue should be empty");
    
    /* Test invalid parameters */
    status = dsrtos_rr_init(NULL, TEST_TIME_SLICE_MS);
    TEST_ASSERT(status == DSRTOS_INVALID_PARAM, "NULL scheduler should fail");
    
    status = dsrtos_rr_init(rr, 0);
    TEST_ASSERT(status == DSRTOS_INVALID_PARAM, "Zero time slice should fail");
    
    status = dsrtos_rr_init(rr, 1000);
    TEST_ASSERT(status == DSRTOS_INVALID_PARAM, "Excessive time slice should fail");
    
    return true;
}

/**
 * @brief Test Round Robin task enqueueing
 */
static bool test_rr_enqueue(void)
{
    dsrtos_status_t status;
    dsrtos_rr_scheduler_t* rr = &g_test_ctx.rr_scheduler;
    test_task_t* task;
    uint32_t i;
    uint32_t start_time, end_time;
    
    TEST_PRINT("Testing Round Robin enqueue operations...");
    
    /* Initialize scheduler */
    status = dsrtos_rr_init(rr, TEST_TIME_SLICE_MS);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR init failed");
    
    status = dsrtos_rr_start(rr);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR start failed");
    
    /* Enqueue multiple tasks */
    for (i = 0; i < TEST_NUM_TASKS; i++) {
        task = &g_test_ctx.tasks[i];
        task->tcb.magic = TCB_MAGIC;
        task->tcb.tid = i + 1;
        task->tcb.state = TASK_STATE_READY;
        
        start_time = dsrtos_port_get_cycle_count();
        status = dsrtos_rr_enqueue(rr, &task->tcb);
        end_time = dsrtos_port_get_cycle_count();
        
        TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue should succeed");
        
        /* Verify performance */
        uint32_t enqueue_us = dsrtos_port_cycles_to_us(end_time - start_time);
        TEST_ASSERT(enqueue_us <= MAX_ENQUEUE_US, "Enqueue too slow");
    }
    
    TEST_ASSERT(rr->ready_queue.count == TEST_NUM_TASKS, "Queue count mismatch");
    TEST_ASSERT(dsrtos_rr_queue_verify_integrity(&rr->ready_queue), 
                "Queue integrity check failed");
    
    return true;
}

/**
 * @brief Test Round Robin scheduling
 */
static bool test_rr_scheduling(void)
{
    dsrtos_rr_scheduler_t* rr = &g_test_ctx.rr_scheduler;
    dsrtos_tcb_t* selected;
    test_task_t* task;
    uint32_t i, j;
    uint32_t start_time, end_time;
    dsrtos_status_t status;
    
    TEST_PRINT("Testing Round Robin scheduling...");
    
    /* Initialize and start scheduler */
    status = dsrtos_rr_init(rr, TEST_TIME_SLICE_MS);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR init failed");
    
    status = dsrtos_rr_start(rr);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR start failed");
    
    /* Enqueue tasks */
    for (i = 0; i < 4; i++) {
        task = &g_test_ctx.tasks[i];
        task->tcb.magic = TCB_MAGIC;
        task->tcb.tid = i + 1;
        task->tcb.state = TASK_STATE_READY;
        task->execution_count = 0;
        
        status = dsrtos_rr_enqueue(rr, &task->tcb);
        TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue failed");
    }
    
    /* Test scheduling order (should be round-robin) */
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            start_time = dsrtos_port_get_cycle_count();
            selected = dsrtos_rr_select_next(rr);
            end_time = dsrtos_port_get_cycle_count();
            
            TEST_ASSERT(selected != NULL, "Should select a task");
            TEST_ASSERT(selected->tid == (i + 1), "Wrong task order");
            
            /* Verify performance */
            uint32_t schedule_us = dsrtos_port_cycles_to_us(end_time - start_time);
            TEST_ASSERT(schedule_us <= MAX_RR_SCHEDULE_US, "Schedule too slow");
            
            /* Re-enqueue for next round */
            if (j < 2 || i < 3) {
                status = dsrtos_rr_enqueue(rr, selected);
                TEST_ASSERT(status == DSRTOS_SUCCESS, "Re-enqueue failed");
            }
        }
    }
    
    return true;
}

/**
 * @brief Test Round Robin time slicing
 */
static bool test_rr_time_slicing(void)
{
    dsrtos_rr_scheduler_t* rr = &g_test_ctx.rr_scheduler;
    dsrtos_tcb_t* current;
    uint32_t i;
    dsrtos_status_t status;
    
    TEST_PRINT("Testing Round Robin time slicing...");
    
    /* Initialize with short time slice */
    status = dsrtos_rr_init(rr, 5);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR init failed");
    
    status = dsrtos_rr_start(rr);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR start failed");
    
    /* Enqueue a task */
    g_test_ctx.tasks[0].tcb.magic = TCB_MAGIC;
    g_test_ctx.tasks[0].tcb.tid = 1;
    g_test_ctx.tasks[0].tcb.state = TASK_STATE_READY;
    
    status = dsrtos_rr_enqueue(rr, &g_test_ctx.tasks[0].tcb);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue failed");
    
    /* Select task */
    current = dsrtos_rr_select_next(rr);
    TEST_ASSERT(current != NULL, "Should select task");
    TEST_ASSERT(rr->slice_remaining == 5, "Slice should be reset");
    
    /* Simulate ticks */
    for (i = 0; i < 5; i++) {
        dsrtos_rr_tick_update(rr);
        TEST_ASSERT(rr->slice_remaining == (4 - i), "Slice countdown wrong");
    }
    
    /* Slice should be expired */
    TEST_ASSERT(rr->slice_remaining == 0, "Slice should be expired");
    TEST_ASSERT(rr->stats.total_preemptions > 0, "Should count preemption");
    
    return true;
}

/* ============================================================================
 * PRIORITY SCHEDULER TESTS
 * ============================================================================ */

/**
 * @brief Test Priority scheduler initialization
 */
static bool test_priority_init(void)
{
    dsrtos_status_t status;
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    uint32_t i;
    
    TEST_PRINT("Testing Priority scheduler initialization...");
    
    /* Test valid initialization */
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init should succeed");
    TEST_ASSERT(prio->base.magic == SCHEDULER_MAGIC, "Magic number should be set");
    TEST_ASSERT(prio->num_priorities == PRIO_NUM_LEVELS, "Priority levels wrong");
    
    /* Verify all queues are empty */
    for (i = 0; i < PRIO_NUM_LEVELS; i++) {
        TEST_ASSERT(prio->ready_queues[i].count == 0, "Queue should be empty");
    }
    
    /* Verify bitmap is clear */
    for (i = 0; i < PRIO_BITMAP_WORDS; i++) {
        TEST_ASSERT(prio->priority_map.bitmap[i] == 0, "Bitmap should be clear");
    }
    
    return true;
}

/**
 * @brief Test O(1) priority bitmap operations
 */
static bool test_priority_bitmap(void)
{
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    uint8_t highest;
    uint32_t start_time, end_time;
    dsrtos_status_t status;
    
    TEST_PRINT("Testing O(1) priority bitmap operations...");
    
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init failed");
    
    /* Test setting individual priorities */
    PRIO_BITMAP_SET(prio, 0);
    TEST_ASSERT(PRIO_BITMAP_TEST(prio, 0), "Bit 0 should be set");
    
    PRIO_BITMAP_SET(prio, 63);
    TEST_ASSERT(PRIO_BITMAP_TEST(prio, 63), "Bit 63 should be set");
    
    PRIO_BITMAP_SET(prio, 255);
    TEST_ASSERT(PRIO_BITMAP_TEST(prio, 255), "Bit 255 should be set");
    
    /* Test finding highest priority - should be O(1) */
    start_time = dsrtos_port_get_cycle_count();
    highest = prio_bitmap_ffs(prio->priority_map.bitmap);
    end_time = dsrtos_port_get_cycle_count();
    
    TEST_ASSERT(highest == 0, "Highest priority should be 0");
    
    /* Verify O(1) performance */
    uint32_t find_cycles = end_time - start_time;
    TEST_ASSERT(find_cycles < 100, "Bitmap find should be O(1)");
    
    /* Clear bit 0 and retest */
    PRIO_BITMAP_CLEAR(prio, 0);
    highest = prio_bitmap_ffs(prio->priority_map.bitmap);
    TEST_ASSERT(highest == 63, "Highest priority should be 63");
    
    return true;
}

/**
 * @brief Test priority-based scheduling
 */
static bool test_priority_scheduling(void)
{
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    dsrtos_tcb_t* selected;
    test_task_t* task;
    uint32_t start_time, end_time;
    dsrtos_status_t status;
    
    TEST_PRINT("Testing priority-based scheduling...");
    
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init failed");
    
    status = dsrtos_priority_start(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority start failed");
    
    /* Enqueue tasks at different priorities */
    task = &g_test_ctx.tasks[0];
    task->tcb.magic = TCB_MAGIC;
    task->tcb.tid = 1;
    task->tcb.priority = TEST_PRIO_LOW;
    task->tcb.state = TASK_STATE_READY;
    status = dsrtos_priority_enqueue(prio, &task->tcb, TEST_PRIO_LOW);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue low priority failed");
    
    task = &g_test_ctx.tasks[1];
    task->tcb.magic = TCB_MAGIC;
    task->tcb.tid = 2;
    task->tcb.priority = TEST_PRIO_HIGH;
    task->tcb.state = TASK_STATE_READY;
    status = dsrtos_priority_enqueue(prio, &task->tcb, TEST_PRIO_HIGH);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue high priority failed");
    
    task = &g_test_ctx.tasks[2];
    task->tcb.magic = TCB_MAGIC;
    task->tcb.tid = 3;
    task->tcb.priority = TEST_PRIO_MEDIUM;
    task->tcb.state = TASK_STATE_READY;
    status = dsrtos_priority_enqueue(prio, &task->tcb, TEST_PRIO_MEDIUM);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue medium priority failed");
    
    /* Should select highest priority first */
    start_time = dsrtos_port_get_cycle_count();
    selected = dsrtos_priority_select_next(prio);
    end_time = dsrtos_port_get_cycle_count();
    
    TEST_ASSERT(selected != NULL, "Should select a task");
    TEST_ASSERT(selected->tid == 2, "Should select high priority task");
    
    /* Verify O(1) performance */
    uint32_t schedule_us = dsrtos_port_cycles_to_us(end_time - start_time);
    TEST_ASSERT(schedule_us <= MAX_PRIO_SCHEDULE_US, "Schedule too slow");
    
    /* Next should be medium priority */
    selected = dsrtos_priority_select_next(prio);
    TEST_ASSERT(selected != NULL, "Should select a task");
    TEST_ASSERT(selected->tid == 3, "Should select medium priority task");
    
    /* Finally low priority */
    selected = dsrtos_priority_select_next(prio);
    TEST_ASSERT(selected != NULL, "Should select a task");
    TEST_ASSERT(selected->tid == 1, "Should select low priority task");
    
    return true;
}

/**
 * @brief Test priority inheritance
 */
static bool test_priority_inheritance(void)
{
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    test_task_t* task;
    dsrtos_status_t status;
    uint8_t original_prio, current_prio;
    void* dummy_resource = (void*)0x1234;
    
    TEST_PRINT("Testing priority inheritance...");
    
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init failed");
    
    status = dsrtos_priority_start(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority start failed");
    
    /* Create low priority task */
    task = &g_test_ctx.tasks[0];
    task->tcb.magic = TCB_MAGIC;
    task->tcb.tid = 1;
    task->tcb.priority = TEST_PRIO_LOW;
    task->tcb.state = TASK_STATE_READY;
    
    status = dsrtos_priority_enqueue(prio, &task->tcb, TEST_PRIO_LOW);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue failed");
    
    /* Get original priority */
    original_prio = dsrtos_priority_get(prio, &task->tcb);
    TEST_ASSERT(original_prio == TEST_PRIO_LOW, "Wrong original priority");
    
    /* Apply priority inheritance */
    status = dsrtos_priority_inherit(prio, &task->tcb, TEST_PRIO_HIGH, dummy_resource);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority inheritance failed");
    
    /* Verify priority was elevated */
    current_prio = dsrtos_priority_get(prio, &task->tcb);
    TEST_ASSERT(current_prio == TEST_PRIO_HIGH, "Priority not inherited");
    
    /* Remove inheritance */
    status = dsrtos_priority_uninherit(prio, &task->tcb, dummy_resource);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority uninherit failed");
    
    /* Verify priority restored */
    current_prio = dsrtos_priority_get(prio, &task->tcb);
    TEST_ASSERT(current_prio == TEST_PRIO_LOW, "Priority not restored");
    
    TEST_ASSERT(prio->stats.inheritance_activations > 0, "Should count inheritance");
    
    return true;
}

/**
 * @brief Test priority aging for starvation prevention
 */
static bool test_priority_aging(void)
{
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    test_task_t* task;
    dsrtos_status_t status;
    uint32_t i;
    
    TEST_PRINT("Testing priority aging...");
    
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init failed");
    
    /* Configure aggressive aging for testing */
    status = dsrtos_priority_aging_configure(prio, 100, 200, 20);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Aging config failed");
    
    status = dsrtos_priority_start(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority start failed");
    
    /* Create low priority task */
    task = &g_test_ctx.tasks[0];
    task->tcb.magic = TCB_MAGIC;
    task->tcb.tid = 1;
    task->tcb.priority = 200;  /* Low priority */
    task->tcb.state = TASK_STATE_READY;
    
    /* Enqueue and backdate to simulate aging */
    status = dsrtos_priority_enqueue(prio, &task->tcb, 200);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Enqueue failed");
    
    /* Manually set old enqueue time to trigger aging */
    dsrtos_priority_node_t* node = prio->ready_queues[200].head;
    TEST_ASSERT(node != NULL, "Node should exist");
    node->enqueue_time = dsrtos_get_tick_count() - 6000;  /* 6 seconds ago */
    
    /* Trigger aging check */
    dsrtos_priority_aging_check(prio);
    
    /* Verify task was promoted */
    uint8_t new_prio = dsrtos_priority_get(prio, &task->tcb);
    TEST_ASSERT(new_prio < 200, "Task should be promoted");
    TEST_ASSERT(prio->stats.aging_adjustments > 0, "Should count aging");
    
    return true;
}

/* ============================================================================
 * PERFORMANCE BENCHMARKS
 * ============================================================================ */

/**
 * @brief Benchmark Round Robin performance
 */
static bool benchmark_rr_performance(void)
{
    dsrtos_rr_scheduler_t* rr = &g_test_ctx.rr_scheduler;
    test_task_t* task;
    uint32_t i;
    uint32_t total_schedule_time = 0;
    uint32_t total_enqueue_time = 0;
    dsrtos_status_t status;
    
    TEST_PRINT("Benchmarking Round Robin performance...");
    
    status = dsrtos_rr_init(rr, TEST_TIME_SLICE_MS);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR init failed");
    
    status = dsrtos_rr_start(rr);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "RR start failed");
    
    /* Prepare tasks */
    for (i = 0; i < TEST_NUM_TASKS; i++) {
        task = &g_test_ctx.tasks[i];
        task->tcb.magic = TCB_MAGIC;
        task->tcb.tid = i + 1;
        task->tcb.state = TASK_STATE_READY;
    }
    
    /* Benchmark enqueue operations */
    for (i = 0; i < TEST_ITERATIONS; i++) {
        uint32_t task_idx = i % TEST_NUM_TASKS;
        uint32_t start = dsrtos_port_get_cycle_count();
        
        status = dsrtos_rr_enqueue(rr, &g_test_ctx.tasks[task_idx].tcb);
        
        uint32_t end = dsrtos_port_get_cycle_count();
        total_enqueue_time += (end - start);
        
        if (status != DSRTOS_SUCCESS) {
            /* Remove to make room */
            dsrtos_rr_select_next(rr);
        }
    }
    
    /* Benchmark schedule operations */
    for (i = 0; i < TEST_ITERATIONS; i++) {
        uint32_t start = dsrtos_port_get_cycle_count();
        
        dsrtos_tcb_t* selected = dsrtos_rr_select_next(rr);
        
        uint32_t end = dsrtos_port_get_cycle_count();
        total_schedule_time += (end - start);
        
        if (selected != NULL) {
            /* Re-enqueue for next iteration */
            dsrtos_rr_enqueue(rr, selected);
        }
    }
    
    /* Calculate averages */
    uint32_t avg_enqueue_us = dsrtos_port_cycles_to_us(total_enqueue_time / TEST_ITERATIONS);
    uint32_t avg_schedule_us = dsrtos_port_cycles_to_us(total_schedule_time / TEST_ITERATIONS);
    
    TEST_PRINT("  RR Enqueue: avg=%u us (target<%u)", avg_enqueue_us, MAX_ENQUEUE_US);
    TEST_PRINT("  RR Schedule: avg=%u us (target<%u)", avg_schedule_us, MAX_RR_SCHEDULE_US);
    
    TEST_ASSERT(avg_enqueue_us <= MAX_ENQUEUE_US, "RR enqueue too slow");
    TEST_ASSERT(avg_schedule_us <= MAX_RR_SCHEDULE_US, "RR schedule too slow");
    
    return true;
}

/**
 * @brief Benchmark Priority scheduler performance
 */
static bool benchmark_priority_performance(void)
{
    dsrtos_priority_scheduler_t* prio = &g_test_ctx.prio_scheduler;
    test_task_t* task;
    uint32_t i;
    uint32_t total_schedule_time = 0;
    uint32_t total_enqueue_time = 0;
    dsrtos_status_t status;
    
    TEST_PRINT("Benchmarking Priority scheduler performance...");
    
    status = dsrtos_priority_init(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority init failed");
    
    status = dsrtos_priority_start(prio);
    TEST_ASSERT(status == DSRTOS_SUCCESS, "Priority start failed");
    
    /* Prepare tasks with various priorities */
    for (i = 0; i < TEST_NUM_TASKS; i++) {
        task = &g_test_ctx.tasks[i];
        task->tcb.magic = TCB_MAGIC;
        task->tcb.tid = i + 1;
        task->tcb.state = TASK_STATE_READY;
        task->tcb.priority = (uint8_t)(i * 16);  /* Distribute across priorities */
    }
    
    /* Benchmark enqueue operations */
    for (i = 0; i < TEST_ITERATIONS; i++) {
        uint32_t task_idx = i % TEST_NUM_TASKS;
        task = &g_test_ctx.tasks[task_idx];
        
        uint32_t start = dsrtos_port_get_cycle_count();
        
        status = dsrtos_priority_enqueue(prio, &task->tcb, task->tcb.priority);
        
        uint32_t end = dsrtos_port_get_cycle_count();
        total_enqueue_time += (end - start);
        
        if (status != DSRTOS_SUCCESS) {
            /* Remove to make room */
            dsrtos_priority_select_next(prio);
        }
    }
    
    /* Benchmark O(1) schedule operations */
    for (i = 0; i < TEST_ITERATIONS; i++) {
        uint32_t start = dsrtos_port_get_cycle_count();
        
        dsrtos_tcb_t* selected = dsrtos_priority_select_next(prio);
        
        uint32_t end = dsrtos_port_get_cycle_count();
        total_schedule_time += (end - start);
        
        if (selected != NULL) {
            /* Re-enqueue for next iteration */
            dsrtos_priority_enqueue(prio, selected, selected->priority);
        }
    }
    
    /* Calculate averages */
    uint32_t avg_enqueue_us = dsrtos_port_cycles_to_us(total_enqueue_time / TEST_ITERATIONS);
    uint32_t avg_schedule_us = dsrtos_port_cycles_to_us(total_schedule_time / TEST_ITERATIONS);
    
    TEST_PRINT("  Priority Enqueue: avg=%u us (target<%u)", avg_enqueue_us, MAX_ENQUEUE_US);
    TEST_PRINT("  Priority Schedule: avg=%u us (target<%u)", avg_schedule_us, MAX_PRIO_SCHEDULE_US);
    
    TEST_ASSERT(avg_enqueue_us <= MAX_ENQUEUE_US, "Priority enqueue too slow");
    TEST_ASSERT(avg_schedule_us <= MAX_PRIO_SCHEDULE_US, "Priority schedule too slow");
    
    /* Verify O(1) property */
    TEST_PRINT("  Bitmap scans: %u (should be ~%u)", 
               prio->stats.bitmap_scans, TEST_ITERATIONS);
    
    return true;
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

/**
 * @brief Run all Phase 6 scheduler tests
 */
bool test_phase6_schedulers_run(void)
{
    uint32_t passed = 0;
    uint32_t failed = 0;
    
    TEST_PRINT("\n========================================");
    TEST_PRINT("PHASE 6: SCHEDULER IMPLEMENTATION TESTS");
    TEST_PRINT("========================================\n");
    
    /* Clear test context */
    memset(&g_test_ctx, 0, sizeof(g_test_ctx));
    
    /* Round Robin Tests */
    TEST_RUN(test_rr_init, &passed, &failed);
    TEST_RUN(test_rr_enqueue, &passed, &failed);
    TEST_RUN(test_rr_scheduling, &passed, &failed);
    TEST_RUN(test_rr_time_slicing, &passed, &failed);
    
    /* Priority Scheduler Tests */
    TEST_RUN(test_priority_init, &passed, &failed);
    TEST_RUN(test_priority_bitmap, &passed, &failed);
    TEST_RUN(test_priority_scheduling, &passed, &failed);
    TEST_RUN(test_priority_inheritance, &passed, &failed);
    TEST_RUN(test_priority_aging, &passed, &failed);
    
    /* Performance Benchmarks */
    TEST_RUN(benchmark_rr_performance, &passed, &failed);
    TEST_RUN(benchmark_priority_performance, &passed, &failed);
    
    /* Summary */
    TEST_PRINT("\n========================================");
    TEST_PRINT("TEST SUMMARY: %u passed, %u failed", passed, failed);
    TEST_PRINT("========================================\n");
    
    return (failed == 0);
}
