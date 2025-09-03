/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: System Call Interface Header
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

#ifndef DSRTOS_SYSCALL_H
#define DSRTOS_SYSCALL_H

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
 * SYSTEM CALL NUMBERS
 *============================================================================*/
typedef enum {
    /* Task Management */
    DSRTOS_SYSCALL_TASK_CREATE      = 0x0001U,
    DSRTOS_SYSCALL_TASK_DELETE      = 0x0002U,
    DSRTOS_SYSCALL_TASK_SUSPEND     = 0x0003U,
    DSRTOS_SYSCALL_TASK_RESUME      = 0x0004U,
    DSRTOS_SYSCALL_TASK_YIELD       = 0x0005U,
    DSRTOS_SYSCALL_TASK_DELAY       = 0x0006U,
    DSRTOS_SYSCALL_TASK_GET_INFO    = 0x0007U,
    
    /* Memory Management */
    DSRTOS_SYSCALL_MEM_ALLOC        = 0x0010U,
    DSRTOS_SYSCALL_MEM_FREE         = 0x0011U,
    DSRTOS_SYSCALL_MEM_REALLOC      = 0x0012U,
    DSRTOS_SYSCALL_MEM_GET_INFO     = 0x0013U,
    
    /* Synchronization */
    DSRTOS_SYSCALL_MUTEX_CREATE     = 0x0020U,
    DSRTOS_SYSCALL_MUTEX_DELETE     = 0x0021U,
    DSRTOS_SYSCALL_MUTEX_LOCK       = 0x0022U,
    DSRTOS_SYSCALL_MUTEX_UNLOCK     = 0x0023U,
    DSRTOS_SYSCALL_SEM_CREATE       = 0x0024U,
    DSRTOS_SYSCALL_SEM_DELETE       = 0x0025U,
    DSRTOS_SYSCALL_SEM_WAIT         = 0x0026U,
    DSRTOS_SYSCALL_SEM_POST         = 0x0027U,
    
    /* Message Passing */
    DSRTOS_SYSCALL_QUEUE_CREATE     = 0x0030U,
    DSRTOS_SYSCALL_QUEUE_DELETE     = 0x0031U,
    DSRTOS_SYSCALL_QUEUE_SEND       = 0x0032U,
    DSRTOS_SYSCALL_QUEUE_RECEIVE    = 0x0033U,
    
    /* Time Management */
    DSRTOS_SYSCALL_TIME_GET         = 0x0040U,
    DSRTOS_SYSCALL_TIME_SET         = 0x0041U,
    DSRTOS_SYSCALL_TIMER_CREATE     = 0x0042U,
    DSRTOS_SYSCALL_TIMER_DELETE     = 0x0043U,
    DSRTOS_SYSCALL_TIMER_START      = 0x0044U,
    DSRTOS_SYSCALL_TIMER_STOP       = 0x0045U,
    
    /* System Information */
    DSRTOS_SYSCALL_SYS_GET_INFO     = 0x0050U,
    DSRTOS_SYSCALL_SYS_GET_STATS    = 0x0051U,
    DSRTOS_SYSCALL_SYS_RESET        = 0x0052U,
    
    /* Device I/O */
    DSRTOS_SYSCALL_IO_OPEN          = 0x0060U,
    DSRTOS_SYSCALL_IO_CLOSE         = 0x0061U,
    DSRTOS_SYSCALL_IO_READ          = 0x0062U,
    DSRTOS_SYSCALL_IO_WRITE         = 0x0063U,
    DSRTOS_SYSCALL_IO_IOCTL         = 0x0064U,
    
    /* Custom/Extended */
    DSRTOS_SYSCALL_CUSTOM_BASE      = 0x1000U,
    
    DSRTOS_SYSCALL_MAX              = 0xFFFFU
} dsrtos_syscall_num_t;

/*=============================================================================
 * SYSTEM CALL PARAMETER STRUCTURE
 *============================================================================*/
