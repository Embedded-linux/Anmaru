/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Plugin Helper Implementation
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_scheduler_plugin.h"
#include "dsrtos_scheduler_core.h"
#include "dsrtos_critical.h"
#include "dsrtos_memory.h"
#include "dsrtos_assert.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define PLUGIN_ASSERT(cond)     DSRTOS_ASSERT(cond)
#define PLUGIN_TRACE(msg)       DSRTOS_TRACE(TRACE_SCHEDULER, msg)

/* Bit manipulation for priority bitmap */
#define BITMAP_INDEX(prio)      ((prio) >> 5U)
#define BITMAP_MASK(prio)       (1U << ((prio) & 31U))

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Plugin helper control block */
typedef struct {
    /* Statistics */
    struct {
        uint32_t total_enqueues;
        uint32_t total_dequeues;
        uint32_t bitmap_operations;
        uint32_t priority_calculations;
    } stats;
    
    /* IPC tracking */
    struct {
        uint32_t messages_sent;
        uint32_t messages_received;
        uint64_t last_ipc_time;
        uint32_t ipc_rate_samples[16];
        uint32_t sample_index;
    } ipc;
    
    /* Resource tracking */
    struct {
        uint32_t resource_usage[32];
        uint32_t resource_owners[32];
        uint32_t contention_count[32];
    } resources;
} plugin_helper_cb_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static plugin_helper_cb_t g_plugin_helper = {0};

