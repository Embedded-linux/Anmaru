/**
 * @file main_minimal.c
 * @brief DSRTOS Minimal Test Main Entry Point
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 * 
 * This file provides the minimal main entry point for target board testing.
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_minimal.h"
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

/* System configuration */
#define SYSTEM_CLOCK_FREQ_HZ         168000000U  /* 168 MHz */
#define UART_BAUD_RATE               115200U
#define TIMER_PERIOD_US              1000U       /* 1ms */

/*==============================================================================
 *                            STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief System initialization
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_error_t system_init(void)
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
        .period_us = TIMER_PERIOD_US,
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
        .baud_rate = UART_BAUD_RATE,
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
        .memory_size = 64U * 1024U,  /* 64 KB */
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
    
    return DSRTOS_OK;
}

/**
 * @brief System deinitialization
 * @return DSRTOS_OK on success, error code on failure
 */
static dsrtos_error_t system_deinit(void)
{
    dsrtos_error_t result;
    
    /* Deinitialize all systems in reverse order */
    result = dsrtos_assert_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_critical_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_memory_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_uart_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_timer_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_interrupt_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_clock_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    result = dsrtos_boot_deinit();
    if (result != DSRTOS_OK) {
        return result;
    }
    
    return DSRTOS_OK;
}

/*==============================================================================
 *                            PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Main entry point for minimal testing
 * @return DSRTOS_OK on success, error code on failure
 */
int main(void)
{
    dsrtos_error_t result;
    
    /* Initialize system */
    result = system_init();
    if (result != DSRTOS_OK) {
        /* System initialization failed - enter infinite loop */
        while (1) {
            __asm volatile ("wfi");
        }
    }
    
    /* Run minimal test suite */
    result = dsrtos_test_minimal_main();
    
    /* Deinitialize system */
    (void)system_deinit();
    
    /* Test completed - enter infinite loop */
    while (1) {
        __asm volatile ("wfi");
    }
    
    /* Never reached */
    return (int)result;
}
