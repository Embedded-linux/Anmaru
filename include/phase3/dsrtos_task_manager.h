/**
 * @file dsrtos_task_manager.h
 * @brief DSRTOS Task Manager Interface - Production-Ready Kernel
 * @details Comprehensive task management system for DSRTOS Phase3
 * 
 * Standards Compliance:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable (Avionics)
 * - IEC 62304 Class B compliant (Medical)
 * - ISO 26262 ASIL D compliant (Automotive)
 * 
 * @version 3.0.0
 * @date 2024-09-03
 */

#ifndef DSRTOS_TASK_MANAGER_H
#define DSRTOS_TASK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../common/dsrtos_error.h"
#include "../common/dsrtos_types.h"

/*==============================================================================
 * CONSTANTS AND CONFIGURATION
 *============================================================================*/

#define DSRTOS_TASK_NAME_MAX_LENGTH        (16U)
#define DSRTOS_TASK_STACK_ALIGNMENT        (8U)
#define DSRTOS_TASK_MIN_STACK_SIZE         (128U)
#define DSRTOS_TASK_MAX_STACK_SIZE         (4096U)
#define DSRTOS_TASK_MAX_PRIORITY           (31U)
#define DSRTOS_TASK_MAX_COUNT              (32U)

/*==============================================================================
 * TASK STATES
 *============================================================================*/

/* dsrtos_task_state_t is defined in common/dsrtos_types.h */

/*==============================================================================
 * TASK PRIORITIES
 *============================================================================*/

typedef enum {
    DSRTOS_TASK_PRIORITY_IDLE      = 0U,
    DSRTOS_TASK_PRIORITY_LOW       = 1U,
    DSRTOS_TASK_PRIORITY_NORMAL    = 2U,
    DSRTOS_TASK_PRIORITY_HIGH      = 3U,
    DSRTOS_TASK_PRIORITY_CRITICAL  = 4U,
    DSRTOS_TASK_PRIORITY_MAX       = 5U
} dsrtos_task_priority_t;

/*==============================================================================
 * TASK ENTRY POINT
 *============================================================================*/

typedef void (*dsrtos_task_entry_t)(void* parameter);

/*==============================================================================
 * CPU CONTEXT STRUCTURE
 *============================================================================*/

typedef struct {
    uint32_t sp;        /* Stack pointer */
    uint32_t lr;        /* Link register */
    uint32_t pc;        /* Program counter */
    uint32_t psr;       /* Program status register */
    uint32_t r0;        /* General purpose registers */
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
} dsrtos_cpu_context_t;

/*==============================================================================
 * TASK CONTROL BLOCK (TCB)
 *============================================================================*/

typedef struct dsrtos_tcb {
    uint32_t task_id;
    char name[DSRTOS_TASK_NAME_MAX_LENGTH];
    dsrtos_task_state_t state;
    dsrtos_task_priority_t priority;
    dsrtos_task_priority_t effective_priority;  /* Effective priority for scheduling */
    void* stack_base;
    void* stack_pointer;  /* Current stack pointer */
    uint32_t stack_size;
    uint32_t stack_usage;
    uint32_t stack_watermark;
    dsrtos_cpu_context_t cpu_context;
    dsrtos_task_entry_t entry_point;
    void* parameter;
    uint32_t cpu_usage;
    uint32_t run_count;
    uint32_t context_switches;
    struct dsrtos_tcb* next;
    struct dsrtos_tcb* prev;
    
    /* Additional Phase3 fields */
    void* task_param;  /* Task parameter (alias for parameter) */
    uint32_t static_priority;
    uint32_t flags;
    uint32_t sched_class;
    uint32_t deadline;
    uint32_t period;
    uint32_t wcet;
    uint32_t cpu_affinity;
    void* exit_handler;
    dsrtos_task_state_t prev_state;
    
    /* Timing information */
    struct {
        uint32_t total_runtime;
        uint32_t last_runtime;
        uint32_t activation_time;
        uint32_t time_slice_remaining;
        uint32_t deadline;
        uint32_t period;
        uint32_t wcet;
        uint32_t response_time;  /* Response time tracking */
        uint32_t jitter;         /* Timing jitter */
    } timing;
    
    /* Resource information */
    struct {
        uint32_t resource_mask;
        uint32_t waiting_on;
    } resources;
    
    /* IPC information */
    struct {
        uint32_t messages_sent;
        uint32_t messages_received;
        uint32_t ipc_blocked_count;
        uint32_t signals_pending;
    } ipc;
    
    /* Statistics */
    struct {
        uint32_t total_runtime;
        uint32_t max_runtime;
        uint32_t min_runtime;
        uint32_t context_switches;
        uint32_t preemptions;
        uint32_t deadline_misses;
    } stats;
    
    /* Phase 4 scheduler fields */
    void* queue_node;        /* Queue node pointer for ready queue */
    uint32_t magic_number;   /* Magic number for validation */
    uint32_t stack_canary;   /* Stack canary for overflow detection */
    uint32_t voluntary_yields; /* Count of voluntary task yields */
} dsrtos_tcb_t;

