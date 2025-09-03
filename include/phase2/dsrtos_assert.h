/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Assertion Framework Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_ASSERT_H
#define DSRTOS_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "dsrtos_types.h"
#include "dsrtos_config.h"
#include "dsrtos_error.h"

/*=============================================================================
 * ASSERTION TYPES
 *============================================================================*/
typedef enum {
    DSRTOS_ASSERT_TYPE_PRECONDITION  = 0x00U,
    DSRTOS_ASSERT_TYPE_POSTCONDITION = 0x01U,
    DSRTOS_ASSERT_TYPE_INVARIANT     = 0x02U,
    DSRTOS_ASSERT_TYPE_STATE         = 0x03U,
    DSRTOS_ASSERT_TYPE_PARAMETER     = 0x04U,
    DSRTOS_ASSERT_TYPE_BOUNDS        = 0x05U,
    DSRTOS_ASSERT_TYPE_NULL_CHECK    = 0x06U,
    DSRTOS_ASSERT_TYPE_ALIGNMENT     = 0x07U,
    DSRTOS_ASSERT_TYPE_MAGIC         = 0x08U,
    DSRTOS_ASSERT_TYPE_CUSTOM        = 0x09U
} dsrtos_assert_type_t;

#define DSRTOS_NORETURN __attribute__((noreturn))

/*=============================================================================
 * ASSERTION HANDLER
 *============================================================================*/
typedef void (*dsrtos_assert_handler_t)(
    const char* expr,
    const char* file,
    uint32_t line,
    const char* func,
    dsrtos_assert_type_t type
);

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize assertion framework
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 */
dsrtos_error_t dsrtos_assert_init(void);

/**
 * @brief Set custom assertion handler
 * 
 * @param[in] handler Custom handler function
 * @return Previous handler
 */
dsrtos_assert_handler_t dsrtos_assert_set_handler(dsrtos_assert_handler_t handler);

/**
 * @brief Assert failure handler
 * 
 * @param[in] expr Expression that failed
 * @param[in] file Source file
 * @param[in] line Line number
 * @param[in] func Function name
 * @param[in] type Assertion type
 */

#define DSRTOS_NORETURN __attribute__((noreturn))

#if 0
void dsrtos_assert_failed(
    const char* expr,
    const char* file,
    uint32_t line,
    const char* func,
    dsrtos_assert_type_t type
) DSRTOS_NORETURN;

void dsrtos_assert_failed(
    const char* file,
    int line, 
    const char* func,
    const char* expr
);  /* Make sure this semicolon is present */

#endif

void dsrtos_assert_failed(const char* expr, const char* file, int line, const char* func);

/**
 * @brief Get assertion statistics
 * 
 * @param[out] count Total assertion failures
 * @return DSRTOS_SUCCESS on success
 */
dsrtos_error_t dsrtos_assert_get_stats(uint32_t* count);

/*=============================================================================
 * ASSERTION MACROS
 *============================================================================*/

#if DSRTOS_ASSERT_ENABLED

/**
 * @brief Basic assertion
 */
#if 0
#define DSRTOS_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            dsrtos_assert_failed(#expr, __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_INVARIANT); \
        } \
    } while (0)

#endif

#define DSRTOS_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            dsrtos_assert_failed(#expr, __FILE__, __LINE__, __func__); \
        } \
    } while (0)

/**
 * @brief Parameter assertion
 */
#define DSRTOS_ASSERT_PARAM(expr) \
    do { \
        if (!(expr)) { \
            dsrtos_assert_failed(#expr, __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_PARAMETER); \
        } \
    } while (0)

/**
 * @brief Null pointer assertion
 */
#define DSRTOS_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            dsrtos_assert_failed(#ptr " != NULL", __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_NULL_CHECK); \
        } \
    } while (0)

/**
 * @brief Bounds check assertion
 */
#define DSRTOS_ASSERT_BOUNDS(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            dsrtos_assert_failed(#val " in bounds", __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_BOUNDS); \
        } \
    } while (0)

/**
 * @brief Alignment assertion
 */
#define DSRTOS_ASSERT_ALIGNED(ptr, align) \
    do { \
        if (!DSRTOS_IS_ALIGNED((uintptr_t)(ptr), (align))) { \
            dsrtos_assert_failed(#ptr " aligned", __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_ALIGNMENT); \
        } \
    } while (0)

/**
 * @brief Magic number assertion
 */
#define DSRTOS_ASSERT_MAGIC(val, expected) \
    do { \
        if ((val) != (expected)) { \
            dsrtos_assert_failed("Magic number", __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_MAGIC); \
        } \
    } while (0)

/**
 * @brief State assertion
 */
#if 0
#define DSRTOS_ASSERT_STATE(state, expected) \
    do { \
        if ((state) != (expected)) { \
            dsrtos_assert_failed("State check", __FILE__, __LINE__, __func__, \
                                DSRTOS_ASSERT_TYPE_STATE); \
        } \
    } while (0)
#endif
#else /* DSRTOS_ASSERT_ENABLED == 0 */

/* Assertions compiled out in release builds */
#define DSRTOS_ASSERT(expr)                    ((void)0)
#define DSRTOS_ASSERT_PARAM(expr)              ((void)0)
#define DSRTOS_ASSERT_NOT_NULL(ptr)            ((void)0)
#define DSRTOS_ASSERT_BOUNDS(val, min, max)    ((void)0)
#define DSRTOS_ASSERT_ALIGNED(ptr, align)      ((void)0)
#define DSRTOS_ASSERT_MAGIC(val, expected)     ((void)0)
#define DSRTOS_ASSERT_STATE(state, expected)   ((void)0)

#endif /* DSRTOS_ASSERT_ENABLED */

/**
 * @brief Static assertion (compile-time)
 */

#if 0
#define DSRTOS_STATIC_ASSERT(expr) \
    _Static_assert(expr, "Static assertion failed: " #expr)

#endif

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_ASSERT_H */
