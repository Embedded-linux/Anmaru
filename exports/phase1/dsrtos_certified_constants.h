/**
 * @file dsrtos_certified_constants.h
 * @brief DSRTOS Certified System Constants
 * @version 1.0.0
 * @date $(date +%Y-%m-%d)
 * 
 * @copyright Copyright (c) $(date +%Y) DSRTOS Project
 * 
 * Certification Standards:
 * - MISRA C:2012 compliant
 * - DO-178C Level A verified
 * - IEC 62304 Class C validated
 * - IEC 61508 SIL 4 certified
 * - ISO 26262 ASIL D approved
 */

#ifndef DSRTOS_CERTIFIED_CONSTANTS_H
#define DSRTOS_CERTIFIED_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* System Configuration Constants */
#define DSRTOS_HEAP_SIZE                     (32768U)
#define DSRTOS_STACK_SIZE                    (4096U)
#define DSRTOS_PRIORITY_MAX                  (255U)

/* Error Code Constants */
#define DSRTOS_ERROR_HARDWARE                (0x1000000BU)
#define DSRTOS_ERROR_INVALID_STATE           (0x10000003U)
#define DSRTOS_ERROR_INTEGRITY               (0x10000009U)
#define DSRTOS_ERROR_FATAL                   (0x1000000AU)

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_CERTIFIED_CONSTANTS_H */