/* Task Exit Handler */
typedef void (*dsrtos_task_exit_handler_t)(dsrtos_tcb_t* task);

/*==============================================================================
 * TASK CREATION PARAMETERS
 *============================================================================*/

typedef struct {
    char name[DSRTOS_TASK_NAME_MAX_LENGTH];
    dsrtos_task_entry_t entry_point;
    void* parameter;
    void* param;  /* Alias for parameter for compatibility */
    dsrtos_task_priority_t priority;
    uint32_t stack_size;
    void* stack_memory;
    void* stack_buffer;  /* Stack buffer for static allocation */
    uint32_t flags;      /* Task flags */
    uint32_t sched_class; /* Scheduling class */
    uint32_t deadline;   /* Task deadline */
    uint32_t period;     /* Task period */
    uint32_t wcet;       /* Worst case execution time */
    uint32_t cpu_affinity; /* CPU affinity mask */
    dsrtos_task_exit_handler_t exit_handler; /* Custom exit handler */
} dsrtos_task_params_t;

/*==============================================================================
 * TASK STATISTICS
 *============================================================================*/

typedef struct {
    uint32_t total_tasks;
    uint32_t active_tasks;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
    uint32_t suspended_tasks;
    uint32_t total_context_switches;
    uint32_t total_cpu_usage;
    uint32_t peak_stack_usage;
    uint32_t total_errors;
} dsrtos_task_manager_stats_t;

/*==============================================================================
 * TASK MANAGER FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize the task manager
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_init(void);

/**
 * @brief Create a new task
 * @param params Task creation parameters
 * @param task_id Pointer to store the created task ID
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_create_task(const dsrtos_task_params_t* params, 
                                               uint32_t* task_id);

/**
 * @brief Delete a task
 * @param task_id ID of the task to delete
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_delete_task(uint32_t task_id);

/**
 * @brief Suspend a task
 * @param task_id ID of the task to suspend
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_suspend_task(uint32_t task_id);

/**
 * @brief Resume a suspended task
 * @param task_id ID of the task to resume
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_resume_task(uint32_t task_id);

/**
 * @brief Get task information
 * @param task_id ID of the task
 * @param tcb Pointer to store task control block
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_get_task_info(uint32_t task_id, dsrtos_tcb_t* tcb);

/**
 * @brief Get task by index
 * @param index Task index
 * @return Pointer to task control block, NULL if not found
 */
dsrtos_tcb_t* dsrtos_task_get_by_index(uint32_t index);

/**
 * @brief Get current running task
 * @return Pointer to current task control block, NULL if none
 */
dsrtos_tcb_t* dsrtos_task_get_current(void);

/**
 * @brief Set task priority
 * @param task_id ID of the task
 * @param priority New priority level
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_set_priority(uint32_t task_id, dsrtos_task_priority_t priority);

/**
 * @brief Get task priority
 * @param task_id ID of the task
 * @param priority Pointer to store priority
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_get_priority(uint32_t task_id, dsrtos_task_priority_t* priority);

/**
 * @brief Get task statistics
 * @param stats Pointer to store statistics
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_get_stats(dsrtos_task_manager_stats_t* stats);

/**
 * @brief Reset task statistics
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_reset_stats(void);

/**
 * @brief Validate task manager integrity
 * @return DSRTOS_SUCCESS if valid, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_validate(void);

/**
 * @brief Shutdown task manager
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_manager_shutdown(void);

/*==============================================================================
 * TASK SCHEDULER FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize task scheduler
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_init(void);

/**
 * @brief Start task scheduler
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_start(void);

/**
 * @brief Stop task scheduler
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_stop(void);

/**
 * @brief Perform context switch
 * @param from_task Task to switch from
 * @param to_task Task to switch to
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_context_switch(dsrtos_tcb_t* from_task, dsrtos_tcb_t* to_task);

/**
 * @brief Get next ready task
 * @return Pointer to next ready task, NULL if none
 */