/* Helper function table */
static const dsrtos_plugin_helpers_t g_helpers = {
    .set_priority_bit = dsrtos_plugin_set_priority_bit,
    .clear_priority_bit = dsrtos_plugin_clear_priority_bit,
    .find_highest_priority = dsrtos_plugin_find_highest_priority,
    .is_priority_set = dsrtos_plugin_is_priority_set,
    .enqueue_task_priority = dsrtos_plugin_enqueue_priority,
    .dequeue_task_priority = dsrtos_plugin_dequeue_priority,
    .calculate_time_slice = dsrtos_plugin_calculate_time_slice,
    .calculate_vruntime = NULL, /* Implemented by specific schedulers */
    .calculate_dynamic_priority = dsrtos_plugin_calculate_nice_priority,
    .apply_priority_boost = dsrtos_plugin_apply_priority_inheritance,
    .track_resource_usage = dsrtos_plugin_track_resource,
    .get_resource_contention = dsrtos_plugin_get_resource_usage
};

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize common plugin data
 * @param data Pointer to data pointer
 * @param params Initialization parameters
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_plugin_common_init(
    dsrtos_plugin_common_data_t** data,
    const dsrtos_plugin_init_params_t* params)
{
    dsrtos_plugin_common_data_t* common;
    
    PLUGIN_ASSERT(data != NULL);
    PLUGIN_ASSERT(params != NULL);
    
    /* Allocate common data */
    common = (dsrtos_plugin_common_data_t*)dsrtos_malloc(
        sizeof(dsrtos_plugin_common_data_t));
    if (common == NULL) {
        return DSRTOS_ERROR_NO_MEMORY;
    }
    
    /* Clear the structure */
    (void)memset(common, 0, sizeof(dsrtos_plugin_common_data_t));
    
    /* Initialize ready queues */
    for (uint32_t i = 0U; i < params->priority_levels; i++) {
        dsrtos_queue_init(&common->ready_queues[i]);
    }
    
    /* Set time slice */
    common->time_slice_ticks = (params->time_slice_us * DSRTOS_TICK_RATE_HZ) / 1000000U;
    if (common->time_slice_ticks == 0U) {
        common->time_slice_ticks = 1U;
    }
    
    *data = common;
    
    PLUGIN_TRACE("Plugin common data initialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Deinitialize common plugin data
 * @param data Common data to free
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_plugin_common_deinit(dsrtos_plugin_common_data_t* data)
{
    if (data == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear queues */
    for (uint32_t i = 0U; i < DSRTOS_MAX_PRIORITIES; i++) {
        /* Remove any remaining tasks */
        while (!dsrtos_queue_is_empty(&data->ready_queues[i])) {
            (void)dsrtos_queue_dequeue(&data->ready_queues[i]);
        }
    }
    
    /* Free the structure */
    dsrtos_free(data);
    
    PLUGIN_TRACE("Plugin common data deinitialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Set priority bit in bitmap
 * @param bitmap Priority bitmap
 * @param priority Priority level
 */
void dsrtos_plugin_set_priority_bit(uint32_t* bitmap, uint8_t priority)
{
    uint32_t sr;
    
    PLUGIN_ASSERT(bitmap != NULL);
    PLUGIN_ASSERT(priority < DSRTOS_MAX_PRIORITIES);
    
    sr = dsrtos_critical_enter();
    bitmap[BITMAP_INDEX(priority)] |= BITMAP_MASK(priority);
    g_plugin_helper.stats.bitmap_operations++;
    dsrtos_critical_exit(sr);
}

/**
 * @brief Clear priority bit in bitmap
 * @param bitmap Priority bitmap
 * @param priority Priority level
 */
void dsrtos_plugin_clear_priority_bit(uint32_t* bitmap, uint8_t priority)
{
    uint32_t sr;
    
    PLUGIN_ASSERT(bitmap != NULL);
    PLUGIN_ASSERT(priority < DSRTOS_MAX_PRIORITIES);
    
    sr = dsrtos_critical_enter();
    bitmap[BITMAP_INDEX(priority)] &= ~BITMAP_MASK(priority);
    g_plugin_helper.stats.bitmap_operations++;
    dsrtos_critical_exit(sr);
}

/**
 * @brief Find highest priority set in bitmap
 * @param bitmap Priority bitmap
 * @return Highest priority or 255 if none set
 */
uint8_t dsrtos_plugin_find_highest_priority(const uint32_t* bitmap)
{
    int32_t word_idx;
    uint32_t word;
    uint8_t bit_idx;
    
    PLUGIN_ASSERT(bitmap != NULL);
    
    /* Search from highest priority (MSB) to lowest */
    for (word_idx = (int32_t)(DSRTOS_PRIORITY_BITMAP_SIZE - 1U); 
         word_idx >= 0; word_idx--) {
        word = bitmap[word_idx];
        if (word != 0U) {
            /* Find MSB in word using CLZ if available */
#ifdef __ARM_ARCH_7M__
            bit_idx = 31U - (uint8_t)__builtin_clz(word);
#else
            /* Software implementation */
            bit_idx = 31U;
            while (((word & (1U << bit_idx)) == 0U) && (bit_idx > 0U)) {
                bit_idx--;
            }
#endif
            return (uint8_t)((word_idx * 32U) + bit_idx);
        }
    }
    
    return 255U; /* No priority set */
}

/**
 * @brief Check if priority is set in bitmap
 * @param bitmap Priority bitmap
 * @param priority Priority to check
 * @return true if set, false otherwise
 */
bool dsrtos_plugin_is_priority_set(const uint32_t* bitmap, uint8_t priority)
{
    PLUGIN_ASSERT(bitmap != NULL);
    PLUGIN_ASSERT(priority < DSRTOS_MAX_PRIORITIES);
    
    return (bitmap[BITMAP_INDEX(priority)] & BITMAP_MASK(priority)) != 0U;
}

/**
 * @brief Enqueue task at priority level
 * @param queues Array of priority queues
 * @param task Task to enqueue
 * @param priority Priority level
 */
void dsrtos_plugin_enqueue_priority(
    dsrtos_queue_t* queues, dsrtos_tcb_t* task, uint8_t priority)
{
    uint32_t sr;
    
    PLUGIN_ASSERT(queues != NULL);
    PLUGIN_ASSERT(task != NULL);
    PLUGIN_ASSERT(priority < DSRTOS_MAX_PRIORITIES);
    
    sr = dsrtos_critical_enter();
    
    /* Add to appropriate queue */
    dsrtos_queue_enqueue(&queues[priority], &task->queue_node);
    
    g_plugin_helper.stats.total_enqueues++;
    
    dsrtos_critical_exit(sr);
}

/**
 * @brief Dequeue task from priority level
 * @param queues Array of priority queues
 * @param priority Priority level
 * @return Dequeued task or NULL
 */
dsrtos_tcb_t* dsrtos_plugin_dequeue_priority(
    dsrtos_queue_t* queues, uint8_t priority)
{
    uint32_t sr;
    dsrtos_queue_node_t* node;
    dsrtos_tcb_t* task = NULL;
    
    PLUGIN_ASSERT(queues != NULL);
    PLUGIN_ASSERT(priority < DSRTOS_MAX_PRIORITIES);
    
    sr = dsrtos_critical_enter();
    
    node = dsrtos_queue_dequeue(&queues[priority]);
    if (node != NULL) {
        /* Get task from queue node */
        task = CONTAINER_OF(node, dsrtos_tcb_t, queue_node);
        g_plugin_helper.stats.total_dequeues++;
    }
    
    dsrtos_critical_exit(sr);
    
    return task;
}

/**
 * @brief Calculate time slice for task
 * @param task Task
 * @param default_slice Default time slice
 * @return Calculated time slice
 */
uint32_t dsrtos_plugin_calculate_time_slice(
    dsrtos_tcb_t* task, uint32_t default_slice)
{
    uint32_t slice;
    
    PLUGIN_ASSERT(task != NULL);
    
    /* Base slice on priority */
    if (task->effective_priority < 64U) {
        /* High priority - shorter slice */
        slice = default_slice / 2U;
    } else if (task->effective_priority < 128U) {
        /* Normal priority - default slice */
        slice = default_slice;
    } else {
        /* Low priority - longer slice */
        slice = default_slice * 2U;
    }
    
    /* Ensure minimum slice */
    if (slice == 0U) {
        slice = 1U;
    }
    
    g_plugin_helper.stats.priority_calculations++;
    
    return slice;
}

/**
 * @brief Update task runtime
 * @param task Task
 * @param elapsed_ticks Elapsed ticks
 */
void dsrtos_plugin_update_runtime(dsrtos_tcb_t* task, uint32_t elapsed_ticks)
{
    PLUGIN_ASSERT(task != NULL);
    
    /* Update total runtime */
    task->total_runtime += elapsed_ticks;
    
    /* Update time slice */
    if (task->time_slice_remaining > elapsed_ticks) {
        task->time_slice_remaining -= elapsed_ticks;
    } else {
        task->time_slice_remaining = 0U;
    }
}

/**
 * @brief Calculate priority from nice value
 * @param nice_value Nice value (-20 to 19)
 * @return Priority (0-255)
 */
uint8_t dsrtos_plugin_calculate_nice_priority(int8_t nice_value)
{
    int16_t priority;
    
    /* Map nice value to priority */
    /* Nice -20 = priority 0 (highest) */
    /* Nice 0 = priority 128 (normal) */
    /* Nice 19 = priority 255 (lowest) */
    
    priority = 128 + (nice_value * 6);
    
    /* Clamp to valid range */
    if (priority < 0) {
        priority = 0;
    } else if (priority > 255) {
        priority = 255;
    }
    
    return (uint8_t)priority;
}

/**
 * @brief Apply priority inheritance
 * @param task Task
 * @param inherited_priority Inherited priority
 */
void dsrtos_plugin_apply_priority_inheritance(
    dsrtos_tcb_t* task, uint8_t inherited_priority)
{
    PLUGIN_ASSERT(task != NULL);
    
    /* Only inherit if higher priority */
    if (inherited_priority < task->effective_priority) {
        task->effective_priority = inherited_priority;
        PLUGIN_TRACE("Priority inheritance applied");
    }
}

/**
 * @brief Remove priority inheritance
 * @param task Task
 */
void dsrtos_plugin_remove_priority_inheritance(dsrtos_tcb_t* task)
{
    PLUGIN_ASSERT(task != NULL);
    
    /* Restore original priority */
    task->effective_priority = task->static_priority;
    PLUGIN_TRACE("Priority inheritance removed");
}

/**
 * @brief Track resource acquisition/release
 * @param task Task
 * @param resource_id Resource ID
 * @param acquired true if acquired, false if released
 */
void dsrtos_plugin_track_resource(
    dsrtos_tcb_t* task, uint32_t resource_id, bool acquired)
{
    uint32_t sr;
    
    PLUGIN_ASSERT(task != NULL);
    PLUGIN_ASSERT(resource_id < 32U);
    
    sr = dsrtos_critical_enter();
    
    if (acquired) {
        g_plugin_helper.resources.resource_usage[resource_id]++;
        g_plugin_helper.resources.resource_owners[resource_id] = task->task_id;
        task->resource_mask |= (1U << resource_id);
    } else {
        if (g_plugin_helper.resources.resource_usage[resource_id] > 0U) {
            g_plugin_helper.resources.resource_usage[resource_id]--;
        }
        if (g_plugin_helper.resources.resource_owners[resource_id] == task->task_id) {
            g_plugin_helper.resources.resource_owners[resource_id] = 0U;
        }
        task->resource_mask &= ~(1U << resource_id);
    }
    
    dsrtos_critical_exit(sr);
}

/**
 * @brief Get resource usage/contention
 * @param resource_id Resource ID
 * @return Usage count
 */
uint32_t dsrtos_plugin_get_resource_usage(uint32_t resource_id)
{
    uint32_t usage;
    
    if (resource_id >= 32U) {
        return 0U;
    }
    
    usage = g_plugin_helper.resources.resource_usage[resource_id];
    
    /* Track contention */
    if (usage > 1U) {
        g_plugin_helper.resources.contention_count[resource_id]++;
    }
    
    return usage;
}

/**
 * @brief Track IPC operation
 * @param sender Sending task
 * @param receiver Receiving task
 */
void dsrtos_plugin_track_ipc(dsrtos_tcb_t* sender, dsrtos_tcb_t* receiver)
{
    uint32_t sr;
    uint64_t current_time;
    uint32_t time_diff;
    
    PLUGIN_ASSERT(sender != NULL);
    PLUGIN_ASSERT(receiver != NULL);
    
    sr = dsrtos_critical_enter();
    
    /* Update counters */
    sender->ipc.messages_sent++;
    receiver->ipc.messages_received++;
    g_plugin_helper.ipc.messages_sent++;
    g_plugin_helper.ipc.messages_received++;
    
    /* Calculate IPC rate */
    current_time = dsrtos_get_system_time();
    if (g_plugin_helper.ipc.last_ipc_time > 0U) {
        time_diff = (uint32_t)(current_time - g_plugin_helper.ipc.last_ipc_time);
        if (time_diff > 0U) {
            uint32_t rate = 1000000U / time_diff; /* Messages per second */
            g_plugin_helper.ipc.ipc_rate_samples[g_plugin_helper.ipc.sample_index] = rate;
            g_plugin_helper.ipc.sample_index = 
                (g_plugin_helper.ipc.sample_index + 1U) & 15U;
        }
    }
    g_plugin_helper.ipc.last_ipc_time = current_time;
    
    dsrtos_critical_exit(sr);
}

/**
 * @brief Get current IPC rate
 * @return Average IPC rate in messages/second
 */
uint32_t dsrtos_plugin_get_ipc_rate(void)
{
    uint32_t sr;
    uint32_t total = 0U;
    uint32_t count = 0U;
    
    sr = dsrtos_critical_enter();
    
    /* Calculate average from samples */
    for (uint32_t i = 0U; i < 16U; i++) {
        if (g_plugin_helper.ipc.ipc_rate_samples[i] > 0U) {
            total += g_plugin_helper.ipc.ipc_rate_samples[i];
            count++;
        }
    }
    
    dsrtos_critical_exit(sr);
    
    if (count > 0U) {
        return total / count;
    }
    
    return 0U;
}

/**
 * @brief Validate task structure
 * @param task Task to validate
 * @return true if valid
 */
bool dsrtos_plugin_validate_task(dsrtos_tcb_t* task)
{
    if (task == NULL) {
        return false;
    }
    
    /* Check magic number */
    if (task->magic_number != DSRTOS_TASK_MAGIC) {
        return false;
    }
    
    /* Check state */
    if (task->state > DSRTOS_TASK_STATE_TERMINATED) {
        return false;
    }
    
    /* Check stack */
    if ((task->stack_base == NULL) || (task->stack_size < DSRTOS_MIN_STACK_SIZE)) {
        return false;
    }
    
    /* Check stack canary */
    if (task->stack_canary != DSRTOS_STACK_CANARY_VALUE) {
        return false;
    }
    
    return true;
}

/**
 * @brief Validate priority value
 * @param priority Priority to validate
 * @return true if valid
 */
bool dsrtos_plugin_validate_priority(uint8_t priority)
{
    return priority < DSRTOS_MAX_PRIORITIES;
}

/**
 * @brief Get plugin helper functions
 * @return Pointer to helper function table
 */
const dsrtos_plugin_helpers_t* dsrtos_plugin_get_helpers(void)
{
    return &g_helpers;
}