typedef struct {
    uint32_t param0;                    /* First parameter */
    uint32_t param1;                    /* Second parameter */
    uint32_t param2;                    /* Third parameter */
    uint32_t param3;                    /* Fourth parameter */
    void* data_ptr;                     /* Data pointer */
    size_t data_size;                   /* Data size */
} dsrtos_syscall_params_t;

/*=============================================================================
 * SYSTEM CALL RESULT STRUCTURE
 *============================================================================*/
typedef struct {
    dsrtos_error_t error;               /* Error code */
    uint32_t return_value;              /* Return value */
    void* return_ptr;                   /* Return pointer */
} dsrtos_syscall_result_t;

/*=============================================================================
 * SYSTEM CALL CONTEXT
 *============================================================================*/
typedef struct {
    dsrtos_syscall_num_t syscall_num;  /* System call number */
    dsrtos_syscall_params_t params;    /* Parameters */
    dsrtos_syscall_result_t result;    /* Result */
    void* caller;                       /* Calling task/context */
    uint32_t timestamp;                 /* Call timestamp */
    uint32_t flags;                     /* Call flags */
} dsrtos_syscall_context_t;

/*=============================================================================
 * SYSTEM CALL HANDLER TYPE
 *============================================================================*/
typedef dsrtos_error_t (*dsrtos_syscall_handler_t)(
    dsrtos_syscall_context_t* context
);

/*=============================================================================
 * SYSTEM CALL TABLE ENTRY
 *============================================================================*/
typedef struct {
    dsrtos_syscall_num_t syscall_num;  /* System call number */
    dsrtos_syscall_handler_t handler;  /* Handler function */
    uint32_t min_privilege;             /* Minimum privilege level */
    uint32_t flags;                     /* Handler flags */
    const char* name;                   /* System call name */
} dsrtos_syscall_entry_t;

/*=============================================================================
 * SYSTEM CALL FLAGS
 *============================================================================*/
#define DSRTOS_SYSCALL_FLAG_BLOCKING    0x0001U  /* Blocking call */
#define DSRTOS_SYSCALL_FLAG_PRIVILEGED  0x0002U  /* Privileged call */
#define DSRTOS_SYSCALL_FLAG_ATOMIC      0x0004U  /* Atomic operation */
#define DSRTOS_SYSCALL_FLAG_REENTRANT   0x0008U  /* Reentrant call */
#define DSRTOS_SYSCALL_FLAG_VALIDATE    0x0010U  /* Validate params */

/*=============================================================================
 * SYSTEM CALL STATISTICS
 *============================================================================*/
typedef struct {
    uint32_t total_calls;               /* Total syscall count */
    uint32_t failed_calls;              /* Failed syscall count */
    uint32_t max_duration_cycles;      /* Maximum duration */
    uint32_t total_duration_cycles;    /* Total duration */
    uint32_t privilege_violations;     /* Privilege violations */
} dsrtos_syscall_stats_t;

/*=============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize system call interface
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-001: Initialization
 * @safety Must be called during kernel initialization
 */
dsrtos_error_t dsrtos_syscall_init(void);

/**
 * @brief Register system call handler
 * 
 * @param[in] entry System call table entry
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-002: Handler registration
 * @safety Must be called before kernel start
 */
dsrtos_error_t dsrtos_syscall_register(const dsrtos_syscall_entry_t* entry);

/**
 * @brief Unregister system call handler
 * 
 * @param[in] syscall_num System call number
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-003: Handler removal
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_syscall_unregister(dsrtos_syscall_num_t syscall_num);

/**
 * @brief Execute system call
 * 
 * @param[in] syscall_num System call number
 * @param[in] params Parameters
 * @param[out] result Result
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-004: System call execution
 * @safety May block depending on call type
 */
dsrtos_error_t dsrtos_syscall(
    dsrtos_syscall_num_t syscall_num,
    const dsrtos_syscall_params_t* params,
    dsrtos_syscall_result_t* result
);

