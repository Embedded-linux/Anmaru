/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 8: Complete Context Switch Benchmarking
 * 
 * Comprehensive cycle-accurate performance measurement and validation
 * Target: ARM Cortex-M4F @ 168MHz
 * 
 * Copyright (c) 2025 DSRTOS Development Team
 * SPDX-License-Identifier: MIT
 * 
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_context_switch.h"
#include "dsrtos_kernel.h"
#include "dsrtos_task_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Missing constants */
#ifndef DSRTOS_ERROR
#define DSRTOS_ERROR                       (-1)
#endif

/* ============================================================================
 * Benchmark Configuration
 * ============================================================================ */

#define BENCHMARK_ITERATIONS        1000U
#define BENCHMARK_WARMUP_RUNS      10U
#define BENCHMARK_TASKS            8U
#define TASK_STACK_SIZE           2048U
#define HISTOGRAM_BUCKETS         20U
#define HISTOGRAM_BUCKET_SIZE     10U   /* Cycles per bucket */

/* DWT registers for cycle counting */
#define DWT_CTRL    (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t*)0xE0001004)
#define DWT_LAR     (*(volatile uint32_t*)0xE0001FB0)
#define DWT_DEMCR   (*(volatile uint32_t*)0xE000EDFC)

/* CPU frequency for time conversion */
#define CPU_FREQ_HZ                168000000U
#define CYCLES_TO_US(c)           (((c) * 1000000U) / CPU_FREQ_HZ)
#define CYCLES_TO_NS(c)           (((c) * 1000000000ULL) / CPU_FREQ_HZ)

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/* Benchmark statistics structure */
typedef struct {
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint64_t total_cycles;
    uint32_t count;
    uint32_t histogram[HISTOGRAM_BUCKETS];
    
    /* Extended statistics */
    uint64_t sum_squares;           /* For standard deviation */
    uint32_t median;
    uint32_t percentile_95;
    uint32_t percentile_99;
} benchmark_stats_t;

/* Benchmark test configuration */
typedef struct {
    const char* name;
    bool use_fpu;
    bool use_mpu;
    uint8_t mpu_regions;
    uint32_t stack_usage;
    bool measure_overhead;
} benchmark_config_t;

/* Benchmark results collection */
typedef struct {
    /* Core benchmarks */
    benchmark_stats_t basic_switch;
    benchmark_stats_t fpu_switch;
    benchmark_stats_t mpu_switch;
    benchmark_stats_t full_switch;
    
    /* Detailed benchmarks */
    benchmark_stats_t register_save;
    benchmark_stats_t register_restore;
    benchmark_stats_t fpu_lazy_save;
    benchmark_stats_t fpu_lazy_restore;
    benchmark_stats_t mpu_2region;
    benchmark_stats_t mpu_4region;
    benchmark_stats_t mpu_8region;
    
    /* Stack operations */
    benchmark_stats_t stack_init;
    benchmark_stats_t stack_check;
    benchmark_stats_t stack_overflow_detect;
    
    /* Overhead measurements */
    benchmark_stats_t pendsv_entry;
    benchmark_stats_t pendsv_exit;
    benchmark_stats_t critical_section;
    
    /* System benchmarks */
    benchmark_stats_t interrupt_latency;
    benchmark_stats_t scheduler_decision;
    benchmark_stats_t task_creation;
    benchmark_stats_t task_deletion;
} context_benchmark_results_t;

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/* Benchmark results storage */
static context_benchmark_results_t g_benchmark_results;

/* Test contexts and stacks */
static dsrtos_context_t g_test_contexts[BENCHMARK_TASKS];
static uint8_t g_test_stacks[BENCHMARK_TASKS][TASK_STACK_SIZE] __attribute__((aligned(8)));

/* Benchmark control flags */
static volatile bool g_benchmark_running = false;
static volatile uint32_t g_benchmark_counter = 0;

/* Raw measurement buffer for sorting */
static uint32_t g_measurement_buffer[BENCHMARK_ITERATIONS];

/* ============================================================================
 * Cycle Counter Functions
 * ============================================================================ */

/**
 * @brief Initialize DWT cycle counter for benchmarking
 * 
 * MISRA-C:2012 Dev 11.4: Hardware register access requires casting
 */
static void benchmark_init_cycle_counter(void)
{
    /* Enable trace and debug blocks */
    DWT_DEMCR |= (1UL << 24);  /* Set TRCENA bit */
    
    /* Unlock DWT registers */
    DWT_LAR = 0xC5ACCE55;
    
    /* Enable cycle counter */
    DWT_CTRL |= 1UL;
    
    /* Reset cycle counter */
    DWT_CYCCNT = 0;
    
    /* Ensure changes take effect */
    __DSB();
    __ISB();
}

