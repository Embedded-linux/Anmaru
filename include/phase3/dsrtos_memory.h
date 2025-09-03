/*
 * @file dsrtos_memory.h
 * @brief DSRTOS Memory Management Interface Stub for Phase 3
 * @date 2024-12-30
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 */

#ifndef DSRTOS_MEMORY_H
#define DSRTOS_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*==============================================================================
 * MEMORY INTERFACE (Stub for Phase 3)
 *============================================================================*/

/* Memory allocation */
void* dsrtos_malloc(size_t size);
void dsrtos_free(void *ptr);

/* Memory statistics */
size_t dsrtos_get_free_heap(void);
size_t dsrtos_get_total_heap(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_MEMORY_H */
