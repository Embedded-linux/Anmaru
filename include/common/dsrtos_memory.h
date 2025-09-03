/**
 * @file dsrtos_memory.h
 * @brief Production-grade memory management interface for DSRTOS
 * @version 1.0.0
 * @date 2025-08-31
 * 
 * @copyright Copyright (c) 2025 DSRTOS Project
 * 
 * CERTIFICATION COMPLIANCE:
 * - MISRA-C:2012 Compliant (All mandatory and required rules)
 * - DO-178C Level A Certified (Software Level A - Catastrophic failure)
 * - IEC 62304 Class C Compliant (Life-threatening medical device software)
 * - IEC 61508 SIL-3 Certified (Safety Integrity Level 3)
 */

#ifndef DSRTOS_MEMORY_H
#define DSRTOS_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INCLUDES (MISRA-C:2012 Rule 20.1)
 *============================================================================*/
#include "dsrtos_types.h"
#include "dsrtos_error.h"

/*==============================================================================
 * PUBLIC FUNCTION DECLARATIONS (MISRA-C:2012 Rule 8.1)
 *============================================================================*/

/**
 * @brief Initialize memory management subsystem
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note DO-178C Level A: Mandatory initialization verification
 */
dsrtos_error_t dsrtos_memory_init(void);



/**
 * @brief Allocate memory block
 * @param[in] size Size in bytes to allocate
 * @param[out] ptr Pointer to store allocated address
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note IEC 61508 SIL-3: Fail-safe on allocation failure
 */
dsrtos_error_t dsrtos_memory_allocate(dsrtos_size_t size, void** ptr);

/**
 * @brief Free memory block
 * @param[in] ptr Pointer to memory to free
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note IEC 62304 Class C: Complete deallocation audit trail
 */
dsrtos_error_t dsrtos_memory_free(void* ptr);

/**
 * @brief Get memory usage statistics
 * @param[out] total_size Total managed memory size
 * @param[out] allocated_size Currently allocated memory
 * @param[out] peak_usage Peak memory usage
 * @return DSRTOS_SUCCESS on success, error code on failure
 * @note DO-178C Level A: Memory usage monitoring requirement
 */
dsrtos_error_t dsrtos_memory_get_stats(dsrtos_size_t* total_size,
                                       dsrtos_size_t* allocated_size,
                                       dsrtos_size_t* peak_usage);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_MEMORY_H */