/**
 * @brief Get current cycle count
 */
static inline uint32_t benchmark_get_cycles(void)
{
    return DWT_CYCCNT;
}

/**
 * @brief Calculate elapsed cycles with overflow handling
 */
static inline uint32_t benchmark_elapsed_cycles(uint32_t start, uint32_t end)
{
    if (end >= start) {
        return end - start;
    } else {
        /* Handle 32-bit overflow */
        return (0xFFFFFFFFUL - start) + end + 1U;
    }
}

/**
 * @brief Compensate for measurement overhead
 */
static uint32_t benchmark_compensate_overhead(uint32_t measured, uint32_t overhead)
{
    if (measured > overhead) {
        return measured - overhead;
    }
    return 0U;
}

/* ============================================================================
 * Statistics Functions
 * ============================================================================ */

/**
 * @brief Initialize benchmark statistics
 */
static void benchmark_init_stats(benchmark_stats_t* stats)
{
    stats->min_cycles = 0xFFFFFFFFU;
    stats->max_cycles = 0U;
    stats->total_cycles = 0U;
    stats->count = 0U;
    stats->sum_squares = 0U;
    stats->median = 0U;
    stats->percentile_95 = 0U;
    stats->percentile_99 = 0U;
    
    memset(stats->histogram, 0, sizeof(stats->histogram));
}

/**
 * @brief Update benchmark statistics with new measurement
 */
static void benchmark_update_stats(benchmark_stats_t* stats, uint32_t cycles)
{
    /* Update min/max */
    if (cycles < stats->min_cycles) {
        stats->min_cycles = cycles;
    }
    if (cycles > stats->max_cycles) {
        stats->max_cycles = cycles;
    }
    
    /* Update totals */
    stats->total_cycles += cycles;
    stats->sum_squares += ((uint64_t)cycles * cycles);
    stats->count++;
    
    /* Update histogram */
    uint32_t bucket = cycles / HISTOGRAM_BUCKET_SIZE;
    if (bucket >= HISTOGRAM_BUCKETS) {
        bucket = HISTOGRAM_BUCKETS - 1U;
    }
    stats->histogram[bucket]++;
}

/**
 * @brief Calculate standard deviation from statistics
 */