dsrtos_tcb_t* dsrtos_scheduler_get_next_ready(void);

/**
 * @brief Add task to ready queue
 * @param task Task to add
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_add_ready(dsrtos_tcb_t* task);

/**
 * @brief Remove task from ready queue
 * @param task Task to remove
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_scheduler_remove_ready(dsrtos_tcb_t* task);

/*==============================================================================
 * TASK SYNCHRONIZATION FUNCTIONS
 *============================================================================*/

/**
 * @brief Yield current task
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_yield(void);

/**
 * @brief Delay task execution
 * @param delay_ms Delay in milliseconds
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_delay(uint32_t delay_ms);

/**
 * @brief Block current task
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_block(void);

/**
 * @brief Unblock a task
 * @param task_id ID of the task to unblock
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_unblock(uint32_t task_id);

/*==============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/**
 * @brief Get task ID by name
 * @param name Task name
 * @param task_id Pointer to store task ID
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_get_id_by_name(const char* name, uint32_t* task_id);

/**
 * @brief Check if task exists
 * @param task_id ID of the task
 * @return true if task exists, false otherwise
 */
bool dsrtos_task_exists(uint32_t task_id);

/**
 * @brief Get task count
 * @return Number of active tasks
 */
uint32_t dsrtos_task_get_count(void);

/**
 * @brief Get maximum task count
 * @return Maximum number of tasks
 */
uint32_t dsrtos_task_get_max_count(void);

/**
 * @brief Convert task state to string
 * @param state Task state
 * @return String representation of task state
 */
const char* dsrtos_task_state_to_string(dsrtos_task_state_t state);

/**
 * @brief Convert task priority to string
 * @param priority Task priority
 * @return String representation of task priority
 */
const char* dsrtos_task_priority_to_string(dsrtos_task_priority_t priority);

/*==============================================================================
 * MISSING FUNCTION DECLARATIONS FOR PHASE3 COMPATIBILITY
 *============================================================================*/

/**
 * @brief Suspend a task (Phase3 compatibility)
 * @param tcb Task control block to suspend
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_suspend(dsrtos_tcb_t* tcb);

/* dsrtos_port_init_stack is declared in dsrtos_port.h */

/**
 * @brief Get system time (Phase3 compatibility)
 * @return Current system time in ticks
 */
uint64_t dsrtos_get_system_time(void);

/**
 * @brief Task exit function (Phase3 compatibility)
 * @note Called when a task completes execution
 */
void dsrtos_task_exit(void);

/**
 * @brief Create a task (Phase3 compatibility)
 * @param tcb Pointer to task control block
 * @param params Task creation parameters
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_create(dsrtos_tcb_t** tcb, const dsrtos_task_params_t* params);

/**
 * @brief Create a static task (Phase3 compatibility)
 * @param tcb Pointer to task control block
 * @param params Task creation parameters
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_create_static(dsrtos_tcb_t* tcb, const dsrtos_task_params_t* params);

/**
 * @brief Delete a task (Phase3 compatibility)
 * @param tcb Task control block to delete
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_delete(dsrtos_tcb_t* tcb);

/**
 * @brief Validate TCB (Phase3 compatibility)
 * @param tcb Task control block to validate
 * @return DSRTOS_SUCCESS if valid, error code on failure
 */
dsrtos_error_t dsrtos_task_validate_tcb(const dsrtos_tcb_t* tcb);

/**
 * @brief Insert task into ready queue (Phase3 compatibility)
 * @param tcb Task control block to insert
 * @return DSRTOS_SUCCESS on success, error code on failure
 */
dsrtos_error_t dsrtos_task_ready_insert(dsrtos_tcb_t* tcb);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_TASK_MANAGER_H */