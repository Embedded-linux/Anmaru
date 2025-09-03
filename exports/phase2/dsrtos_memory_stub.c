#include "dsrtos_error.h"
#include "dsrtos_types.h"

/* Stub implementations for memory functions */
/* CERTIFIED_DUPLICATE_REMOVED: dsrtos_error_t dsrtos_memory_init(void)
{
    return DSRTOS_OK;
}

dsrtos_error_t dsrtos_memory_allocate(size_t size, void** ptr)
{
    (void)size;
    (void)ptr;
    return DSRTOS_ERR_NOT_IMPLEMENTED;
}

dsrtos_error_t dsrtos_memory_free(void* ptr)
{
    (void)ptr;
    return DSRTOS_ERR_NOT_IMPLEMENTED;
}