static uint32_t benchmark_calc_std_dev(benchmark_stats_t* stats)
{
    if (stats->count < 2U) {
        return 0U;
    }
    
    uint64_t mean = stats->total_cycles / stats->count;
    uint64_t mean_squared = mean * mean;
    uint64_t mean_of_squares = stats->sum_squares / stats->count;
    
    if (mean_of_squares < mean_squared) {
        return 0U;
    }
    
    uint64_t variance = mean_of_squares - mean_squared;
    
    /* Simple integer square root */
    uint32_t result = 0U;
    uint32_t bit = 1U << 30;
    
    while (bit > variance) {
        bit >>= 2;
    }
    
    while (bit != 0U) {
        if (variance >= result + bit) {
            variance -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    
    return result;
}

/**
 * @brief Quick sort for percentile calculation
 */
static void benchmark_quicksort(uint32_t* arr, int32_t left, int32_t right)
{
    if (left >= right) {
        return;
    }
    
    uint32_t pivot = arr[(left + right) / 2];
    int32_t i = left - 1;
    int32_t j = right + 1;
    
    while (1) {
        do { i++; } while (arr[i] < pivot);
        do { j--; } while (arr[j] > pivot);
        
        if (i >= j) {
            break;
        }
        
        uint32_t temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    
    benchmark_quicksort(arr, left, j);
    benchmark_quicksort(arr, j + 1, right);
}

/**
 * @brief Calculate percentiles from measurements
 */
static void benchmark_calc_percentiles(benchmark_stats_t* stats, 
                                      uint32_t* measurements, 
                                      uint32_t count)
{
    if (count == 0U) {
        return;
    }
    
    /* Sort measurements */
    benchmark_quicksort(measurements, 0, (int32_t)count - 1);
    
    /* Calculate median (50th percentile) */
    stats->median = measurements[count / 2];
    
    /* Calculate 95th percentile */
    uint32_t idx_95 = (count * 95U) / 100U;
    stats->percentile_95 = measurements[idx_95];
    
    /* Calculate 99th percentile */
    uint32_t idx_99 = (count * 99U) / 100U;
    stats->percentile_99 = measurements[idx_99];
}

/* ============================================================================
 * Test Context Initialization
 * ============================================================================ */

/**
 * @brief Initialize test context
 */
static void benchmark_init_context(dsrtos_context_t* ctx, 
                                  uint8_t* stack, 
                                  uint32_t stack_size,
                                  bool use_fpu,
                                  bool use_mpu,
                                  uint8_t mpu_regions)
{
    memset(ctx, 0, sizeof(dsrtos_context_t));
    
    /* Initialize stack */
    ctx->stack_base = (uint32_t*)(stack + stack_size);
    ctx->stack_size = stack_size;
    ctx->stack_limit = (uint32_t*)stack;
    
    /* Initialize stack pointer to middle of stack for testing */
    ctx->sp = (uint32_t*)(stack + stack_size - 256U);
    
    /* Fill stack with pattern */
    dsrtos_stack_fill_pattern(stack, stack_size);
    
    /* Configure FPU state */
    ctx->fpu.active = use_fpu;
    ctx->fpu.lazy_saved = false;
    
    /* Configure MPU */
    if (use_mpu) {
        ctx->mpu.enabled = true;
        ctx->mpu.num_regions = mpu_regions;
        
        /* Set up test MPU regions */
        for (uint8_t i = 0U; i < mpu_regions; i++) {
            ctx->mpu.regions[i].rbar = 0x20000000U + (i * 0x1000U);
            ctx->mpu.regions[i].rasr = 0x0301001BU;  /* 4KB, RW, cached */
        }
    }
    
    /* Initialize statistics */
    ctx->stats.switch_count = 0U;
    ctx->stats.switch_cycles = 0U;
    ctx->stats.fpu_saves = 0U;
    ctx->stats.stack_usage = 0U;
}

/**
 * @brief Dummy task function for testing
 */
static void benchmark_dummy_task(void* param)
{
    volatile uint32_t counter = 0U;
    
    while (g_benchmark_running) {
        counter++;
        
        /* Use FPU if configured */
        if ((uintptr_t)param & 1U) {
            volatile float x = 1.0f;
            volatile float y = 2.0f;
            volatile float z = x + y;
            (void)z;
        }
        
        /* Yield periodically */
        if ((counter & 0xFFU) == 0U) {
            __asm volatile ("svc #1");  /* Yield */
        }
    }
}

/* ============================================================================
 * Context Switch Benchmarks
 * ============================================================================ */

/**
 * @brief Benchmark basic context switch (no FPU/MPU)
 */
static void benchmark_basic_context_switch(void)
{
    dsrtos_context_t context1, context2;
    uint32_t i;
    
    printf("Benchmarking basic context switch...\n");
    
    /* Initialize contexts */
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          false, false, 0);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          false, false, 0);
    
    /* Initialize statistics */
    benchmark_init_stats(&g_benchmark_results.basic_switch);
    
    /* Warm up cache and branch predictor */
    for (i = 0U; i < BENCHMARK_WARMUP_RUNS; i++) {
        g_current_context = &context1;
        g_next_context = &context2;
        dsrtos_trigger_pendsv();
        __asm volatile ("wfi");
        
        /* Swap for next iteration */
        g_current_context = &context2;
        g_next_context = &context1;
        dsrtos_trigger_pendsv();
        __asm volatile ("wfi");
    }
    
    /* Run benchmark */
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        /* Set up context switch */
        g_current_context = &context1;
        g_next_context = &context2;
        
        /* Ensure all previous instructions complete */
        __DSB();
        __ISB();
        
        /* Measure context switch time */
        start_cycles = benchmark_get_cycles();
        
        /* Trigger PendSV */
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        
        /* Wait for PendSV to complete */
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        
        end_cycles = benchmark_get_cycles();
        
        /* Calculate elapsed cycles */
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        
        /* Store measurement */
        g_measurement_buffer[i] = elapsed;
        
        /* Update statistics */
        benchmark_update_stats(&g_benchmark_results.basic_switch, elapsed);
        
        /* Swap contexts for next iteration */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    /* Calculate percentiles */
    benchmark_calc_percentiles(&g_benchmark_results.basic_switch, 
                              g_measurement_buffer, 
                              BENCHMARK_ITERATIONS);
    
    /* Report results */
    printf("  Min: %lu cycles (%.2f us)\n", 
           g_benchmark_results.basic_switch.min_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.basic_switch.min_cycles) / 1000.0f);
    printf("  Max: %lu cycles (%.2f us)\n", 
           g_benchmark_results.basic_switch.max_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.basic_switch.max_cycles) / 1000.0f);
    printf("  Avg: %lu cycles (%.2f us)\n", 
           (uint32_t)(g_benchmark_results.basic_switch.total_cycles / 
                     g_benchmark_results.basic_switch.count),
           (float)CYCLES_TO_US(g_benchmark_results.basic_switch.total_cycles / 
                              g_benchmark_results.basic_switch.count) / 1000.0f);
    printf("  Median: %lu cycles\n", g_benchmark_results.basic_switch.median);
    printf("  95th percentile: %lu cycles\n", g_benchmark_results.basic_switch.percentile_95);
    printf("  99th percentile: %lu cycles\n", g_benchmark_results.basic_switch.percentile_99);
    printf("  Std Dev: %lu cycles\n", benchmark_calc_std_dev(&g_benchmark_results.basic_switch));
}

