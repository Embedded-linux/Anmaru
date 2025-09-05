/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Copyright (C) 2024 DSRTOS Development Team
 * 
 * File: dsrtos_scheduler_priority.c
 * Description: Static Priority Scheduler with O(1) operations
 * Phase: 6 - Concrete Scheduler Implementations
 * 
 * Performance Requirements:
 * - Schedule decision: < 3μs
 * - Priority set: < 1μs
 * - Enqueue operation: < 2μs
 */

#include "dsrtos_scheduler_priority.h"
#include "dsrtos_kernel.h"
#include "dsrtos_port.h"
#include <string.h>

/* ============================================================================
 * STATIC FUNCTION PROTOTYPES
 * ============================================================================ */

static dsrtos_status_t prio_plugin_init(void* scheduler);
static dsrtos_status_t prio_plugin_schedule(void* scheduler, dsrtos_tcb_t** next_task);
static dsrtos_status_t prio_plugin_add_task(void* scheduler, dsrtos_tcb_t* task);
static dsrtos_status_t prio_plugin_remove_task(void* scheduler, dsrtos_tcb_t* task);
static dsrtos_status_t prio_plugin_tick(void* scheduler);
static uint32_t prio_find_free_node(dsrtos_priority_scheduler_t* scheduler);
static void prio_mark_node_used(dsrtos_priority_scheduler_t* scheduler, uint32_t index);
static void prio_mark_node_free(dsrtos_priority_scheduler_t* scheduler, uint32_t index);
static void prio_update_bitmap(dsrtos_priority_scheduler_t* scheduler, uint8_t priority);
static uint8_t prio_find_highest_priority(dsrtos_priority_scheduler_t* scheduler);
static dsrtos_pi_record_t* prio_alloc_pi_record(dsrtos_priority_scheduler_t* scheduler);
static void prio_free_pi_record(dsrtos_priority_scheduler_t* scheduler, 
                               dsrtos_pi_record_t* record);
static void prio_apply_aging(dsrtos_priority_scheduler_t* scheduler);

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief Initialize priority scheduler
 * MISRA-C:2012 Rule 8.13: scheduler parameter cannot be const
 */
