/**
 * @file dsrtos_test_phase1.c
 * @brief DSRTOS Phase 1 Test Cases - Boot, Clock, Interrupt, Timer, UART
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file contains comprehensive test cases for Phase 1 functionality
 * following MISRA-C:2012 standards and DO-178C DAL-B requirements.
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
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/

/* Test timeouts */
#define PHASE1_TEST_TIMEOUT_MS       1000U
#define PHASE1_CLOCK_TEST_CYCLES     100U
#define PHASE1_UART_TEST_BAUD        115200U
#define PHASE1_TIMER_TEST_PERIOD     1000U  /* 1ms */

/* Test data patterns */
#define PHASE1_UART_TEST_PATTERN     0x55U
#define PHASE1_UART_TEST_LENGTH      16U

/*==============================================================================
 *                            STATIC VARIABLES
 *============================================================================*/

/* Test suite instance */
static dsrtos_test_suite_t g_phase1_test_suite;

/* Test case array */
static dsrtos_test_case_t g_phase1_test_cases[] = {
    {"test_boot_initialization", test_boot_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_boot_sequence", test_boot_sequence, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_clock_initialization", test_clock_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_clock_frequency", test_clock_frequency, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_clock_source_switch", test_clock_source_switch, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_interrupt_initialization", test_interrupt_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_interrupt_enable_disable", test_interrupt_enable_disable, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_interrupt_priority", test_interrupt_priority, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_timer_initialization", test_timer_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_timer_start_stop", test_timer_start_stop, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_timer_period", test_timer_period, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_uart_initialization", test_uart_initialization, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_uart_baud_rate", test_uart_baud_rate, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_uart_transmit", test_uart_transmit, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_uart_receive", test_uart_receive, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL},
    {"test_uart_flow_control", test_uart_flow_control, DSRTOS_TEST_TIMEOUT_MS(PHASE1_TEST_TIMEOUT_MS), true, DSRTOS_TEST_RESULT_PASS, 0U, 0U, NULL}
};

/* Test counters */
static volatile uint32_t g_interrupt_count = 0U;
static volatile uint32_t g_timer_count = 0U;
static volatile bool g_uart_tx_complete = false;
static volatile bool g_uart_rx_complete = false;

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Test interrupt callback
 */
static void test_interrupt_callback(void)
{
    g_interrupt_count++;
}

/**
 * @brief Test timer callback
 */
static void test_timer_callback(void)
{
    g_timer_count++;
}

/**
 * @brief Test UART transmit callback
 */
static void test_uart_tx_callback(void)
{
    g_uart_tx_complete = true;
}

/**
 * @brief Test UART receive callback
 */
static void test_uart_rx_callback(void)
{
    g_uart_rx_complete = true;
}

/*==============================================================================
 *                            TEST FUNCTIONS
 *============================================================================*/

/**
 * @brief Test boot initialization
 * @return Test result
 */
dsrtos_test_result_t test_boot_initialization(void)
{
    dsrtos_error_t result;
    
    /* Test boot initialization */
    result = dsrtos_boot_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test boot state */
    dsrtos_boot_state_t state = dsrtos_boot_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_BOOT_STATE_INITIALIZED);
    
    /* Test boot version */
    uint32_t version = dsrtos_boot_get_version();
    DSRTOS_TEST_ASSERT_NE(version, 0U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test boot sequence
 * @return Test result
 */
dsrtos_test_result_t test_boot_sequence(void)
{
    dsrtos_error_t result;
    dsrtos_boot_state_t state;
    
    /* Test boot sequence start */
    result = dsrtos_boot_start_sequence();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test boot state progression */
    state = dsrtos_boot_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_BOOT_STATE_RUNNING);
    
    /* Test boot completion */
    result = dsrtos_boot_complete_sequence();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    state = dsrtos_boot_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_BOOT_STATE_COMPLETE);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test clock initialization
 * @return Test result
 */
