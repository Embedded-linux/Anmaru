/*
 * @file dsrtos_critical.h
 * @brief DSRTOS Critical Section Interface
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_CRITICAL_H
#define DSRTOS_CRITICAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * CRITICAL SECTION INTERFACE
 *============================================================================*/

/* Critical section management */
void dsrtos_critical_enter(void);
void dsrtos_critical_exit(void);
uint32_t dsrtos_critical_enter_from_isr(void);
void dsrtos_critical_exit_from_isr(uint32_t state);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CRITICAL_H */
