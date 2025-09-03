/*
 * DSRTOS Ready Queue Operations Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_ready_queue_ops.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_trace.h"
#include <string.h>

/* Global operations statistics */
static dsrtos_queue_ops_stats_t g_queue_ops_stats = {0};

/**
 * @brief Initialize queue iterator
 */
dsrtos_error_t dsrtos_queue_iterator_init(
    dsrtos_queue_iterator_t* iter,
    dsrtos_ready_queue_t* queue)
{
    if ((iter == NULL) || (queue == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate queue */
    if (!dsrtos_ready_queue_validate(queue)) {
        return DSRTOS_ERROR_CORRUPTED;
    }
    
    (void)memset(iter, 0, sizeof(dsrtos_queue_iterator_t));
    
    /* Find highest priority with tasks */
    iter->current_priority = dsrtos_priority_bitmap_get_highest(queue->priority_bitmap);
    if (iter->current_priority > 0U) {
        iter->current_priority--;  /* Convert to 0-based index */
        iter->current_node = queue->priority_lists[iter->current_priority].head;
        iter->remaining_tasks = queue->stats.total_tasks;
        iter->iteration_count = 0U;
    }
    
    g_queue_ops_stats.total_iterations++;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get next task from iterator
 */
dsrtos_tcb_t* dsrtos_queue_iterator_next(dsrtos_queue_iterator_t* iter)
{
    dsrtos_queue_node_t* node;
    dsrtos_tcb_t* task = NULL;
    uint32_t start_cycles;
    
    if ((iter == NULL) || (iter->remaining_tasks == 0U)) {
        return NULL;
    }
    
    start_cycles = DWT->CYCCNT;
    
    node = (dsrtos_queue_node_t*)iter->current_node;
    if (node != NULL) {
        /* Validate node */
        if ((node->magic_start == DSRTOS_NODE_MAGIC) &&
            (node->magic_end == DSRTOS_NODE_MAGIC)) {
            task = node->task;
            
            /* Move to next node */
            iter->current_node = node->next;
            
            /* If end of priority list, find next priority */
            if ((iter->current_node == NULL) && (iter->current_priority > 0U)) {
                while (--iter->current_priority > 0U) {
                    if (dsrtos_priority_bitmap_is_set(NULL, (uint8_t)iter->current_priority)) {
                        /* Found next priority with tasks */
                        break;
                    }
                }
            }
        }
        
        iter->remaining_tasks--;
        iter->iteration_count++;
    }
    
    /* Update statistics */
    uint32_t elapsed = DWT->CYCCNT - start_cycles;
    if (elapsed > g_queue_ops_stats.max_iteration_time) {
        g_queue_ops_stats.max_iteration_time = elapsed;
    }
    
    return task;
}

/**
 * @brief Rebalance ready queue
 */
dsrtos_error_t dsrtos_ready_queue_rebalance(dsrtos_ready_queue_t* queue)
{
    uint32_t i;
    dsrtos_priority_list_t* list;
    dsrtos_queue_node_t* node;
    dsrtos_queue_node_t* next;
    uint32_t removed_count = 0U;
    
    if (queue == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    DSRTOS_TRACE_QUEUE("Rebalancing ready queue");
    
    dsrtos_ready_queue_lock(queue);
    
    /* Process each priority list */
    for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
        list = &queue->priority_lists[i];
        node = list->head;
        
        /* Remove invalid nodes */
        while (node != NULL) {
            next = node->next;
            
            /* Check node validity */
            if ((node->magic_start != DSRTOS_NODE_MAGIC) ||
                (node->magic_end != DSRTOS_NODE_MAGIC) ||
                (node->task == NULL)) {
                
                /* Remove invalid node */
                if (node->prev != NULL) {
                    node->prev->next = node->next;
                } else {
                    list->head = node->next;
                }
                
                if (node->next != NULL) {
                    node->next->prev = node->prev;
                } else {
                    list->tail = node->prev;
                }
                
                list->count--;
                removed_count++;
            }
            
            node = next;
        }
        
        /* Update bitmap if list became empty */
        if ((list->count == 0U) && (list->head == NULL)) {
            dsrtos_priority_bitmap_clear(queue->priority_bitmap, (uint8_t)i);
            dsrtos_priority_bitmap_clear(queue->priority_bitmap_mirror, (uint8_t)i);
        }
    }
    
    /* Update statistics */
    if (queue->stats.total_tasks >= removed_count) {
        queue->stats.total_tasks -= removed_count;
    }
    g_queue_ops_stats.rebalance_count++;
    
    dsrtos_ready_queue_unlock(queue);
    
    DSRTOS_TRACE_QUEUE("Rebalance complete, removed %u invalid nodes", removed_count);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate task between queues (SMP support)
 */
dsrtos_error_t dsrtos_ready_queue_migrate(
    dsrtos_ready_queue_t* src_queue,
    dsrtos_ready_queue_t* dst_queue,
    dsrtos_tcb_t* task)
{
    dsrtos_error_t result;
    
    if ((src_queue == NULL) || (dst_queue == NULL) || (task == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    DSRTOS_TRACE_QUEUE("Migrating task %u between queues", task->task_id);
    
    /* Remove from source queue */
    result = dsrtos_ready_queue_remove(src_queue, task);
    if (result != DSRTOS_SUCCESS) {
        return result;
    }
    
    /* Insert into destination queue */
    result = dsrtos_ready_queue_insert(dst_queue, task);
    if (result != DSRTOS_SUCCESS) {
        /* Try to restore to source queue */
        (void)dsrtos_ready_queue_insert(src_queue, task);
        return result;
    }
    
    /* Update statistics */
    task->migrations++;
    g_queue_ops_stats.migration_count++;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Clear entire ready queue
 */
dsrtos_error_t dsrtos_ready_queue_clear(dsrtos_ready_queue_t* queue)
{
    uint32_t i;
    
    if (queue == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    DSRTOS_TRACE_QUEUE("Clearing ready queue");
    
    dsrtos_ready_queue_lock(queue);
    
    /* Clear all priority lists */
    for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
        queue->priority_lists[i].head = NULL;
        queue->priority_lists[i].tail = NULL;
        queue->priority_lists[i].count = 0U;
        queue->priority_lists[i].list_checksum = 0U;
    }
    
    /* Clear bitmaps */
    (void)memset((void*)queue->priority_bitmap, 0, sizeof(queue->priority_bitmap));
    (void)memset((void*)queue->priority_bitmap_mirror, 0, sizeof(queue->priority_bitmap_mirror));
    
    /* Reset statistics */
    queue->stats.total_tasks = 0U;
    queue->stats.highest_priority = 0U;
    
    dsrtos_ready_queue_unlock(queue);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get queue operations statistics
 */
dsrtos_error_t dsrtos_ready_queue_get_stats(
    dsrtos_ready_queue_t* queue,
    dsrtos_queue_ops_stats_t* stats)
{
    if ((queue == NULL) || (stats == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    *stats = g_queue_ops_stats;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Check if queue contains task
 */
bool dsrtos_ready_queue_contains(
    dsrtos_ready_queue_t* queue,
    dsrtos_tcb_t* task)
{
    dsrtos_priority_list_t* list;
    dsrtos_queue_node_t* node;
    bool found = false;
    
    if ((queue == NULL) || (task == NULL)) {
        return false;
    }
    
    dsrtos_ready_queue_lock(queue);
    
    /* Check the task's priority list */
    list = &queue->priority_lists[task->effective_priority];
    node = list->head;
    
    while (node != NULL) {
        if (node->task == task) {
            found = true;
            break;
        }
        node = node->next;
    }
    
    dsrtos_ready_queue_unlock(queue);
    
    return found;
}

/**
 * @brief Count tasks at specific priority
 */
uint32_t dsrtos_ready_queue_count_at_priority(
    dsrtos_ready_queue_t* queue,
    uint8_t priority)
{
    uint32_t count = 0U;
    
    if ((queue != NULL) && (priority < DSRTOS_PRIORITY_LEVELS)) {
        dsrtos_ready_queue_lock(queue);
        count = queue->priority_lists[priority].count;
        dsrtos_ready_queue_unlock(queue);
    }
    
    return count;
}
