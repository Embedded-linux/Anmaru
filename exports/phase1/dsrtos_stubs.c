/**
 * @file dsrtos_stubs.c  
 * @brief DSRTOS Stub Implementations - MISRA Compliant
 */

#include "dsrtos_auto_constants.h"
#include <stdint.h>
#include <stdbool.h>

/* MISRA compliant stub implementations */
bool dsrtos_error_is_critical(uint32_t error_code)
{
    return (error_code == DSRTOS_ERROR_FATAL);
}

bool dsrtos_error_requires_shutdown(uint32_t error_code)
{
    return (error_code == DSRTOS_ERROR_FATAL);
}

uint32_t dsrtos_memory_init(void)
{
    return 0U; /* Success */
}

void dsrtos_memory_get_stats(uint32_t* total, uint32_t* allocated, uint32_t* peak)
{
    if (total != ((void*)0)) { *total = DSRTOS_HEAP_SIZE; }
    if (allocated != ((void*)0)) { *allocated = 0U; }
    if (peak != ((void*)0)) { *peak = 0U; }
}

void __stack_chk_fail(void)
{
    while(1) { /* Infinite loop on stack corruption */ }
}

uint32_t __stack_chk_guard = 0xDEADBEEFU;