dsrtos_test_result_t test_clock_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_clock_config_t config;
    
    /* Configure clock */
    config.source = DSRTOS_CLOCK_SOURCE_HSE;
    config.frequency = 8000000U; /* 8 MHz */
    config.pll_enabled = true;
    config.pll_multiplier = 25U;
    config.pll_divider = 2U;
    
    /* Test clock initialization */
    result = dsrtos_clock_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test clock state */
    dsrtos_clock_state_t state = dsrtos_clock_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_CLOCK_STATE_RUNNING);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test clock frequency
 * @return Test result
 */
dsrtos_test_result_t test_clock_frequency(void)
{
    uint32_t frequency;
    uint32_t expected_frequency;
    
    /* Test system clock frequency */
    frequency = dsrtos_clock_get_system_frequency();
    expected_frequency = 100000000U; /* 100 MHz */
    DSRTOS_TEST_ASSERT_EQ(frequency, expected_frequency);
    
    /* Test AHB clock frequency */
    frequency = dsrtos_clock_get_ahb_frequency();
    DSRTOS_TEST_ASSERT_EQ(frequency, expected_frequency);
    
    /* Test APB1 clock frequency */
    frequency = dsrtos_clock_get_apb1_frequency();
    expected_frequency = 50000000U; /* 50 MHz */
    DSRTOS_TEST_ASSERT_EQ(frequency, expected_frequency);
    
    /* Test APB2 clock frequency */
    frequency = dsrtos_clock_get_apb2_frequency();
    expected_frequency = 100000000U; /* 100 MHz */
    DSRTOS_TEST_ASSERT_EQ(frequency, expected_frequency);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test clock source switching
 * @return Test result
 */
dsrtos_test_result_t test_clock_source_switch(void)
{
    dsrtos_error_t result;
    dsrtos_clock_source_t source;
    
    /* Test switching to HSI */
    result = dsrtos_clock_set_source(DSRTOS_CLOCK_SOURCE_HSI);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    source = dsrtos_clock_get_source();
    DSRTOS_TEST_ASSERT_EQ(source, DSRTOS_CLOCK_SOURCE_HSI);
    
    /* Test switching to HSE */
    result = dsrtos_clock_set_source(DSRTOS_CLOCK_SOURCE_HSE);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    source = dsrtos_clock_get_source();
    DSRTOS_TEST_ASSERT_EQ(source, DSRTOS_CLOCK_SOURCE_HSE);
    
    /* Test switching to PLL */
    result = dsrtos_clock_set_source(DSRTOS_CLOCK_SOURCE_PLL);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    source = dsrtos_clock_get_source();
    DSRTOS_TEST_ASSERT_EQ(source, DSRTOS_CLOCK_SOURCE_PLL);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test interrupt initialization
 * @return Test result
 */
dsrtos_test_result_t test_interrupt_initialization(void)
{
    dsrtos_error_t result;
    
    /* Test interrupt system initialization */
    result = dsrtos_interrupt_init();
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test interrupt state */
    dsrtos_interrupt_state_t state = dsrtos_interrupt_get_state();
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_INTERRUPT_STATE_READY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test interrupt enable/disable
 * @return Test result
 */
dsrtos_test_result_t test_interrupt_enable_disable(void)
{
    dsrtos_error_t result;
    dsrtos_interrupt_id_t irq_id = DSRTOS_INTERRUPT_SYSTICK;
    
    /* Test interrupt enable */
    result = dsrtos_interrupt_enable(irq_id);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test interrupt is enabled */
    bool enabled = dsrtos_interrupt_is_enabled(irq_id);
    DSRTOS_TEST_ASSERT_EQ(enabled, true);
    
    /* Test interrupt disable */
    result = dsrtos_interrupt_disable(irq_id);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test interrupt is disabled */
    enabled = dsrtos_interrupt_is_enabled(irq_id);
    DSRTOS_TEST_ASSERT_EQ(enabled, false);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test interrupt priority
 * @return Test result
 */
dsrtos_test_result_t test_interrupt_priority(void)
{
    dsrtos_error_t result;
    dsrtos_interrupt_id_t irq_id = DSRTOS_INTERRUPT_SYSTICK;
    uint32_t priority = 5U;
    
    /* Test setting interrupt priority */
    result = dsrtos_interrupt_set_priority(irq_id, priority);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test getting interrupt priority */
    uint32_t actual_priority = dsrtos_interrupt_get_priority(irq_id);
    DSRTOS_TEST_ASSERT_EQ(actual_priority, priority);
    
    /* Test priority range */
    result = dsrtos_interrupt_set_priority(irq_id, 16U);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test timer initialization
 * @return Test result
 */
dsrtos_test_result_t test_timer_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_timer_config_t config;
    
    /* Configure timer */
    config.timer_id = DSRTOS_TIMER_SYSTICK;
    config.period_us = PHASE1_TIMER_TEST_PERIOD;
    config.callback = test_timer_callback;
    config.auto_reload = true;
    config.enabled = false;
    
    /* Test timer initialization */
    result = dsrtos_timer_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test timer state */
    dsrtos_timer_state_t state = dsrtos_timer_get_state(config.timer_id);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TIMER_STATE_READY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test timer start/stop
 * @return Test result
 */
dsrtos_test_result_t test_timer_start_stop(void)
{
    dsrtos_error_t result;
    dsrtos_timer_id_t timer_id = DSRTOS_TIMER_SYSTICK;
    
    /* Reset counter */
    g_timer_count = 0U;
    
    /* Test timer start */
    result = dsrtos_timer_start(timer_id);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test timer state */
    dsrtos_timer_state_t state = dsrtos_timer_get_state(timer_id);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TIMER_STATE_RUNNING);
    
    /* Wait for timer ticks */
    for (uint32_t i = 0U; i < PHASE1_CLOCK_TEST_CYCLES; i++) {
        /* Wait for timer interrupt */
        __asm volatile ("wfi");
    }
    
    /* Test timer stop */
    result = dsrtos_timer_stop(timer_id);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test timer state */
    state = dsrtos_timer_get_state(timer_id);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_TIMER_STATE_STOPPED);
    
    /* Verify timer callback was called */
    DSRTOS_TEST_ASSERT_NE(g_timer_count, 0U);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test timer period
 * @return Test result
 */
dsrtos_test_result_t test_timer_period(void)
{
    dsrtos_error_t result;
    dsrtos_timer_id_t timer_id = DSRTOS_TIMER_SYSTICK;
    uint32_t period_us = 1000U; /* 1ms */
    
    /* Test setting timer period */
    result = dsrtos_timer_set_period(timer_id, period_us);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test getting timer period */
    uint32_t actual_period = dsrtos_timer_get_period(timer_id);
    DSRTOS_TEST_ASSERT_EQ(actual_period, period_us);
    
    /* Test period range */
    result = dsrtos_timer_set_period(timer_id, 0U);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test UART initialization
 * @return Test result
 */
dsrtos_test_result_t test_uart_initialization(void)
{
    dsrtos_error_t result;
    dsrtos_uart_config_t config;
    
    /* Configure UART */
    config.uart_id = DSRTOS_UART_1;
    config.baud_rate = PHASE1_UART_TEST_BAUD;
    config.data_bits = DSRTOS_UART_DATA_BITS_8;
    config.stop_bits = DSRTOS_UART_STOP_BITS_1;
    config.parity = DSRTOS_UART_PARITY_NONE;
    config.flow_control = DSRTOS_UART_FLOW_CONTROL_NONE;
    config.tx_callback = test_uart_tx_callback;
    config.rx_callback = test_uart_rx_callback;
    
    /* Test UART initialization */
    result = dsrtos_uart_init(&config);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test UART state */
    dsrtos_uart_state_t state = dsrtos_uart_get_state(config.uart_id);
    DSRTOS_TEST_ASSERT_EQ(state, DSRTOS_UART_STATE_READY);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test UART baud rate
 * @return Test result
 */
dsrtos_test_result_t test_uart_baud_rate(void)
{
    dsrtos_error_t result;
    dsrtos_uart_id_t uart_id = DSRTOS_UART_1;
    uint32_t baud_rate = 9600U;
    
    /* Test setting baud rate */
    result = dsrtos_uart_set_baud_rate(uart_id, baud_rate);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test getting baud rate */
    uint32_t actual_baud = dsrtos_uart_get_baud_rate(uart_id);
    DSRTOS_TEST_ASSERT_EQ(actual_baud, baud_rate);
    
    /* Test baud rate range */
    result = dsrtos_uart_set_baud_rate(uart_id, 0U);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_ERROR_INVALID_PARAM);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test UART transmit
 * @return Test result
 */
dsrtos_test_result_t test_uart_transmit(void)
{
    dsrtos_error_t result;
    dsrtos_uart_id_t uart_id = DSRTOS_UART_1;
    uint8_t test_data[PHASE1_UART_TEST_LENGTH];
    uint32_t i;
    
    /* Prepare test data */
    for (i = 0U; i < PHASE1_UART_TEST_LENGTH; i++) {
        test_data[i] = PHASE1_UART_TEST_PATTERN;
    }
    
    /* Reset callback flag */
    g_uart_tx_complete = false;
    
    /* Test UART transmit */
    result = dsrtos_uart_transmit(uart_id, test_data, PHASE1_UART_TEST_LENGTH);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Wait for transmission complete */
    for (i = 0U; i < 1000U; i++) {
        if (g_uart_tx_complete) {
            break;
        }
        __asm volatile ("nop");
    }
    
    /* Verify transmission complete */
    DSRTOS_TEST_ASSERT_EQ(g_uart_tx_complete, true);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test UART receive
 * @return Test result
 */
dsrtos_test_result_t test_uart_receive(void)
{
    dsrtos_error_t result;
    dsrtos_uart_id_t uart_id = DSRTOS_UART_1;
    uint8_t test_data[PHASE1_UART_TEST_LENGTH];
    uint32_t received_length;
    uint32_t i;
    
    /* Reset callback flag */
    g_uart_rx_complete = false;
    
    /* Test UART receive */
    result = dsrtos_uart_receive(uart_id, test_data, PHASE1_UART_TEST_LENGTH, &received_length);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Wait for reception complete */
    for (i = 0U; i < 1000U; i++) {
        if (g_uart_rx_complete) {
            break;
        }
        __asm volatile ("nop");
    }
    
    /* Verify reception complete */
    DSRTOS_TEST_ASSERT_EQ(g_uart_rx_complete, true);
    DSRTOS_TEST_ASSERT_EQ(received_length, PHASE1_UART_TEST_LENGTH);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/**
 * @brief Test UART flow control
 * @return Test result
 */
dsrtos_test_result_t test_uart_flow_control(void)
{
    dsrtos_error_t result;
    dsrtos_uart_id_t uart_id = DSRTOS_UART_1;
    dsrtos_uart_flow_control_t flow_control;
    
    /* Test setting flow control */
    result = dsrtos_uart_set_flow_control(uart_id, DSRTOS_UART_FLOW_CONTROL_RTS_CTS);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    /* Test getting flow control */
    result = dsrtos_uart_get_flow_control(uart_id, &flow_control);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(flow_control, DSRTOS_UART_FLOW_CONTROL_RTS_CTS);
    
    /* Test disabling flow control */
    result = dsrtos_uart_set_flow_control(uart_id, DSRTOS_UART_FLOW_CONTROL_NONE);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    
    result = dsrtos_uart_get_flow_control(uart_id, &flow_control);
    DSRTOS_TEST_ASSERT_EQ(result, DSRTOS_OK);
    DSRTOS_TEST_ASSERT_EQ(flow_control, DSRTOS_UART_FLOW_CONTROL_NONE);
    
    return DSRTOS_TEST_RESULT_PASS;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize Phase 1 test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_phase1_init(void)
{
    /* Initialize test suite */
    g_phase1_test_suite.name = "Phase 1 - Boot, Clock, Interrupt, Timer, UART";
    g_phase1_test_suite.test_cases = g_phase1_test_cases;
    g_phase1_test_suite.test_count = sizeof(g_phase1_test_cases) / sizeof(g_phase1_test_cases[0]);
    g_phase1_test_suite.passed_count = 0U;
    g_phase1_test_suite.failed_count = 0U;
    g_phase1_test_suite.skipped_count = 0U;
    g_phase1_test_suite.error_count = 0U;
    g_phase1_test_suite.total_time_us = 0U;
    g_phase1_test_suite.enabled = true;
    
    /* Register test suite */
    return dsrtos_test_register_suite(&g_phase1_test_suite);
}
