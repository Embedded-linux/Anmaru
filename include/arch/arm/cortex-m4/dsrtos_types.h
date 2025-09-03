/*
 * File: dsrtos_types.h
 * Common type definitions for DSRTOS
 */

#ifndef DSRTOS_TYPES_H
#define DSRTOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* IO definitions */
#ifdef __cplusplus
  #define   __I     volatile
#else
  #define   __I     volatile const
#endif
#define     __O     volatile
#define     __IO    volatile

/* Common macros */
#define UNUSED(x) ((void)(x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Bit manipulation macros */
#define BIT(n) (1UL << (n))
#define SET_BIT(REG, BIT) ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT) ((REG) & (BIT))

#endif /* DSRTOS_TYPES_H */