/**
 * @brief Execute system call from ISR
 * 
 * @param[in] syscall_num System call number
 * @param[in] params Parameters
 * @param[out] result Result
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-005: ISR system calls
 * @safety Non-blocking calls only
 */
dsrtos_error_t dsrtos_syscall_from_isr(
    dsrtos_syscall_num_t syscall_num,
    const dsrtos_syscall_params_t* params,
    dsrtos_syscall_result_t* result
);

/**
 * @brief Get system call statistics
 * 
 * @param[out] stats Statistics structure
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-006: Statistics
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_syscall_get_stats(dsrtos_syscall_stats_t* stats);

/**
 * @brief Reset system call statistics
 * 
 * @return DSRTOS_SUCCESS on success, error code otherwise
 * 
 * @requirements REQ-SYSCALL-007: Statistics reset
 * @safety Thread-safe
 */
dsrtos_error_t dsrtos_syscall_reset_stats(void);

/**
 * @brief Validate system call parameters
 * 
 * @param[in] syscall_num System call number
 * @param[in] params Parameters to validate
 * @return DSRTOS_SUCCESS if valid, error code otherwise
 * 
 * @requirements REQ-SYSCALL-008: Parameter validation
 * @safety Pure function
 */
dsrtos_error_t dsrtos_syscall_validate_params(
    dsrtos_syscall_num_t syscall_num,
    const dsrtos_syscall_params_t* params
);

/*=============================================================================
 * SYSTEM CALL MACROS
 *============================================================================*/

/**
 * @brief System call wrapper macro (0 parameters)
 */
#define DSRTOS_SYSCALL0(num) \
    ({ \
        dsrtos_syscall_params_t _params = {0}; \
        dsrtos_syscall_result_t _result = {0}; \
        dsrtos_syscall((num), &_params, &_result); \
        _result.return_value; \
    })

/**
 * @brief System call wrapper macro (1 parameter)
 */
#define DSRTOS_SYSCALL1(num, p0) \
    ({ \
        dsrtos_syscall_params_t _params = {.param0 = (uint32_t)(p0)}; \
        dsrtos_syscall_result_t _result = {0}; \
        dsrtos_syscall((num), &_params, &_result); \
        _result.return_value; \
    })

/**
 * @brief System call wrapper macro (2 parameters)
 */
#define DSRTOS_SYSCALL2(num, p0, p1) \
    ({ \
        dsrtos_syscall_params_t _params = { \
            .param0 = (uint32_t)(p0), \
            .param1 = (uint32_t)(p1) \
        }; \
        dsrtos_syscall_result_t _result = {0}; \
        dsrtos_syscall((num), &_params, &_result); \
        _result.return_value; \
    })

/**
 * @brief System call wrapper macro (3 parameters)
 */
#define DSRTOS_SYSCALL3(num, p0, p1, p2) \
    ({ \
        dsrtos_syscall_params_t _params = { \
            .param0 = (uint32_t)(p0), \
            .param1 = (uint32_t)(p1), \
            .param2 = (uint32_t)(p2) \
        }; \
        dsrtos_syscall_result_t _result = {0}; \
        dsrtos_syscall((num), &_params, &_result); \
        _result.return_value; \
    })

/**
 * @brief System call wrapper macro (4 parameters)
 */
#define DSRTOS_SYSCALL4(num, p0, p1, p2, p3) \
    ({ \
        dsrtos_syscall_params_t _params = { \
            .param0 = (uint32_t)(p0), \
            .param1 = (uint32_t)(p1), \
            .param2 = (uint32_t)(p2), \
            .param3 = (uint32_t)(p3) \
        }; \
        dsrtos_syscall_result_t _result = {0}; \
        dsrtos_syscall((num), &_params, &_result); \
        _result.return_value; \
    })

/*=============================================================================
 * SVC HANDLER DECLARATION
 *============================================================================*/

/**
 * @brief SVC exception handler
 */
void SVC_Handler(void) __attribute__((naked));

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SYSCALL_H */
