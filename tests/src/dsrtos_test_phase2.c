/**
 * @file dsrtos_test_phase2.c
 * @brief DSRTOS Phase 2 Test Cases - Memory Management, Critical Sections, Assertions
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file contains comprehensive test cases for Phase 2 functionality
 * following MISRA-C:2012 standards and DO-178C DAL-B requirements.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"
#include "dsrtos_memory.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_heap.h"
#include "dsrtos_stack.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test timeouts */
#define PHASE2_TEST_TIMEOUT_MS       1000U
#define PHASE2_MEMORY_TEST_SIZE      1024U
#define PHASE2_HEAP_TEST_SIZE        2048U
#define PHASE2_STACK_TEST_SIZE       512U

/* Test patterns */
#define PHASE2_MEMORY_PATTERN_1      0xAAAAAAAAU
#define PHASE2_MEMORY_PATTERN_2      0x55555555U
#define PHASE2_MEMORY_PATTERN_3      0x12345678U
#define PHASE2_MEMORY_PATTERN_4      0x87654321U

/* Test iterations */
#define PHASE2_CRITICAL_TEST_ITER    100U
#define PHASE2_MEMORY_TEST_ITER      50U

/*==============================================================================
 *                            STATIC VARIABLES
 *============================================================================*/

/* Test suite instance */
static dsrtos_test_suite_t g_phase2_test_suite;

