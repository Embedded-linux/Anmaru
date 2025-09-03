/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Project
 *
 * File: board.h
 * Description: Board-specific interface definitions
 * Phase: 1 - Boot & Board Bring-up
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#ifndef DSRTOS_BOARD_H
#define DSRTOS_BOARD_H

#include <stdint.h>

/* Board identification */
#define BOARD_NAME              "STM32F4-Discovery"
#define BOARD_REVISION          "1.0"
#define BOARD_MCU               "STM32F407VGT6"

/* Board LED definitions */
#define BOARD_LED_GREEN         0U
#define BOARD_LED_ORANGE        1U
#define BOARD_LED_RED           2U
#define BOARD_LED_BLUE          3U
#define BOARD_LED_COUNT         4U

/* Board button definitions */
#define BOARD_BUTTON_USER       0U
#define BOARD_BUTTON_COUNT      1U

/* Board features */
#define BOARD_HAS_LEDS          1
#define BOARD_HAS_BUTTONS       1
#define BOARD_HAS_UART          1
#define BOARD_HAS_USB           1
#define BOARD_HAS_ETHERNET      0
#define BOARD_HAS_SDCARD        0

/* Function prototypes */
void dsrtos_board_init(void);
void dsrtos_board_peripheral_init(void);
void dsrtos_board_led_on(uint32_t led);
void dsrtos_board_led_off(uint32_t led);
void dsrtos_board_led_toggle(uint32_t led);
uint32_t dsrtos_board_button_read(void);
void dsrtos_board_delay_ms(uint32_t ms);

#endif /* DSRTOS_BOARD_H */
