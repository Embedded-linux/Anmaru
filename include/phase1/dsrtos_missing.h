/**
 * @file dsrtos_missing.h
 * @brief DSRTOS Missing Function Declarations
 * @version 1.0.0
 * @date 2025-09-01
 * 
 * Compliance: MISRA C:2012, DO-178C, IEC 62304, IEC 61508
 */

#ifndef DSRTOS_MISSING_H
#define DSRTOS_MISSING_H

#include "dsrtos_types.h"
#include "dsrtos_error.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Determines if error code represents critical system error
 * @param[in] error_code Error code to evaluate
 * @return true if critical, false otherwise
 */
bool dsrtos_error_is_critical(dsrtos_error_t error_code);

/**
 * @brief Determines if error requires immediate system shutdown
 * @param[in] error_code Error code to evaluate  
 * @return true if shutdown required, false otherwise
 */
bool dsrtos_error_requires_shutdown(dsrtos_error_t error_code);

/**
 * @brief Initialize memory management subsystem
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_memory_init(void);

/**
 * @brief Retrieve memory subsystem statistics
 * @param[out] stats_ptr Pointer to statistics structure
 */
void dsrtos_memory_get_stats(void* const stats_ptr);

/**
 * @brief Stack corruption failure handler (GCC runtime)
 * @note Function does not return
 */
void __stack_chk_fail(void);

/**
 * @brief Stack protection guard value (GCC runtime)
 */
extern uint32_t __stack_chk_guard;

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_MISSING_H */