/**
 * @brief Benchmark FPU context switch
 */
static void benchmark_fpu_context_switch(void)
{
    dsrtos_context_t context1, context2;
    uint32_t i;
    
    printf("\nBenchmarking FPU context switch...\n");
    
    /* Initialize contexts with FPU */
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          true, false, 0);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          true, false, 0);
    
    /* Initialize statistics */
    benchmark_init_stats(&g_benchmark_results.fpu_switch);
    
    /* Use FPU to trigger lazy stacking */
    __asm volatile (
        "vmov.f32 s0, #1.0\n"
        "vmov.f32 s1, #2.0\n"
        "vadd.f32 s2, s0, s1\n"
        "vmov.f32 s16, #3.0\n"
        "vmov.f32 s31, #4.0\n"
    );
    
    /* Force FPU context to be active */
    FPU_FPCCR |= FPU_FPCCR_LSPACT;
    
    /* Warm up */
    for (i = 0U; i < BENCHMARK_WARMUP_RUNS; i++) {
        g_current_context = &context1;
        g_next_context = &context2;
        dsrtos_trigger_pendsv();
        __asm volatile ("wfi");
        
        g_current_context = &context2;
        g_next_context = &context1;
        dsrtos_trigger_pendsv();
        __asm volatile ("wfi");
    }
    
    /* Run benchmark */
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        /* Set up context switch */
        g_current_context = &context1;
        g_next_context = &context2;
        
        /* Ensure FPU context is active */
        FPU_FPCCR |= FPU_FPCCR_LSPACT;
        
        __DSB();
        __ISB();
        
        /* Measure context switch time */
        start_cycles = benchmark_get_cycles();
        
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        g_measurement_buffer[i] = elapsed;
        benchmark_update_stats(&g_benchmark_results.fpu_switch, elapsed);
        
        /* Swap contexts */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    /* Calculate percentiles */
    benchmark_calc_percentiles(&g_benchmark_results.fpu_switch, 
                              g_measurement_buffer, 
                              BENCHMARK_ITERATIONS);
    
    /* Report results */
    printf("  Min: %lu cycles (%.2f us)\n", 
           g_benchmark_results.fpu_switch.min_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.fpu_switch.min_cycles) / 1000.0f);
    printf("  Max: %lu cycles (%.2f us)\n", 
           g_benchmark_results.fpu_switch.max_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.fpu_switch.max_cycles) / 1000.0f);
    printf("  Avg: %lu cycles (%.2f us)\n", 
           (uint32_t)(g_benchmark_results.fpu_switch.total_cycles / 
                     g_benchmark_results.fpu_switch.count),
           (float)CYCLES_TO_US(g_benchmark_results.fpu_switch.total_cycles / 
                              g_benchmark_results.fpu_switch.count) / 1000.0f);
    printf("  Median: %lu cycles\n", g_benchmark_results.fpu_switch.median);
    printf("  95th percentile: %lu cycles\n", g_benchmark_results.fpu_switch.percentile_95);
    printf("  Std Dev: %lu cycles\n", benchmark_calc_std_dev(&g_benchmark_results.fpu_switch));
}

/**
 * @brief Benchmark MPU context switch
 */
