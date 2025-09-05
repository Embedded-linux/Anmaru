/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Plugin Interface
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

#ifndef DSRTOS_SCHEDULER_PLUGIN_H
#define DSRTOS_SCHEDULER_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_scheduler_core.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_queue.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
/* Plugin development helpers */
#define DSRTOS_PLUGIN_NAME(name)        #name
#define DSRTOS_PLUGIN_VERSION(maj,min)  ((maj << 8) | min)

/* Standard plugin return codes */
#define DSRTOS_PLUGIN_SUCCESS            (0)
#define DSRTOS_PLUGIN_ERROR             (-1)
#define DSRTOS_PLUGIN_NOT_SUPPORTED     (-2)

/* Plugin priority queue helpers */
#define DSRTOS_PRIORITY_QUEUE_SIZE      (256U)
#define DSRTOS_PRIORITY_BITMAP_SIZE     (8U)   /* 256 bits / 32 */

/*==============================================================================
 *                                  TYPES
 *============================================================================*/
/* Common plugin data structure */
typedef struct {
    /* Ready queue management */
    dsrtos_queue_t ready_queues[DSRTOS_MAX_PRIORITIES];
    uint32_t priority_bitmap[DSRTOS_PRIORITY_BITMAP_SIZE];
    uint32_t ready_task_count;
    
    /* Current task tracking */
    dsrtos_tcb_t* current_task;
    dsrtos_tcb_t* idle_task;
    
    /* Time management */
    uint32_t time_slice_ticks;
    uint32_t current_slice_remaining;
    
    /* Statistics */
    uint32_t schedule_count;
    uint32_t preemption_count;
    uint64_t total_runtime_ns;
} dsrtos_plugin_common_data_t;

/* Plugin initialization parameters */
typedef struct {
    uint32_t max_tasks;
    uint32_t priority_levels;
    uint32_t time_slice_us;
    void* custom_config;
} dsrtos_plugin_init_params_t;

/* Plugin helper functions */
typedef struct {
    /* Priority bitmap operations */
    void (*set_priority_bit)(uint32_t* bitmap, uint8_t priority);
    void (*clear_priority_bit)(uint32_t* bitmap, uint8_t priority);
    uint8_t (*find_highest_priority)(const uint32_t* bitmap);
    bool (*is_priority_set)(const uint32_t* bitmap, uint8_t priority);
    
    /* Queue operations */
    void (*enqueue_task_priority)(dsrtos_queue_t* queues, 
                                  dsrtos_tcb_t* task, uint8_t priority);
    dsrtos_tcb_t* (*dequeue_task_priority)(dsrtos_queue_t* queues, 
                                           uint8_t priority);
    
    /* Time calculations */
    uint32_t (*calculate_time_slice)(dsrtos_tcb_t* task);
    uint32_t (*calculate_vruntime)(dsrtos_tcb_t* task, uint32_t runtime);
    
    /* Priority calculations */
    uint8_t (*calculate_dynamic_priority)(dsrtos_tcb_t* task);
    void (*apply_priority_boost)(dsrtos_tcb_t* task, uint8_t boost);
    
    /* Resource tracking */
    void (*track_resource_usage)(dsrtos_tcb_t* task, uint32_t resource_id);
    uint32_t (*get_resource_contention)(uint32_t resource_id);
} dsrtos_plugin_helpers_t;

/*==============================================================================
 *                           FUNCTION PROTOTYPES
 *============================================================================*/
/* Plugin registration helper */
dsrtos_error_t dsrtos_plugin_register(
    dsrtos_scheduler_plugin_t* plugin,
    const dsrtos_plugin_init_params_t* params);

/* Common plugin operations */
dsrtos_error_t dsrtos_plugin_common_init(
    dsrtos_plugin_common_data_t** data,
    const dsrtos_plugin_init_params_t* params);

dsrtos_error_t dsrtos_plugin_common_deinit(
    dsrtos_plugin_common_data_t* data);

/* Priority bitmap helpers */
void dsrtos_plugin_set_priority_bit(
    uint32_t* bitmap, uint8_t priority);

void dsrtos_plugin_clear_priority_bit(
    uint32_t* bitmap, uint8_t priority);

uint8_t dsrtos_plugin_find_highest_priority(
    const uint32_t* bitmap);

bool dsrtos_plugin_is_priority_set(
    const uint32_t* bitmap, uint8_t priority);

/* Queue management helpers */
void dsrtos_plugin_enqueue_priority(
    dsrtos_queue_t* queues, dsrtos_tcb_t* task, uint8_t priority);

dsrtos_tcb_t* dsrtos_plugin_dequeue_priority(
    dsrtos_queue_t* queues, uint8_t priority);

void dsrtos_plugin_remove_from_queue(
    dsrtos_queue_t* queue, dsrtos_tcb_t* task);

/* Time management helpers */
uint32_t dsrtos_plugin_calculate_time_slice(
    dsrtos_tcb_t* task, uint32_t default_slice);

void dsrtos_plugin_update_runtime(
    dsrtos_tcb_t* task, uint32_t elapsed_ticks);

/* Priority management helpers */
uint8_t dsrtos_plugin_calculate_nice_priority(
    int8_t nice_value);

void dsrtos_plugin_apply_priority_inheritance(
    dsrtos_tcb_t* task, uint8_t inherited_priority);

void dsrtos_plugin_remove_priority_inheritance(
    dsrtos_tcb_t* task);

/* Resource tracking helpers */
void dsrtos_plugin_track_resource(
    dsrtos_tcb_t* task, uint32_t resource_id, bool acquired);

uint32_t dsrtos_plugin_get_resource_usage(
    uint32_t resource_id);

/* IPC tracking helpers */
void dsrtos_plugin_track_ipc(
    dsrtos_tcb_t* sender, dsrtos_tcb_t* receiver);

uint32_t dsrtos_plugin_get_ipc_rate(void);

/* Validation helpers */
bool dsrtos_plugin_validate_task(dsrtos_tcb_t* task);
bool dsrtos_plugin_validate_priority(uint8_t priority);
bool dsrtos_plugin_validate_config(const void* config);

/* Get helper functions */
const dsrtos_plugin_helpers_t* dsrtos_plugin_get_helpers(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_PLUGIN_H */
