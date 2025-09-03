/**
 * @file dsrtos_stubs.c  
 * @brief DSRTOS Stub Implementations - MISRA Compliant
 */

#include "dsrtos_auto_constants.h"
#include "../include/common/dsrtos_error.h"
#include "../include/common/dsrtos_types.h"
#include <stdint.h>
#include <stdbool.h>

/* MISRA compliant stub implementations */
/* These functions are now implemented in dsrtos_error.c */

/* These functions are now implemented in dsrtos_memory_stub.c */

/* Stack protection function - called by compiler when stack corruption detected */
void __stack_chk_fail(void) __attribute__((noreturn));
void __stack_chk_fail(void)
{
    while(1) { /* Infinite loop on stack corruption */ }
}

uint32_t __stack_chk_guard = 0xDEADBEEFU;