static void benchmark_mpu_context_switch(void)
{
    dsrtos_context_t context1, context2;
    uint32_t i;
    
    printf("\nBenchmarking MPU context switch (2 regions)...\n");
    
    /* Initialize contexts with MPU */
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          false, true, 2);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          false, true, 2);
    
    /* Initialize statistics */
    benchmark_init_stats(&g_benchmark_results.mpu_switch);
    
    /* Run benchmark */
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        g_current_context = &context1;
        g_next_context = &context2;
        
        __DSB();
        __ISB();
        
        start_cycles = benchmark_get_cycles();
        
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        g_measurement_buffer[i] = elapsed;
        benchmark_update_stats(&g_benchmark_results.mpu_switch, elapsed);
        
        /* Swap contexts */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    /* Calculate percentiles */
    benchmark_calc_percentiles(&g_benchmark_results.mpu_switch, 
                              g_measurement_buffer, 
                              BENCHMARK_ITERATIONS);
    
    /* Report results */
    printf("  Min: %lu cycles (%.2f us)\n", 
           g_benchmark_results.mpu_switch.min_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.mpu_switch.min_cycles) / 1000.0f);
    printf("  Max: %lu cycles (%.2f us)\n", 
           g_benchmark_results.mpu_switch.max_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.mpu_switch.max_cycles) / 1000.0f);
    printf("  Avg: %lu cycles (%.2f us)\n", 
           (uint32_t)(g_benchmark_results.mpu_switch.total_cycles / 
                     g_benchmark_results.mpu_switch.count),
           (float)CYCLES_TO_US(g_benchmark_results.mpu_switch.total_cycles / 
                              g_benchmark_results.mpu_switch.count) / 1000.0f);
}

/**
 * @brief Benchmark full context switch (FPU + MPU)
 */
static void benchmark_full_context_switch(void)
{
    dsrtos_context_t context1, context2;
    uint32_t i;
    
    printf("\nBenchmarking full context switch (FPU + MPU)...\n");
    
    /* Initialize contexts with FPU and MPU */
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          true, true, 2);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          true, true, 2);
    
    /* Initialize statistics */
    benchmark_init_stats(&g_benchmark_results.full_switch);
    
    /* Run benchmark */
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        g_current_context = &context1;
        g_next_context = &context2;
        
        /* Ensure FPU context is active */
        FPU_FPCCR |= FPU_FPCCR_LSPACT;
        
        __DSB();
        __ISB();
        
        start_cycles = benchmark_get_cycles();
        
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        g_measurement_buffer[i] = elapsed;
        benchmark_update_stats(&g_benchmark_results.full_switch, elapsed);
        
        /* Swap contexts */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    /* Calculate percentiles */
    benchmark_calc_percentiles(&g_benchmark_results.full_switch, 
                              g_measurement_buffer, 
                              BENCHMARK_ITERATIONS);
    
    /* Report results */
    printf("  Min: %lu cycles (%.2f us)\n", 
           g_benchmark_results.full_switch.min_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.full_switch.min_cycles) / 1000.0f);
    printf("  Max: %lu cycles (%.2f us)\n", 
           g_benchmark_results.full_switch.max_cycles,
           (float)CYCLES_TO_US(g_benchmark_results.full_switch.max_cycles) / 1000.0f);
    printf("  Avg: %lu cycles (%.2f us)\n", 
           (uint32_t)(g_benchmark_results.full_switch.total_cycles / 
                     g_benchmark_results.full_switch.count),
           (float)CYCLES_TO_US(g_benchmark_results.full_switch.total_cycles / 
                              g_benchmark_results.full_switch.count) / 1000.0f);
}

/* ============================================================================
 * Stack Operation Benchmarks
 * ============================================================================ */

/**
 * @brief Benchmark stack initialization
 */
