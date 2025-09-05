/**
 * @file dsrtos_test_minimal.h
 * @brief DSRTOS Minimal Test Suite Header
 * @author DSRTOS Team
 * @date 2024
 * @version 1.0
 */

#ifndef DSRTOS_TEST_MINIMAL_H
#define DSRTOS_TEST_MINIMAL_H

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_test_framework.h"

/*==============================================================================
 *                            FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize minimal test suite
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_minimal_init(void);

/**
 * @brief Minimal test main entry point
 * @return DSRTOS_OK on success, error code on failure
 */
dsrtos_error_t dsrtos_test_minimal_main(void);

#endif /* DSRTOS_TEST_MINIMAL_H */
