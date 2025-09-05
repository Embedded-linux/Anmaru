/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Development Team
 * 
 * File: dsrtos_scheduler_rr.c
 * Description: Round Robin Scheduler Implementation
 * Phase: 6 - Concrete Scheduler Implementations
 * 
 * Performance Requirements:
 * - Schedule decision: < 5μs
 * - Enqueue operation: < 2μs
 * - Dequeue operation: < 2μs
 */

#include "dsrtos_scheduler_rr.h"
#include "dsrtos_kernel.h"
#include "dsrtos_port.h"
#include <string.h>

/* ============================================================================
 * STATIC FUNCTION PROTOTYPES
 * ============================================================================ */

static dsrtos_status_t rr_plugin_init(void* scheduler);
static dsrtos_status_t rr_plugin_schedule(void* scheduler, dsrtos_tcb_t** next_task);
static dsrtos_status_t rr_plugin_add_task(void* scheduler, dsrtos_tcb_t* task);
static dsrtos_status_t rr_plugin_remove_task(void* scheduler, dsrtos_tcb_t* task);
static dsrtos_status_t rr_plugin_tick(void* scheduler);
static uint32_t rr_find_first_free_node(dsrtos_rr_scheduler_t* scheduler);
static void rr_mark_node_used(dsrtos_rr_scheduler_t* scheduler, uint32_t index);
static void rr_mark_node_free(dsrtos_rr_scheduler_t* scheduler, uint32_t index);
static void rr_queue_push_back(dsrtos_rr_queue_t* queue, dsrtos_rr_node_t* node);
static dsrtos_rr_node_t* rr_queue_pop_front(dsrtos_rr_queue_t* queue);
static void rr_queue_remove_node(dsrtos_rr_queue_t* queue, dsrtos_rr_node_t* node);
static void rr_adjust_dynamic_slice(dsrtos_rr_scheduler_t* scheduler);
static void rr_check_and_handle_starvation(dsrtos_rr_scheduler_t* scheduler);

/* ============================================================================
 * PLUGIN INTERFACE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize Round Robin scheduler
 * MISRA-C:2012 Rule 8.13: scheduler parameter cannot be const
 */