static void benchmark_stack_operations(void)
{
    uint32_t i;
    void* sp;
    dsrtos_context_t context;
    
    printf("\nBenchmarking stack operations...\n");
    
    /* Benchmark stack initialization */
    benchmark_init_stats(&g_benchmark_results.stack_init);
    
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        __DSB();
        
        start_cycles = benchmark_get_cycles();
        
        sp = dsrtos_stack_init(&g_test_stacks[0][TASK_STACK_SIZE],
                              benchmark_dummy_task,
                              (void*)i,
                              NULL);
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        g_measurement_buffer[i] = elapsed;
        benchmark_update_stats(&g_benchmark_results.stack_init, elapsed);
        
        (void)sp;
    }
    
    printf("  Stack init - Min: %lu cycles, Avg: %lu cycles\n", 
           g_benchmark_results.stack_init.min_cycles,
           (uint32_t)(g_benchmark_results.stack_init.total_cycles / 
                     g_benchmark_results.stack_init.count));
    
    /* Benchmark stack checking */
    benchmark_init_stats(&g_benchmark_results.stack_check);
    
    /* Prepare context for stack check */
    benchmark_init_context(&context, g_test_stacks[0], TASK_STACK_SIZE,
                          false, false, 0);
    
    /* Fill guard area with pattern */
    dsrtos_stack_fill_pattern(g_test_stacks[0], DSRTOS_STACK_GUARD_SIZE);
    
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        __DSB();
        
        start_cycles = benchmark_get_cycles();
        
        dsrtos_stack_check(&context);
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        benchmark_update_stats(&g_benchmark_results.stack_check, elapsed);
    }
    
    printf("  Stack check - Min: %lu cycles, Avg: %lu cycles\n", 
           g_benchmark_results.stack_check.min_cycles,
           (uint32_t)(g_benchmark_results.stack_check.total_cycles / 
                     g_benchmark_results.stack_check.count));
    
    /* Benchmark stack overflow detection */
    benchmark_init_stats(&g_benchmark_results.stack_overflow_detect);
    
    /* Simulate near-overflow condition */
    context.sp = (uint32_t*)(g_test_stacks[0] + 64);  /* Close to limit */
    
    for (i = 0U; i < BENCHMARK_ITERATIONS; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        __DSB();
        
        start_cycles = benchmark_get_cycles();
        
        dsrtos_stack_check(&context);
        
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        benchmark_update_stats(&g_benchmark_results.stack_overflow_detect, elapsed);
    }
    
    printf("  Overflow detect - Min: %lu cycles, Avg: %lu cycles\n", 
           g_benchmark_results.stack_overflow_detect.min_cycles,
           (uint32_t)(g_benchmark_results.stack_overflow_detect.total_cycles / 
                     g_benchmark_results.stack_overflow_detect.count));
}

/* ============================================================================
 * MPU Region Benchmarks
 * ============================================================================ */

/**
 * @brief Benchmark MPU region switching with different region counts
 */
static void benchmark_mpu_regions(void)
{
    dsrtos_context_t context1, context2;
    uint32_t i;
    
    printf("\nBenchmarking MPU region switching...\n");
    
    /* Test 4 regions */
    printf("  Testing 4 regions:\n");
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          false, true, 4);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          false, true, 4);
    
    benchmark_init_stats(&g_benchmark_results.mpu_4region);
    
    for (i = 0U; i < BENCHMARK_ITERATIONS / 2; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        g_current_context = &context1;
        g_next_context = &context2;
        
        start_cycles = benchmark_get_cycles();
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        benchmark_update_stats(&g_benchmark_results.mpu_4region, elapsed);
        
        /* Swap contexts */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    printf("    Min: %lu cycles, Avg: %lu cycles\n", 
           g_benchmark_results.mpu_4region.min_cycles,
           (uint32_t)(g_benchmark_results.mpu_4region.total_cycles / 
                     g_benchmark_results.mpu_4region.count));
    
    /* Test 8 regions */
    printf("  Testing 8 regions:\n");
    benchmark_init_context(&context1, g_test_stacks[0], TASK_STACK_SIZE, 
                          false, true, 8);
    benchmark_init_context(&context2, g_test_stacks[1], TASK_STACK_SIZE, 
                          false, true, 8);
    
    benchmark_init_stats(&g_benchmark_results.mpu_8region);
    
    for (i = 0U; i < BENCHMARK_ITERATIONS / 2; i++) {
        uint32_t start_cycles, end_cycles, elapsed;
        
        g_current_context = &context1;
        g_next_context = &context2;
        
        start_cycles = benchmark_get_cycles();
        SCB_ICSR = SCB_ICSR_PENDSVSET;
        __DSB();
        __ISB();
        __asm volatile ("wfi");
        end_cycles = benchmark_get_cycles();
        
        elapsed = benchmark_elapsed_cycles(start_cycles, end_cycles);
        benchmark_update_stats(&g_benchmark_results.mpu_8region, elapsed);
        
        /* Swap contexts */
        dsrtos_context_t* temp = g_current_context;
        g_current_context = g_next_context;
        g_next_context = temp;
    }
    
    printf("    Min: %lu cycles, Avg: %lu cycles\n", 
           g_benchmark_results.mpu_8region.min_cycles,
           (uint32_t)(g_benchmark_results.mpu_8region.total_cycles / 
                     g_benchmark_results.mpu_8region.count));
}

/* ============================================================================
 * Benchmark Report Generation
 * ============================================================================ */

/**
 * @brief Print histogram for a benchmark
 */