/* Test case array */
static dsrtos_test_case_t g_phase2_test_cases[] = {
    {"test_memory_initialization", test_memory_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_memory_allocation", test_memory_allocation, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_memory_deallocation", test_memory_deallocation, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_memory_alignment", test_memory_alignment, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_memory_integrity", test_memory_integrity, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_heap_initialization", test_heap_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_heap_allocation", test_heap_allocation, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_heap_fragmentation", test_heap_fragmentation, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_stack_initialization", test_stack_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_stack_operations", test_stack_operations, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_critical_section_enter", test_critical_section_enter, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_critical_section_exit", test_critical_section_exit, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_critical_section_nested", test_critical_section_nested, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_assert_condition", test_assert_condition, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_assert_range", test_assert_range, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_assert_pointer", test_assert_pointer, DSRTOS_TEST_TIMEOUT_MS(PHASE2_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL}
};

/* Test memory blocks */
static uint8_t g_test_memory[PHASE2_MEMORY_TEST_SIZE];
static uint8_t g_test_heap[PHASE2_HEAP_TEST_SIZE];
static uint8_t g_test_stack[PHASE2_STACK_TEST_SIZE];

/* Test counters */
static volatile uint32_t g_critical_enter_count = 0U;
static volatile uint32_t g_critical_exit_count = 0U;
static volatile uint32_t g_assert_fail_count = 0U;

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Test assertion handler
 * @param file Source file name
 * @param line Line number
 * @param condition Failed condition
 */
static void test_assert_handler(const char *file, uint32_t line, const char *condition)
{
    g_assert_fail_count++;
    /* This would normally trigger system reset or error handling */
}

/**
 * @brief Test memory pattern verification
 * @param ptr Pointer to memory
 * @param size Size in bytes
 * @param pattern Expected pattern
 * @return true if pattern matches, false otherwise
 */
static bool verify_memory_pattern(const void *ptr, size_t size, uint32_t pattern)
{
    const uint32_t *word_ptr = (const uint32_t *)ptr;
    const uint8_t *byte_ptr;
    uint32_t i;
    
    if (ptr == NULL || size == 0U) {
        return false;
    }
    
    /* Check word pattern */
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

/*==============================================================================
 *                            TEST FUNCTIONS
 *============================================================================*/

/**
 * @brief Test memory initialization
 * @return Test result
 */
dsrtos_test_result_t test_memory_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_memory_config_t config;
    
    /* Configure memory */
    config.memory_start = (uintptr_t)g_test_memory;
    config.memory_size = sizeof(g_test_memory);
    config.alignment = 4U;
    config.heap_enabled = true;
    config.stack_enabled = true;
    
    /* Test memory initialization */
    result = dsrtos_memory_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test memory state */
    dsrtos_memory_state_t state = dsrtos_memory_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_MEMORY_STATE_READY);
    
    /* Test memory statistics */
    dsrtos_memory_stats_t stats;
    result = dsrtos_memory_get_stats(&stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(stats.total_size, sizeof(g_test_memory));
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test memory allocation
 * @return Test result
 */
dsrtos_test_result_t test_memory_allocation(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 256U;
    
    /* Test memory allocation */
    result = dsrtos_memory_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    /* Test allocated memory is writable */
    uint32_t *test_ptr = (uint32_t *)ptr;
    test_ptr[0] = PHASE2_MEMORY_PATTERN_1;
    DSRTOS_TEST_ASSERT_EQ(test_ptr[0], PHASE2_MEMORY_PATTERN_1);
    
    /* Test memory alignment */
    uintptr_t addr = (uintptr_t)ptr;
    DSRTOS_TEST_ASSERT_EQ(addr % 4U, 0U);
    
    /* Test memory deallocation */
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test memory deallocation
 * @return Test result
 */
dsrtos_test_result_t test_memory_deallocation(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 128U;
    
    /* Allocate memory */
    result = dsrtos_memory_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    /* Test deallocation */
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test double deallocation */
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    /* Test deallocation of NULL pointer */
    result = dsrtos_memory_deallocate(NULL);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test memory alignment
 * @return Test result
 */
dsrtos_test_result_t test_memory_alignment(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 64U;
    uintptr_t addr;
    
    /* Test 4-byte alignment */
    result = dsrtos_memory_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    addr = (uintptr_t)ptr;
    DSRTOS_TEST_ASSERT_EQ(addr % 4U, 0U);
    
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test 8-byte alignment */
    result = dsrtos_memory_allocate_aligned(size, 8U, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    addr = (uintptr_t)ptr;
    DSRTOS_TEST_ASSERT_EQ(addr % 8U, 0U);
    
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test memory integrity
 * @return Test result
 */
dsrtos_test_result_t test_memory_integrity(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 512U;
    uint32_t i;
    
    /* Allocate memory */
    result = dsrtos_memory_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    /* Fill memory with pattern */
    dsrtos_test_fill_pattern(ptr, size, PHASE2_MEMORY_PATTERN_1);
    
    /* Verify pattern */
    bool pattern_ok = verify_memory_pattern(ptr, size, PHASE2_MEMORY_PATTERN_1);
    DSRTOS_TEST_ASSERT_EQ(pattern_ok, true);
    
    /* Test memory integrity check */
    result = dsrtos_memory_check_integrity();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test memory corruption detection */
    uint32_t *test_ptr = (uint32_t *)ptr;
    test_ptr[0] = PHASE2_MEMORY_PATTERN_2; /* Corrupt first word */
    
    result = dsrtos_memory_check_integrity();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_CORRUPTION);
    
    /* Restore pattern */
    test_ptr[0] = PHASE2_MEMORY_PATTERN_1;
    
    result = dsrtos_memory_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test heap initialization
 * @return Test result
 */
dsrtos_test_result_t test_heap_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_heap_config_t config;
    
    /* Configure heap */
    config.heap_start = (uintptr_t)g_test_heap;
    config.heap_size = sizeof(g_test_heap);
    config.block_size = 32U;
    config.alignment = 4U;
    config.auto_defrag = true;
    
    /* Test heap initialization */
    result = dsrtos_heap_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test heap state */
    dsrtos_heap_state_t state = dsrtos_heap_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_HEAP_STATE_READY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test heap allocation
 * @return Test result
 */
dsrtos_test_result_t test_heap_allocation(void)
{
    dsrtos_error_t result;
    void *ptr;
    size_t size = 128U;
    
    /* Test heap allocation */
    result = dsrtos_heap_allocate(size, &ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NOT_NULL(ptr);
    
    /* Test allocated memory is writable */
    uint32_t *test_ptr = (uint32_t *)ptr;
    test_ptr[0] = PHASE2_MEMORY_PATTERN_3;
    DSRTOS_TEST_ASSERT_EQ(test_ptr[0], PHASE2_MEMORY_PATTERN_3);
    
    /* Test heap statistics */
    dsrtos_heap_stats_t stats;
    result = dsrtos_heap_get_stats(&stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_NE(stats.allocated_blocks, 0U);
    
    /* Test heap deallocation */
    result = dsrtos_heap_deallocate(ptr);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test heap fragmentation
 * @return Test result
 */
dsrtos_test_result_t test_heap_fragmentation(void)
{
    dsrtos_error_t result;
    void *ptr1, *ptr2, *ptr3;
    size_t size = 64U;
    
    /* Allocate multiple blocks */
    result = dsrtos_heap_allocate(size, &ptr1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_heap_allocate(size, &ptr2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_heap_allocate(size, &ptr3);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Free middle block to create fragmentation */
    result = dsrtos_heap_deallocate(ptr2);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test defragmentation */
    result = dsrtos_heap_defragment();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test heap statistics after defragmentation */
    dsrtos_heap_stats_t stats;
    result = dsrtos_heap_get_stats(&stats);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Clean up remaining allocations */
    result = dsrtos_heap_deallocate(ptr1);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_heap_deallocate(ptr3);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test stack initialization
 * @return Test result
 */
dsrtos_test_result_t test_stack_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_stack_config_t config;
    
    /* Configure stack */
    config.stack_start = (uintptr_t)g_test_stack;
    config.stack_size = sizeof(g_test_stack);
    config.element_size = sizeof(uint32_t);
    config.auto_grow = false;
    config.overflow_protection = true;
    
    /* Test stack initialization */
    result = dsrtos_stack_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test stack state */
    dsrtos_stack_state_t state = dsrtos_stack_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_STACK_STATE_READY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test stack operations
 * @return Test result
 */
dsrtos_test_result_t test_stack_operations(void)
{
    dsrtos_error_t result;
    uint32_t value;
    uint32_t test_value = PHASE2_MEMORY_PATTERN_4;
    
    /* Test stack push */
    result = dsrtos_stack_push(&test_value);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test stack pop */
    result = dsrtos_stack_pop(&value);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(value, test_value);
    
    /* Test stack empty */
    bool empty = dsrtos_stack_is_empty();
    DSRTOS_TEST_ASSERT_EQ(empty, true);
    
    /* Test stack full */
    bool full = dsrtos_stack_is_full();
    DSRTOS_TEST_ASSERT_EQ(full, false);
    
    /* Test stack size */
    uint32_t size = dsrtos_stack_get_size();
    DSRTOS_TEST_ASSERT_EQ(size, 0U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test critical section enter
 * @return Test result
 */
dsrtos_test_result_t test_critical_section_enter(void)
{
    dsrtos_error_t result;
    uint32_t i;
    
    /* Reset counter */
    g_critical_enter_count = 0U;
    
    /* Test critical section enter */
    for (i = 0U; i < PHASE2_CRITICAL_TEST_ITER; i++) {
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        g_critical_enter_count++;
        
        /* Test nested critical sections */
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        g_critical_enter_count++;
        
        /* Exit nested critical section */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Exit outer critical section */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    }
    
    /* Verify counter */
    DSRTOS_TEST_ASSERT_EQ(g_critical_enter_count, PHASE2_CRITICAL_TEST_ITER * 2U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test critical section exit
 * @return Test result
 */
dsrtos_test_result_t test_critical_section_exit(void)
{
    dsrtos_error_t result;
    uint32_t i;
    
    /* Reset counter */
    g_critical_exit_count = 0U;
    
    /* Test critical section exit */
    for (i = 0U; i < PHASE2_CRITICAL_TEST_ITER; i++) {
        /* Enter critical section */
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Exit critical section */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        g_critical_exit_count++;
    }
    
    /* Verify counter */
    DSRTOS_TEST_ASSERT_EQ(g_critical_exit_count, PHASE2_CRITICAL_TEST_ITER);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test nested critical sections
 * @return Test result
 */
dsrtos_test_result_t test_critical_section_nested(void)
{
    dsrtos_error_t result;
    uint32_t i;
    
    /* Test nested critical sections */
    for (i = 0U; i < PHASE2_CRITICAL_TEST_ITER; i++) {
        /* Enter first level */
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Enter second level */
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Enter third level */
        result = dsrtos_critical_enter();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Exit third level */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Exit second level */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
        
        /* Exit first level */
        result = dsrtos_critical_exit();
        DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    }
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test assertion condition
 * @return Test result
 */
dsrtos_test_result_t test_assert_condition(void)
{
    dsrtos_error_t result;
    
    /* Reset counter */
    g_assert_fail_count = 0U;
    
    /* Test valid assertion */
    result = dsrtos_assert_condition(true, "Valid condition");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 0U);
    
    /* Test invalid assertion */
    result = dsrtos_assert_condition(false, "Invalid condition");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_ASSERTION_FAILED);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 1U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test assertion range
 * @return Test result
 */
dsrtos_test_result_t test_assert_range(void)
{
    dsrtos_error_t result;
    int32_t value = 5;
    int32_t min_val = 0;
    int32_t max_val = 10;
    
    /* Reset counter */
    g_assert_fail_count = 0U;
    
    /* Test valid range */
    result = dsrtos_assert_range(value, min_val, max_val, "Valid range");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 0U);
    
    /* Test invalid range - too low */
    value = -1;
    result = dsrtos_assert_range(value, min_val, max_val, "Invalid range - too low");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_ASSERTION_FAILED);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 1U);
    
    /* Test invalid range - too high */
    value = 11;
    result = dsrtos_assert_range(value, min_val, max_val, "Invalid range - too high");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_ASSERTION_FAILED);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 2U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test assertion pointer
 * @return Test result
 */
dsrtos_test_result_t test_assert_pointer(void)
{
    dsrtos_error_t result;
    void *valid_ptr = g_test_memory;
    void *null_ptr = NULL;
    
    /* Reset counter */
    g_assert_fail_count = 0U;
    
    /* Test valid pointer */
    result = dsrtos_assert_pointer(valid_ptr, "Valid pointer");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 0U);
    
    /* Test null pointer */
    result = dsrtos_assert_pointer(null_ptr, "Null pointer");
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_ASSERTION_FAILED);
    DSRTOS_TEST_ASSERT_EQ(g_assert_fail_count, 1U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize Phase 2 test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_phase2_init(void)
{
    /* Initialize test suite */
    g_phase2_test_suite.name = "Phase 2 - Memory Management, Critical Sections, Assertions";
    g_phase2_test_suite.test_cases = g_phase2_test_cases;
    g_phase2_test_suite.test_count = sizeof(g_phase2_test_cases) / sizeof(g_phase2_test_cases[0]);
    g_phase2_test_suite.passed_count = 0U;
    g_phase2_test_suite.failed_count = 0U;
    g_phase2_test_suite.skipped_count = 0U;
    g_phase2_test_suite.error_count = 0U;
    g_phase2_test_suite.total_time_us = 0U;
    g_phase2_test_suite.enabled = true;
    
    /* Register test suite */
    return dsrtos_test_register_suite(&g_phase2_test_suite);
}