dsrtos_status_t dsrtos_rr_init(dsrtos_rr_scheduler_t* scheduler,
                               uint32_t time_slice_ms)
{
    uint32_t i;
    
    /* Validate parameters */
    if (scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    if ((time_slice_ms < RR_MIN_TIMESLICE_MS) || 
        (time_slice_ms > RR_MAX_TIMESLICE_MS)) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Clear scheduler structure */
    (void)memset(scheduler, 0, sizeof(dsrtos_rr_scheduler_t));
    
    /* Initialize base plugin interface */
    scheduler->base.magic = SCHEDULER_MAGIC;
    scheduler->base.scheduler_id = SCHEDULER_ID_ROUND_ROBIN;
    scheduler->base.priority = 128U;  /* Medium priority */
    scheduler->base.state = SCHEDULER_STATE_INACTIVE;
    (void)strncpy(scheduler->base.name, "RoundRobin", SCHEDULER_NAME_MAX - 1U);
    
    /* Set plugin operations */
    scheduler->base.ops.init = rr_plugin_init;
    scheduler->base.ops.schedule = rr_plugin_schedule;
    scheduler->base.ops.add_task = rr_plugin_add_task;
    scheduler->base.ops.remove_task = rr_plugin_remove_task;
    scheduler->base.ops.tick = rr_plugin_tick;
    scheduler->base.ops.get_stats = NULL;  /* Use RR-specific stats */
    scheduler->base.ops.reset = (dsrtos_status_t (*)(void*))dsrtos_rr_reset;
    
    /* Initialize configuration */
    scheduler->time_slice_ms = time_slice_ms;
    scheduler->min_slice_ms = RR_MIN_TIMESLICE_MS;
    scheduler->max_slice_ms = RR_MAX_TIMESLICE_MS;
    scheduler->dynamic_slice = false;
    
    /* Initialize ready queue */
    scheduler->ready_queue.head = NULL;
    scheduler->ready_queue.tail = NULL;
    scheduler->ready_queue.count = 0U;
    scheduler->ready_queue.integrity_check = RR_QUEUE_INTEGRITY_CHECK;
    scheduler->ready_queue.enqueue_count = 0U;
    scheduler->ready_queue.dequeue_count = 0U;
    
    /* Initialize node pool bitmap (all nodes free) */
    for (i = 0U; i < 8U; i++) {
        scheduler->node_pool_bitmap[i] = 0U;  /* 0 = free, 1 = used */
    }
    
    /* Initialize fairness parameters */
    scheduler->starvation_threshold = RR_STARVATION_THRESHOLD_MS;
    scheduler->boost_priority = RR_STARVATION_BOOST_FACTOR;
    scheduler->starving_list = NULL;
    
    /* Initialize current state */
    scheduler->current_task = NULL;
    scheduler->slice_remaining = 0U;
    scheduler->slice_start_time = 0U;
    scheduler->slice_extensions = 0U;
    
    /* Initialize statistics */
    scheduler->stats.min_schedule_time_us = UINT32_MAX;
    scheduler->stats.max_schedule_time_us = 0U;
    scheduler->stats.avg_schedule_time_us = 0U;
    scheduler->stats.fairness_index = 100U;  /* Start with perfect fairness */
    
    /* Initialize synchronization */
    scheduler->lock = 0U;
    scheduler->critical_section_count = 0U;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Start Round Robin scheduler
 */
dsrtos_status_t dsrtos_rr_start(dsrtos_rr_scheduler_t* scheduler)
{
    RR_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (scheduler->base.state == SCHEDULER_STATE_RUNNING) {
        return DSRTOS_ALREADY_EXISTS;
    }
    
    RR_ENTER_CRITICAL(scheduler);
    
    scheduler->base.state = SCHEDULER_STATE_RUNNING;
    scheduler->slice_remaining = scheduler->time_slice_ms;
    scheduler->slice_start_time = dsrtos_get_tick_count();
    
    RR_EXIT_CRITICAL(scheduler);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Stop Round Robin scheduler
 */
dsrtos_status_t dsrtos_rr_stop(dsrtos_rr_scheduler_t* scheduler)
{
    RR_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (scheduler->base.state != SCHEDULER_STATE_RUNNING) {
        return DSRTOS_INVALID_STATE;
    }
    
    RR_ENTER_CRITICAL(scheduler);
    
    scheduler->base.state = SCHEDULER_STATE_SUSPENDED;
    scheduler->current_task = NULL;
    scheduler->slice_remaining = 0U;
    
    RR_EXIT_CRITICAL(scheduler);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Select next task to run
 * Performance critical: Must complete in < 5μs
 */
dsrtos_tcb_t* dsrtos_rr_select_next(dsrtos_rr_scheduler_t* scheduler)
{
    dsrtos_rr_node_t* node;
    dsrtos_tcb_t* next_task = NULL;
    uint32_t start_cycles;
    uint32_t end_cycles;
    uint32_t schedule_time_us;
    
    if ((scheduler == NULL) || 
        (scheduler->base.state != SCHEDULER_STATE_RUNNING)) {
        return NULL;
    }
    
    /* Performance measurement start */
    start_cycles = dsrtos_port_get_cycle_count();
    
    RR_ENTER_CRITICAL(scheduler);
    
    /* Check queue integrity */
    if (!dsrtos_rr_queue_verify_integrity(&scheduler->ready_queue)) {
        RR_EXIT_CRITICAL(scheduler);
        return NULL;
    }
    
    /* Check for starvation if enabled */
    if (scheduler->starvation_threshold > 0U) {
        rr_check_and_handle_starvation(scheduler);
    }
    
    /* Get next task from front of queue */
    node = rr_queue_pop_front(&scheduler->ready_queue);
    if (node != NULL) {
        next_task = node->task;
        
        /* If current task is still ready, put it back in queue */
        if ((scheduler->current_task != NULL) && 
            (scheduler->current_task != next_task) &&
            (scheduler->current_task->state == TASK_STATE_READY)) {
            
            dsrtos_rr_node_t* current_node = dsrtos_rr_alloc_node(scheduler);
            if (current_node != NULL) {
                current_node->task = scheduler->current_task;
                current_node->enqueue_time = dsrtos_get_tick_count();
                rr_queue_push_back(&scheduler->ready_queue, current_node);
            }
        }
        
        /* Update current task */
        scheduler->current_task = next_task;
        scheduler->slice_remaining = scheduler->time_slice_ms;
        scheduler->slice_start_time = dsrtos_get_tick_count();
        scheduler->slice_extensions = 0U;
        
        /* Free the node */
        dsrtos_rr_free_node(scheduler, node);
        
        /* Update statistics */
        scheduler->stats.total_slices++;
        
        /* Adjust dynamic slice if enabled */
        if (scheduler->dynamic_slice) {
            rr_adjust_dynamic_slice(scheduler);
        }
    }
    
    RR_EXIT_CRITICAL(scheduler);
    
    /* Performance measurement end */
    end_cycles = dsrtos_port_get_cycle_count();
    schedule_time_us = dsrtos_port_cycles_to_us(end_cycles - start_cycles);
    dsrtos_rr_update_performance_metrics(scheduler, schedule_time_us);
    
    /* Verify performance requirement */
    if (schedule_time_us > RR_MAX_SCHEDULE_TIME_US) {
        /* Log performance violation */
        scheduler->base.stats.errors++;
    }
    
    return next_task;
}

/**
 * @brief Enqueue task to ready queue
 * Performance critical: Must complete in < 2μs
 */
dsrtos_status_t dsrtos_rr_enqueue(dsrtos_rr_scheduler_t* scheduler,
                                  dsrtos_tcb_t* task)
{
    dsrtos_rr_node_t* node;
    uint32_t start_cycles;
    uint32_t enqueue_time_us;
    
    RR_ASSERT_VALID_SCHEDULER(scheduler);
    RR_ASSERT_VALID_TASK(task);
    
    start_cycles = dsrtos_port_get_cycle_count();
    
    RR_ENTER_CRITICAL(scheduler);
    
    /* Allocate node from pool */
    node = dsrtos_rr_alloc_node(scheduler);
    if (node == NULL) {
        RR_EXIT_CRITICAL(scheduler);
        return DSRTOS_NO_MEMORY;
    }
    
    /* Initialize node */
    node->task = task;
    node->enqueue_time = dsrtos_get_tick_count();
    node->accumulated_wait = 0U;
    node->boost_level = 0U;
    
    /* Add to queue tail */
    rr_queue_push_back(&scheduler->ready_queue, node);
    
    /* Update statistics */
    scheduler->ready_queue.enqueue_count++;
    if (scheduler->ready_queue.count > scheduler->stats.max_queue_length) {
        scheduler->stats.max_queue_length = scheduler->ready_queue.count;
    }
    
    RR_EXIT_CRITICAL(scheduler);
    
    /* Verify performance requirement */
    enqueue_time_us = dsrtos_port_cycles_to_us(
        dsrtos_port_get_cycle_count() - start_cycles);
    if (enqueue_time_us > RR_MAX_ENQUEUE_TIME_US) {
        scheduler->base.stats.errors++;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Handle timer tick
 */
void dsrtos_rr_tick_update(dsrtos_rr_scheduler_t* scheduler)
{
    if ((scheduler == NULL) || 
        (scheduler->base.state != SCHEDULER_STATE_RUNNING)) {
        return;
    }
    
    RR_ENTER_CRITICAL(scheduler);
    
    /* Decrement time slice */
    if (scheduler->slice_remaining > 0U) {
        scheduler->slice_remaining--;
        
        /* Check if time slice expired */
        if (scheduler->slice_remaining == 0U) {
            scheduler->stats.total_preemptions++;
            /* Trigger scheduler to select next task */
            scheduler->base.stats.switches++;
        }
    }
    
    /* Update wait times for queued tasks */
    dsrtos_rr_node_t* node = scheduler->ready_queue.head;
    while (node != NULL) {
        node->accumulated_wait++;
        node = node->next;
    }
    
    RR_EXIT_CRITICAL(scheduler);
}

/* ============================================================================
 * STATIC HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Plugin init wrapper
 */
static dsrtos_status_t rr_plugin_init(void* scheduler)
{
    dsrtos_rr_scheduler_t* rr = (dsrtos_rr_scheduler_t*)scheduler;
    return dsrtos_rr_start(rr);
}

/**
 * @brief Plugin schedule wrapper
 */
static dsrtos_status_t rr_plugin_schedule(void* scheduler, dsrtos_tcb_t** next_task)
{
    dsrtos_rr_scheduler_t* rr = (dsrtos_rr_scheduler_t*)scheduler;
    
    if (next_task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    *next_task = dsrtos_rr_select_next(rr);
    return (*next_task != NULL) ? DSRTOS_SUCCESS : DSRTOS_NOT_FOUND;
}

/**
 * @brief Plugin add task wrapper
 */
static dsrtos_status_t rr_plugin_add_task(void* scheduler, dsrtos_tcb_t* task)
{
    dsrtos_rr_scheduler_t* rr = (dsrtos_rr_scheduler_t*)scheduler;
    return dsrtos_rr_enqueue(rr, task);
}

/**
 * @brief Plugin remove task wrapper
 */
static dsrtos_status_t rr_plugin_remove_task(void* scheduler, dsrtos_tcb_t* task)
{
    dsrtos_rr_scheduler_t* rr = (dsrtos_rr_scheduler_t*)scheduler;
    return dsrtos_rr_remove(rr, task);
}

/**
 * @brief Plugin tick wrapper
 */
static dsrtos_status_t rr_plugin_tick(void* scheduler)
{
    dsrtos_rr_scheduler_t* rr = (dsrtos_rr_scheduler_t*)scheduler;
    dsrtos_rr_tick_update(rr);
    return DSRTOS_SUCCESS;
}

/**
 * @brief Find first free node in pool
 */
static uint32_t rr_find_first_free_node(dsrtos_rr_scheduler_t* scheduler)
{
    uint32_t i, j;
    uint32_t bitmap_word;
    
    for (i = 0U; i < 8U; i++) {
        bitmap_word = scheduler->node_pool_bitmap[i];
        if (bitmap_word != 0xFFFFFFFFU) {
            /* Find first zero bit */
            for (j = 0U; j < 32U; j++) {
                if ((bitmap_word & (1U << j)) == 0U) {
                    return (i * 32U) + j;
                }
            }
        }
    }
    
    return RR_MAX_READY_TASKS;  /* No free nodes */
}

/**
 * @brief Mark node as used in bitmap
 */
static void rr_mark_node_used(dsrtos_rr_scheduler_t* scheduler, uint32_t index)
{
    uint32_t word = index / 32U;
    uint32_t bit = index % 32U;
    
    if (word < 8U) {
        scheduler->node_pool_bitmap[word] |= (1U << bit);
    }
}

/**
 * @brief Mark node as free in bitmap
 */
static void rr_mark_node_free(dsrtos_rr_scheduler_t* scheduler, uint32_t index)
{
    uint32_t word = index / 32U;
    uint32_t bit = index % 32U;
    
    if (word < 8U) {
        scheduler->node_pool_bitmap[word] &= ~(1U << bit);
    }
}

/**
 * @brief Allocate node from pool
 */
dsrtos_rr_node_t* dsrtos_rr_alloc_node(dsrtos_rr_scheduler_t* scheduler)
{
    uint32_t index;
    
    if (scheduler == NULL) {
        return NULL;
    }
    
    index = rr_find_first_free_node(scheduler);
    if (index >= RR_MAX_READY_TASKS) {
        return NULL;
    }
    
    rr_mark_node_used(scheduler, index);
    
    /* Clear node */
    (void)memset(&scheduler->node_pool[index], 0, sizeof(dsrtos_rr_node_t));
    
    return &scheduler->node_pool[index];
}

/**
 * @brief Free node back to pool
 */
void dsrtos_rr_free_node(dsrtos_rr_scheduler_t* scheduler,
                        dsrtos_rr_node_t* node)
{
    uint32_t index;
    
    if ((scheduler == NULL) || (node == NULL)) {
        return;
    }
    
    /* Calculate index */
    index = (uint32_t)(node - scheduler->node_pool);
    if (index < RR_MAX_READY_TASKS) {
        rr_mark_node_free(scheduler, index);
    }
}

/**
 * @brief Push node to back of queue
 */
static void rr_queue_push_back(dsrtos_rr_queue_t* queue, dsrtos_rr_node_t* node)
{
    if ((queue == NULL) || (node == NULL)) {
        return;
    }
    
    node->next = NULL;
    node->prev = queue->tail;
    
    if (queue->tail != NULL) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    
    queue->tail = node;
    queue->count++;
}

/**
 * @brief Pop node from front of queue
 */
static dsrtos_rr_node_t* rr_queue_pop_front(dsrtos_rr_queue_t* queue)
{
    dsrtos_rr_node_t* node;
    
    if ((queue == NULL) || (queue->head == NULL)) {
        return NULL;
    }
    
    node = queue->head;
    queue->head = node->next;
    
    if (queue->head != NULL) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }
    
    queue->count--;
    queue->dequeue_count++;
    
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * @brief Remove specific node from queue
 */
static void rr_queue_remove_node(dsrtos_rr_queue_t* queue, dsrtos_rr_node_t* node)
{
    if ((queue == NULL) || (node == NULL)) {
        return;
    }
    
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        queue->head = node->next;
    }
    
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        queue->tail = node->prev;
    }
    
    queue->count--;
    node->next = NULL;
    node->prev = NULL;
}

/**
 * @brief Verify queue integrity
 */
bool dsrtos_rr_queue_verify_integrity(const dsrtos_rr_queue_t* queue)
{
    uint32_t count = 0U;
    dsrtos_rr_node_t* node;
    
    if (queue == NULL) {
        return false;
    }
    
    /* Check integrity marker */
    if (queue->integrity_check != RR_QUEUE_INTEGRITY_CHECK) {
        return false;
    }
    
    /* Count nodes and verify linkage */
    node = queue->head;
    while (node != NULL) {
        count++;
        if (count > RR_MAX_READY_TASKS) {
            return false;  /* Circular reference detected */
        }
        node = node->next;
    }
    
    /* Verify count matches */
    return (count == queue->count);
}

/**
 * @brief Adjust time slice dynamically based on queue length
 */
static void rr_adjust_dynamic_slice(dsrtos_rr_scheduler_t* scheduler)
{
    uint32_t new_slice;
    uint32_t queue_length;
    
    if ((scheduler == NULL) || (!scheduler->dynamic_slice)) {
        return;
    }
    
    queue_length = scheduler->ready_queue.count;
    
    /* Adjust slice based on queue length */
    if (queue_length > 10U) {
        /* Many tasks: shorter slices for better responsiveness */
        new_slice = scheduler->min_slice_ms;
    } else if (queue_length > 5U) {
        /* Moderate tasks: medium slices */
        new_slice = (scheduler->min_slice_ms + scheduler->max_slice_ms) / 2U;
    } else {
        /* Few tasks: longer slices for better throughput */
        new_slice = scheduler->max_slice_ms;
    }
    
    scheduler->time_slice_ms = new_slice;
}

/**
 * @brief Check and handle task starvation
 */
static void rr_check_and_handle_starvation(dsrtos_rr_scheduler_t* scheduler)
{
    dsrtos_rr_node_t* node;
    dsrtos_rr_node_t* next;
    uint32_t current_time;
    
    if (scheduler == NULL) {
        return;
    }
    
    current_time = dsrtos_get_tick_count();
    node = scheduler->ready_queue.head;
    
    while (node != NULL) {
        next = node->next;
        
        /* Check if task is starving */
        if (node->accumulated_wait > scheduler->starvation_threshold) {
            /* Move to front of queue (boost) */
            rr_queue_remove_node(&scheduler->ready_queue, node);
            
            /* Insert at front */
            node->next = scheduler->ready_queue.head;
            node->prev = NULL;
            if (scheduler->ready_queue.head != NULL) {
                scheduler->ready_queue.head->prev = node;
            }
            scheduler->ready_queue.head = node;
            if (scheduler->ready_queue.tail == NULL) {
                scheduler->ready_queue.tail = node;
            }
            
            /* Reset wait time and increment boost */
            node->accumulated_wait = 0U;
            node->boost_level++;
            
            scheduler->stats.starvation_count++;
            scheduler->stats.boost_count++;
        }
        
        node = next;
    }
}

/**
 * @brief Update performance metrics
 */
void dsrtos_rr_update_performance_metrics(dsrtos_rr_scheduler_t* scheduler,
                                          uint32_t schedule_time_us)
{
    if (scheduler == NULL) {
        return;
    }
    
    /* Update min/max */
    if (schedule_time_us < scheduler->stats.min_schedule_time_us) {
        scheduler->stats.min_schedule_time_us = schedule_time_us;
    }
    if (schedule_time_us > scheduler->stats.max_schedule_time_us) {
        scheduler->stats.max_schedule_time_us = schedule_time_us;
    }
    
    /* Update average (exponential moving average) */
    scheduler->stats.avg_schedule_time_us = 
        (scheduler->stats.avg_schedule_time_us * 7U + schedule_time_us) / 8U;
}

/**
 * @brief Remove task from ready queue
 */
dsrtos_status_t dsrtos_rr_remove(dsrtos_rr_scheduler_t* scheduler,
                                 dsrtos_tcb_t* task)
{
    dsrtos_rr_node_t* node;
    
    RR_ASSERT_VALID_SCHEDULER(scheduler);
    RR_ASSERT_VALID_TASK(task);
    
    RR_ENTER_CRITICAL(scheduler);
    
    /* Find task in queue */
    node = scheduler->ready_queue.head;
    while (node != NULL) {
        if (node->task == task) {
            rr_queue_remove_node(&scheduler->ready_queue, node);
            dsrtos_rr_free_node(scheduler, node);
            RR_EXIT_CRITICAL(scheduler);
            return DSRTOS_SUCCESS;
        }
        node = node->next;
    }
    
    RR_EXIT_CRITICAL(scheduler);
    return DSRTOS_NOT_FOUND;
}

/**
 * @brief Get scheduler statistics
 */
dsrtos_status_t dsrtos_rr_get_stats(const dsrtos_rr_scheduler_t* scheduler,
                                    dsrtos_rr_stats_t* stats)
{
    if ((scheduler == NULL) || (stats == NULL)) {
        return DSRTOS_INVALID_PARAM;
    }
    
    *stats = scheduler->stats;
    return DSRTOS_SUCCESS;
}