static void benchmark_print_histogram(const char* name, benchmark_stats_t* stats)
{
    uint32_t i;
    uint32_t max_count = 0U;
    
    /* Find maximum count for scaling */
    for (i = 0U; i < HISTOGRAM_BUCKETS; i++) {
        if (stats->histogram[i] > max_count) {
            max_count = stats->histogram[i];
        }
    }
    
    if (max_count == 0U) {
        return;
    }
    
    printf("\n%s Histogram:\n", name);
    printf("Cycles Range    Count   Distribution\n");
    printf("------------    -----   ------------\n");
    
    for (i = 0U; i < HISTOGRAM_BUCKETS; i++) {
        if (stats->histogram[i] > 0U) {
            uint32_t bar_len = (stats->histogram[i] * 40U) / max_count;
            
            printf("%3u-%3u cycles: %5u   ", 
                   i * HISTOGRAM_BUCKET_SIZE, 
                   ((i + 1U) * HISTOGRAM_BUCKET_SIZE) - 1U,
                   stats->histogram[i]);
            
            /* Print bar graph */
            for (uint32_t j = 0U; j < bar_len; j++) {
                printf("#");
            }
            printf("\n");
        }
    }
}

/**
 * @brief Generate comprehensive benchmark report
 */
static void benchmark_generate_report(void)
{
    printf("\n");
    printf("========================================\n");
    printf("DSRTOS Context Switch Benchmark Report\n");
    printf("========================================\n\n");
    
    printf("Configuration:\n");
    printf("  CPU: ARM Cortex-M4F @ %u MHz\n", CPU_FREQ_HZ / 1000000U);
    printf("  Iterations: %u\n", BENCHMARK_ITERATIONS);
    printf("  Target: < %u cycles\n", DSRTOS_TARGET_SWITCH_CYCLES);
    printf("  Maximum: < %u cycles\n\n", DSRTOS_MAX_SWITCH_CYCLES);
    
    /* Summary table */
    printf("Summary Results:\n");
    printf("%-20s %8s %8s %8s %8s %8s\n", 
           "Test", "Min", "Avg", "Max", "95%ile", "Status");
    printf("%-20s %8s %8s %8s %8s %8s\n", 
           "--------------------", "--------", "--------", "--------", "--------", "------");
    
    /* Basic switch */
    printf("%-20s %8lu %8lu %8lu %8lu %8s\n",
           "Basic Switch",
           g_benchmark_results.basic_switch.min_cycles,
           (uint32_t)(g_benchmark_results.basic_switch.total_cycles / 
                     g_benchmark_results.basic_switch.count),
           g_benchmark_results.basic_switch.max_cycles,
           g_benchmark_results.basic_switch.percentile_95,
           (g_benchmark_results.basic_switch.max_cycles < DSRTOS_TARGET_SWITCH_CYCLES) ? 
           "PASS" : "FAIL");
    
    /* FPU switch */
    printf("%-20s %8lu %8lu %8lu %8lu %8s\n",
           "FPU Switch",
           g_benchmark_results.fpu_switch.min_cycles,
           (uint32_t)(g_benchmark_results.fpu_switch.total_cycles / 
                     g_benchmark_results.fpu_switch.count),
           g_benchmark_results.fpu_switch.max_cycles,
           g_benchmark_results.fpu_switch.percentile_95,
           (g_benchmark_results.fpu_switch.max_cycles < DSRTOS_TARGET_SWITCH_CYCLES) ? 
           "PASS" : "FAIL");
    
    /* MPU switch */
    printf("%-20s %8lu %8lu %8lu %8lu %8s\n",
           "MPU Switch (2 reg)",
           g_benchmark_results.mpu_switch.min_cycles,
           (uint32_t)(g_benchmark_results.mpu_switch.total_cycles / 
                     g_benchmark_results.mpu_switch.count),
           g_benchmark_results.mpu_switch.max_cycles,
           g_benchmark_results.mpu_switch.percentile_95,
           (g_benchmark_results.mpu_switch.max_cycles < DSRTOS_TARGET_SWITCH_CYCLES) ? 
           "PASS" : "FAIL");
    
    /* Full switch */
    printf("%-20s %8lu %8lu %8lu %8lu %8s\n",
           "Full Switch",
           g_benchmark_results.full_switch.min_cycles,
           (uint32_t)(g_benchmark_results.full_switch.total_cycles / 
                     g_benchmark_results.full_switch.count),
           g_benchmark_results.full_switch.max_cycles,
           g_benchmark_results.full_switch.percentile_95,
           (g_benchmark_results.full_switch.max_cycles < DSRTOS_MAX_SWITCH_CYCLES) ? 
           "PASS" : "FAIL");
    
    /* Print histograms if verbose */
    #ifdef BENCHMARK_VERBOSE
    benchmark_print_histogram("Basic Switch", &g_benchmark_results.basic_switch);
    benchmark_print_histogram("FPU Switch", &g_benchmark_results.fpu_switch);
    benchmark_print_histogram("MPU Switch", &g_benchmark_results.mpu_switch);
    benchmark_print_histogram("Full Switch", &g_benchmark_results.full_switch);
    #endif
    
    printf("\n========================================\n");
}