dsrtos_status_t dsrtos_priority_init(dsrtos_priority_scheduler_t* scheduler)
{
    uint32_t i;
    
    /* Validate parameters */
    if (scheduler == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    /* Clear scheduler structure */
    (void)memset(scheduler, 0, sizeof(dsrtos_priority_scheduler_t));
    
    /* Initialize base plugin interface */
    scheduler->base.magic = SCHEDULER_MAGIC;
    scheduler->base.scheduler_id = SCHEDULER_ID_PRIORITY;
    scheduler->base.priority = 255U;  /* Highest plugin priority */
    scheduler->base.state = SCHEDULER_STATE_INACTIVE;
    (void)strncpy(scheduler->base.name, "Priority", SCHEDULER_NAME_MAX - 1U);
    
    /* Set plugin operations */
    scheduler->base.ops.init = prio_plugin_init;
    scheduler->base.ops.schedule = prio_plugin_schedule;
    scheduler->base.ops.add_task = prio_plugin_add_task;
    scheduler->base.ops.remove_task = prio_plugin_remove_task;
    scheduler->base.ops.tick = prio_plugin_tick;
    scheduler->base.ops.get_stats = NULL;  /* Use priority-specific stats */
    scheduler->base.ops.reset = (dsrtos_status_t (*)(void*))dsrtos_priority_reset;
    
    /* Initialize priority configuration */
    scheduler->num_priorities = PRIO_NUM_LEVELS;
    scheduler->highest_priority = PRIO_HIGHEST;
    scheduler->lowest_priority = PRIO_LOWEST;
    
    /* Initialize ready queues */
    for (i = 0U; i < PRIO_NUM_LEVELS; i++) {
        dsrtos_priority_queue_init(&scheduler->ready_queues[i]);
    }
    
    /* Initialize priority bitmap */
    for (i = 0U; i < PRIO_BITMAP_WORDS; i++) {
        scheduler->priority_map.bitmap[i] = 0U;
    }
    scheduler->priority_map.highest_set = PRIO_LOWEST;
    scheduler->priority_map.cache_valid = false;
    scheduler->priority_map.last_update = 0U;
    
    /* Initialize node pool */
    for (i = 0U; i < 8U; i++) {
        scheduler->node_bitmap[i] = 0U;  /* All nodes free */
    }
    
    /* Initialize priority inheritance */
    scheduler->inheritance.enabled = PRIO_INHERITANCE_ENABLED;
    scheduler->inheritance.max_depth = PRIO_MAX_INHERITANCE_DEPTH;
    scheduler->inheritance.active_count = 0U;
    scheduler->inheritance.table_bitmap = 0U;  /* All records free */
    
    /* Initialize priority aging */
    scheduler->aging.enabled = PRIO_AGING_ENABLED;
    scheduler->aging.period_ms = PRIO_AGING_PERIOD_MS;
    scheduler->aging.threshold_ms = PRIO_AGE_THRESHOLD_MS;
    scheduler->aging.boost_amount = PRIO_AGE_BOOST;
    scheduler->aging.last_aging_time = 0U;
    
    /* Initialize current state */
    scheduler->current_task = NULL;
    scheduler->current_priority = PRIO_LOWEST;
    
    /* Initialize statistics */
    scheduler->stats.min_schedule_time_us = UINT32_MAX;
    scheduler->stats.max_schedule_time_us = 0U;
    scheduler->stats.avg_schedule_time_us = 0U;
    
    /* Initialize synchronization */
    scheduler->lock = 0U;
    scheduler->critical_count = 0U;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Start priority scheduler
 */
dsrtos_status_t dsrtos_priority_start(dsrtos_priority_scheduler_t* scheduler)
{
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (scheduler->base.state == SCHEDULER_STATE_RUNNING) {
        return DSRTOS_ALREADY_EXISTS;
    }
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    scheduler->base.state = SCHEDULER_STATE_RUNNING;
    scheduler->aging.last_aging_time = dsrtos_get_tick_count();
    
    PRIO_EXIT_CRITICAL(scheduler);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Stop priority scheduler
 */
dsrtos_status_t dsrtos_priority_stop(dsrtos_priority_scheduler_t* scheduler)
{
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (scheduler->base.state != SCHEDULER_STATE_RUNNING) {
        return DSRTOS_INVALID_STATE;
    }
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    scheduler->base.state = SCHEDULER_STATE_SUSPENDED;
    scheduler->current_task = NULL;
    
    PRIO_EXIT_CRITICAL(scheduler);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Select next task to run - O(1) operation
 * Performance critical: Must complete in < 3μs
 */
dsrtos_tcb_t* dsrtos_priority_select_next(dsrtos_priority_scheduler_t* scheduler)
{
    dsrtos_priority_node_t* node;
    dsrtos_tcb_t* next_task = NULL;
    uint8_t highest_priority;
    uint32_t start_cycles;
    uint32_t schedule_time_us;
    
    if ((scheduler == NULL) || 
        (scheduler->base.state != SCHEDULER_STATE_RUNNING)) {
        return NULL;
    }
    
    /* Performance measurement start */
    start_cycles = dsrtos_port_get_cycle_count();
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Find highest priority with ready tasks - O(1) */
    highest_priority = prio_find_highest_priority(scheduler);
    
    if (highest_priority < PRIO_NUM_LEVELS) {
        /* Get task from highest priority queue */
        node = dsrtos_priority_queue_pop(&scheduler->ready_queues[highest_priority]);
        
        if (node != NULL) {
            next_task = node->task;
            scheduler->current_task = next_task;
            scheduler->current_priority = highest_priority;
            
            /* Update bitmap if queue is now empty */
            if (scheduler->ready_queues[highest_priority].count == 0U) {
                PRIO_BITMAP_CLEAR(scheduler, highest_priority);
            }
            
            /* Free the node */
            dsrtos_priority_free_node(scheduler, node);
            
            /* Update statistics */
            scheduler->stats.total_schedules++;
            scheduler->stats.priority_switches[highest_priority]++;
        }
    }
    
    /* Check for aging if enabled */
    if (scheduler->aging.enabled) {
        uint32_t current_time = dsrtos_get_tick_count();
        if ((current_time - scheduler->aging.last_aging_time) >= 
            scheduler->aging.period_ms) {
            prio_apply_aging(scheduler);
            scheduler->aging.last_aging_time = current_time;
        }
    }
    
    PRIO_EXIT_CRITICAL(scheduler);
    
    /* Performance measurement */
    schedule_time_us = dsrtos_port_cycles_to_us(
        dsrtos_port_get_cycle_count() - start_cycles);
    dsrtos_priority_update_metrics(scheduler, schedule_time_us);
    
    /* Verify performance requirement */
    if (schedule_time_us > PRIO_MAX_SCHEDULE_TIME_US) {
        scheduler->base.stats.errors++;
    }
    
    return next_task;
}

/**
 * @brief Enqueue task at specified priority
 * Performance critical: Must complete in < 2μs
 */
dsrtos_status_t dsrtos_priority_enqueue(dsrtos_priority_scheduler_t* scheduler,
                                        dsrtos_tcb_t* task,
                                        uint8_t priority)
{
    dsrtos_priority_node_t* node;
    uint32_t start_cycles;
    uint32_t enqueue_time_us;
    
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    PRIO_ASSERT_VALID_PRIORITY(priority);
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    start_cycles = dsrtos_port_get_cycle_count();
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Allocate node from pool */
    node = dsrtos_priority_alloc_node(scheduler);
    if (node == NULL) {
        PRIO_EXIT_CRITICAL(scheduler);
        return DSRTOS_NO_MEMORY;
    }
    
    /* Initialize node */
    node->task = task;
    node->effective_priority = priority;
    node->base_priority = priority;
    node->enqueue_time = dsrtos_get_tick_count();
    node->age_counter = 0U;
    
    /* Add to appropriate priority queue */
    dsrtos_priority_queue_push(&scheduler->ready_queues[priority], node);
    
    /* Update bitmap - O(1) */
    PRIO_BITMAP_SET(scheduler, priority);
    
    /* Update statistics */
    scheduler->stats.priority_distribution[priority]++;
    
    PRIO_EXIT_CRITICAL(scheduler);
    
    /* Performance measurement */
    enqueue_time_us = dsrtos_port_cycles_to_us(
        dsrtos_port_get_cycle_count() - start_cycles);
    
    if (enqueue_time_us > PRIO_MAX_ENQUEUE_TIME_US) {
        scheduler->base.stats.errors++;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Set task priority dynamically
 * Performance critical: Must complete in < 1μs
 */
dsrtos_status_t dsrtos_priority_set(dsrtos_priority_scheduler_t* scheduler,
                                    dsrtos_tcb_t* task,
                                    uint8_t new_priority)
{
    dsrtos_priority_node_t* node;
    uint8_t old_priority;
    uint32_t start_cycles;
    uint32_t set_time_us;
    uint32_t i;
    
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    PRIO_ASSERT_VALID_PRIORITY(new_priority);
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    start_cycles = dsrtos_port_get_cycle_count();
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Find task in queues */
    for (i = 0U; i < PRIO_NUM_LEVELS; i++) {
        node = scheduler->ready_queues[i].head;
        while (node != NULL) {
            if (node->task == task) {
                old_priority = (uint8_t)i;
                
                /* Remove from old queue */
                dsrtos_priority_queue_remove(&scheduler->ready_queues[old_priority], node);
                if (scheduler->ready_queues[old_priority].count == 0U) {
                    PRIO_BITMAP_CLEAR(scheduler, old_priority);
                }
                
                /* Update priority */
                node->effective_priority = new_priority;
                node->age_counter = 0U;  /* Reset aging */
                
                /* Add to new queue */
                dsrtos_priority_queue_push(&scheduler->ready_queues[new_priority], node);
                PRIO_BITMAP_SET(scheduler, new_priority);
                
                /* Update statistics */
                scheduler->stats.priority_changes++;
                scheduler->stats.priority_distribution[old_priority]--;
                scheduler->stats.priority_distribution[new_priority]++;
                
                PRIO_EXIT_CRITICAL(scheduler);
                
                /* Performance measurement */
                set_time_us = dsrtos_port_cycles_to_us(
                    dsrtos_port_get_cycle_count() - start_cycles);
                
                if (set_time_us > PRIO_MAX_PRIORITY_SET_US) {
                    scheduler->base.stats.errors++;
                }
                
                return DSRTOS_SUCCESS;
            }
            node = node->next;
        }
    }
    
    PRIO_EXIT_CRITICAL(scheduler);
    return DSRTOS_NOT_FOUND;
}

/**
 * @brief Priority inheritance - elevate task priority
 */
dsrtos_status_t dsrtos_priority_inherit(dsrtos_priority_scheduler_t* scheduler,
                                        dsrtos_tcb_t* task,
                                        uint8_t inherited_priority,
                                        void* resource)
{
    dsrtos_pi_record_t* record;
    uint8_t current_priority;
    
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    PRIO_ASSERT_VALID_PRIORITY(inherited_priority);
    
    if ((task == NULL) || (!scheduler->inheritance.enabled)) {
        return DSRTOS_INVALID_PARAM;
    }
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Check inheritance depth */
    if (scheduler->inheritance.active_count >= scheduler->inheritance.max_depth) {
        PRIO_EXIT_CRITICAL(scheduler);
        return DSRTOS_LIMIT_EXCEEDED;
    }
    
    /* Get current task priority */
    current_priority = dsrtos_priority_get(scheduler, task);
    
    /* Only inherit if new priority is higher (lower value) */
    if (inherited_priority < current_priority) {
        /* Allocate inheritance record */
        record = prio_alloc_pi_record(scheduler);
        if (record == NULL) {
            PRIO_EXIT_CRITICAL(scheduler);
            return DSRTOS_NO_MEMORY;
        }
        
        /* Record inheritance */
        record->task = task;
        record->original_priority = current_priority;
        record->inherited_priority = inherited_priority;
        record->inheritance_count = 1U;
        record->timestamp = dsrtos_get_tick_count();
        record->blocking_resource = resource;
        
        /* Apply new priority */
        (void)dsrtos_priority_set(scheduler, task, inherited_priority);
        
        /* Update statistics */
        scheduler->inheritance.active_count++;
        scheduler->stats.inheritance_activations++;
    }
    
    PRIO_EXIT_CRITICAL(scheduler);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Remove priority inheritance
 */
dsrtos_status_t dsrtos_priority_uninherit(dsrtos_priority_scheduler_t* scheduler,
                                          dsrtos_tcb_t* task,
                                          void* resource)
{
    dsrtos_pi_record_t* record;
    uint32_t i;
    
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Find inheritance record */
    for (i = 0U; i < PRIO_INHERITANCE_TABLE_SIZE; i++) {
        record = &scheduler->inheritance.table[i];
        if ((record->task == task) && 
            (record->blocking_resource == resource)) {
            
            /* Restore original priority */
            (void)dsrtos_priority_set(scheduler, task, record->original_priority);
            
            /* Free record */
            prio_free_pi_record(scheduler, record);
            scheduler->inheritance.active_count--;
            
            PRIO_EXIT_CRITICAL(scheduler);
            return DSRTOS_SUCCESS;
        }
    }
    
    PRIO_EXIT_CRITICAL(scheduler);
    return DSRTOS_NOT_FOUND;
}

/* ============================================================================
 * STATIC HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Find highest priority level with ready tasks - O(1)
 */
static uint8_t prio_find_highest_priority(dsrtos_priority_scheduler_t* scheduler)
{
    /* Use cached value if valid */
    if (scheduler->priority_map.cache_valid) {
        return scheduler->priority_map.highest_set;
    }
    
    /* Find first set bit in bitmap */
    scheduler->priority_map.highest_set = prio_bitmap_ffs(scheduler->priority_map.bitmap);
    scheduler->priority_map.cache_valid = true;
    scheduler->stats.bitmap_scans++;
    
    return scheduler->priority_map.highest_set;
}

/**
 * @brief Initialize priority queue
 */
void dsrtos_priority_queue_init(dsrtos_priority_queue_t* queue)
{
    if (queue != NULL) {
        queue->head = NULL;
        queue->tail = NULL;
        queue->count = 0U;
    }
}

/**
 * @brief Push node to priority queue
 */
void dsrtos_priority_queue_push(dsrtos_priority_queue_t* queue,
                                dsrtos_priority_node_t* node)
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
 * @brief Pop node from priority queue
 */
dsrtos_priority_node_t* dsrtos_priority_queue_pop(dsrtos_priority_queue_t* queue)
{
    dsrtos_priority_node_t* node;
    
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
    
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * @brief Remove specific node from queue
 */
void dsrtos_priority_queue_remove(dsrtos_priority_queue_t* queue,
                                  dsrtos_priority_node_t* node)
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
 * @brief Allocate node from pool
 */
dsrtos_priority_node_t* dsrtos_priority_alloc_node(dsrtos_priority_scheduler_t* scheduler)
{
    uint32_t index = prio_find_free_node(scheduler);
    
    if (index >= 256U) {
        return NULL;
    }
    
    prio_mark_node_used(scheduler, index);
    (void)memset(&scheduler->node_pool[index], 0, sizeof(dsrtos_priority_node_t));
    
    return &scheduler->node_pool[index];
}

/**
 * @brief Free node back to pool
 */
void dsrtos_priority_free_node(dsrtos_priority_scheduler_t* scheduler,
                              dsrtos_priority_node_t* node)
{
    uint32_t index;
    
    if ((scheduler == NULL) || (node == NULL)) {
        return;
    }
    
    index = (uint32_t)(node - scheduler->node_pool);
    if (index < 256U) {
        prio_mark_node_free(scheduler, index);
    }
}

/**
 * @brief Find free node in pool
 */
static uint32_t prio_find_free_node(dsrtos_priority_scheduler_t* scheduler)
{
    uint32_t i, j;
    
    for (i = 0U; i < 8U; i++) {
        if (scheduler->node_bitmap[i] != 0xFFFFFFFFU) {
            for (j = 0U; j < 32U; j++) {
                if ((scheduler->node_bitmap[i] & (1U << j)) == 0U) {
                    return (i * 32U) + j;
                }
            }
        }
    }
    
    return 256U;  /* No free nodes */
}

/**
 * @brief Mark node as used
 */
static void prio_mark_node_used(dsrtos_priority_scheduler_t* scheduler, uint32_t index)
{
    uint32_t word = index / 32U;
    uint32_t bit = index % 32U;
    
    if (word < 8U) {
        scheduler->node_bitmap[word] |= (1U << bit);
    }
}

/**
 * @brief Mark node as free
 */
static void prio_mark_node_free(dsrtos_priority_scheduler_t* scheduler, uint32_t index)
{
    uint32_t word = index / 32U;
    uint32_t bit = index % 32U;
    
    if (word < 8U) {
        scheduler->node_bitmap[word] &= ~(1U << bit);
    }
}

/**
 * @brief Apply priority aging to prevent starvation
 */
static void prio_apply_aging(dsrtos_priority_scheduler_t* scheduler)
{
    dsrtos_priority_node_t* node;
    uint32_t i;
    uint32_t current_time;
    uint32_t wait_time;
    uint8_t new_priority;
    
    if (scheduler == NULL) {
        return;
    }
    
    current_time = dsrtos_get_tick_count();
    
    /* Check all priority levels except highest */
    for (i = 1U; i < PRIO_NUM_LEVELS; i++) {
        node = scheduler->ready_queues[i].head;
        
        while (node != NULL) {
            wait_time = current_time - node->enqueue_time;
            
            /* Check if task has waited too long */
            if (wait_time > scheduler->aging.threshold_ms) {
                /* Calculate new priority (boost) */
                if (node->effective_priority >= scheduler->aging.boost_amount) {
                    new_priority = node->effective_priority - scheduler->aging.boost_amount;
                } else {
                    new_priority = PRIO_HIGHEST;
                }
                
                /* Only boost if it improves priority */
                if (new_priority < node->effective_priority) {
                    /* Move to higher priority queue */
                    dsrtos_priority_queue_remove(&scheduler->ready_queues[i], node);
                    if (scheduler->ready_queues[i].count == 0U) {
                        PRIO_BITMAP_CLEAR(scheduler, (uint8_t)i);
                    }
                    
                    node->effective_priority = new_priority;
                    node->age_counter++;
                    
                    dsrtos_priority_queue_push(&scheduler->ready_queues[new_priority], node);
                    PRIO_BITMAP_SET(scheduler, new_priority);
                    
                    /* Update statistics */
                    scheduler->stats.aging_adjustments++;
                    scheduler->stats.aging_promotions++;
                    scheduler->stats.starvation_prevented++;
                }
            }
            
            node = node->next;
        }
    }
}

/**
 * @brief Plugin interface wrappers
 */
static dsrtos_status_t prio_plugin_init(void* scheduler)
{
    return dsrtos_priority_start((dsrtos_priority_scheduler_t*)scheduler);
}

static dsrtos_status_t prio_plugin_schedule(void* scheduler, dsrtos_tcb_t** next_task)
{
    dsrtos_priority_scheduler_t* prio = (dsrtos_priority_scheduler_t*)scheduler;
    
    if (next_task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    *next_task = dsrtos_priority_select_next(prio);
    return (*next_task != NULL) ? DSRTOS_SUCCESS : DSRTOS_NOT_FOUND;
}

static dsrtos_status_t prio_plugin_add_task(void* scheduler, dsrtos_tcb_t* task)
{
    dsrtos_priority_scheduler_t* prio = (dsrtos_priority_scheduler_t*)scheduler;
    /* Use task's priority from TCB */
    return dsrtos_priority_enqueue(prio, task, task->priority);
}

static dsrtos_status_t prio_plugin_remove_task(void* scheduler, dsrtos_tcb_t* task)
{
    return dsrtos_priority_remove((dsrtos_priority_scheduler_t*)scheduler, task);
}

static dsrtos_status_t prio_plugin_tick(void* scheduler)
{
    dsrtos_priority_scheduler_t* prio = (dsrtos_priority_scheduler_t*)scheduler;
    
    /* Aging check is performed in select_next for efficiency */
    (void)prio;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Allocate priority inheritance record
 */
static dsrtos_pi_record_t* prio_alloc_pi_record(dsrtos_priority_scheduler_t* scheduler)
{
    uint32_t i;
    
    for (i = 0U; i < PRIO_INHERITANCE_TABLE_SIZE; i++) {
        if ((scheduler->inheritance.table_bitmap & (1U << i)) == 0U) {
            scheduler->inheritance.table_bitmap |= (1U << i);
            return &scheduler->inheritance.table[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Free priority inheritance record
 */
static void prio_free_pi_record(dsrtos_priority_scheduler_t* scheduler,
                               dsrtos_pi_record_t* record)
{
    uint32_t index;
    
    if ((scheduler == NULL) || (record == NULL)) {
        return;
    }
    
    index = (uint32_t)(record - scheduler->inheritance.table);
    if (index < PRIO_INHERITANCE_TABLE_SIZE) {
        scheduler->inheritance.table_bitmap &= ~(1U << index);
        (void)memset(record, 0, sizeof(dsrtos_pi_record_t));
    }
}

/**
 * @brief Update performance metrics
 */
void dsrtos_priority_update_metrics(dsrtos_priority_scheduler_t* scheduler,
                                    uint32_t schedule_time_us)
{
    if (scheduler == NULL) {
        return;
    }
    
    if (schedule_time_us < scheduler->stats.min_schedule_time_us) {
        scheduler->stats.min_schedule_time_us = schedule_time_us;
    }
    if (schedule_time_us > scheduler->stats.max_schedule_time_us) {
        scheduler->stats.max_schedule_time_us = schedule_time_us;
    }
    
    /* Exponential moving average */
    scheduler->stats.avg_schedule_time_us = 
        (scheduler->stats.avg_schedule_time_us * 7U + schedule_time_us) / 8U;
}

/**
 * @brief Get task priority
 */
uint8_t dsrtos_priority_get(const dsrtos_priority_scheduler_t* scheduler,
                            const dsrtos_tcb_t* task)
{
    dsrtos_priority_node_t* node;
    uint32_t i;
    
    if ((scheduler == NULL) || (task == NULL)) {
        return PRIO_LOWEST;
    }
    
    /* Search all queues for task */
    for (i = 0U; i < PRIO_NUM_LEVELS; i++) {
        node = scheduler->ready_queues[i].head;
        while (node != NULL) {
            if (node->task == task) {
                return node->effective_priority;
            }
            node = node->next;
        }
    }
    
    return PRIO_LOWEST;
}

/**
 * @brief Remove task from scheduler
 */
dsrtos_status_t dsrtos_priority_remove(dsrtos_priority_scheduler_t* scheduler,
                                       dsrtos_tcb_t* task)
{
    dsrtos_priority_node_t* node;
    uint32_t i;
    
    PRIO_ASSERT_VALID_SCHEDULER(scheduler);
    
    if (task == NULL) {
        return DSRTOS_INVALID_PARAM;
    }
    
    PRIO_ENTER_CRITICAL(scheduler);
    
    /* Find and remove task from queues */
    for (i = 0U; i < PRIO_NUM_LEVELS; i++) {
        node = scheduler->ready_queues[i].head;
        while (node != NULL) {
            if (node->task == task) {
                dsrtos_priority_queue_remove(&scheduler->ready_queues[i], node);
                if (scheduler->ready_queues[i].count == 0U) {
                    PRIO_BITMAP_CLEAR(scheduler, (uint8_t)i);
                }
                dsrtos_priority_free_node(scheduler, node);
                
                scheduler->stats.priority_distribution[i]--;
                
                PRIO_EXIT_CRITICAL(scheduler);
                return DSRTOS_SUCCESS;
            }
            node = node->next;
        }
    }
    
    PRIO_EXIT_CRITICAL(scheduler);
    return DSRTOS_NOT_FOUND;
}