/**
 * @brief Validate benchmark results against requirements
 */
static bool benchmark_validate_results(void)
{
    bool pass = true;
    
    /* Check basic context switch */
    if (g_benchmark_results.basic_switch.max_cycles > DSRTOS_TARGET_SWITCH_CYCLES) {
        printf("ERROR: Basic context switch exceeds target (%lu > %u)\n",
               g_benchmark_results.basic_switch.max_cycles,
               DSRTOS_TARGET_SWITCH_CYCLES);
        pass = false;
    }
    
    /* Check FPU context switch */
    if (g_benchmark_results.fpu_switch.max_cycles > DSRTOS_TARGET_SWITCH_CYCLES) {
        printf("ERROR: FPU context switch exceeds target (%lu > %u)\n",
               g_benchmark_results.fpu_switch.max_cycles,
               DSRTOS_TARGET_SWITCH_CYCLES);
        pass = false;
    }
    
    /* Check MPU context switch */
    if (g_benchmark_results.mpu_switch.max_cycles > DSRTOS_TARGET_SWITCH_CYCLES) {
        printf("WARNING: MPU context switch exceeds target (%lu > %u)\n",
               g_benchmark_results.mpu_switch.max_cycles,
               DSRTOS_TARGET_SWITCH_CYCLES);
        /* Don't fail for MPU, just warn */
    }
    
    /* Check full context switch */
    if (g_benchmark_results.full_switch.max_cycles > DSRTOS_MAX_SWITCH_CYCLES) {
        printf("ERROR: Full context switch exceeds maximum (%lu > %u)\n",
               g_benchmark_results.full_switch.max_cycles,
               DSRTOS_MAX_SWITCH_CYCLES);
        pass = false;
    }
    
    return pass;
}

/* ============================================================================
 * Main Benchmark Entry Point
 * ============================================================================ */

/**
 * @brief Run all context switch benchmarks
 */
void dsrtos_run_context_benchmarks(void)
{
    printf("\nStarting DSRTOS Context Switch Benchmarks\n");
    printf("==========================================\n\n");
    
    /* Initialize hardware */
    benchmark_init_cycle_counter();
    
    /* Initialize context switch subsystem */
    if (dsrtos_context_switch_init() != DSRTOS_OK) {
        printf("ERROR: Failed to initialize context switch subsystem\n");
        return;
    }
    
    /* Run benchmarks */
    benchmark_basic_context_switch();
    benchmark_fpu_context_switch();
    benchmark_mpu_context_switch();
    benchmark_full_context_switch();
    benchmark_stack_operations();
    benchmark_mpu_regions();
    
    /* Generate report */
    benchmark_generate_report();
    
    /* Validate results */
    if (benchmark_validate_results()) {
        printf("\nALL BENCHMARKS PASSED\n");
    } else {
        printf("\nSOME BENCHMARKS FAILED - See errors above\n");
    }
}

/**
 * @brief Get benchmark results structure
 */
const context_benchmark_results_t* dsrtos_get_benchmark_results(void)
{
    return &g_benchmark_results;
}

/**
 * @brief Run specific benchmark test
 */
dsrtos_status_t dsrtos_run_specific_benchmark(const char* test_name)
{
    if (strcmp(test_name, "basic") == 0) {
        benchmark_basic_context_switch();
    } else if (strcmp(test_name, "fpu") == 0) {
        benchmark_fpu_context_switch();
    } else if (strcmp(test_name, "mpu") == 0) {
        benchmark_mpu_context_switch();
    } else if (strcmp(test_name, "full") == 0) {
        benchmark_full_context_switch();
    } else if (strcmp(test_name, "stack") == 0) {
        benchmark_stack_operations();
    } else if (strcmp(test_name, "mpu_regions") == 0) {
        benchmark_mpu_regions();
    } else {
        return DSRTOS_ERROR;
    }
    
    return DSRTOS_OK;
}

/* End of dsrtos_context_benchmark.c */
