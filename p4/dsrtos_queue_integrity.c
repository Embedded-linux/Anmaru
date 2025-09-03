/*
 * DSRTOS Queue Integrity Checking Implementation
 * 
 * Copyright (C) 2024 DSRTOS
 * SPDX-License-Identifier: MIT
 * 
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 Compliant
 */

#include "dsrtos_queue_integrity.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_trace.h"
#include "dsrtos_panic.h"
#include <string.h>

/* Configuration */
#define INTEGRITY_CHECK_INTERVAL    1000U  /* Ticks between checks */
#define MAX_REPAIR_ATTEMPTS        3U
#define MAX_NODES_PER_LIST         DSRTOS_MAX_TASKS

/* Global integrity report */
static dsrtos_integrity_report_t g_integrity_report = {
    .status = DSRTOS_INTEGRITY_OK,
    .error_count = 0U,
    .last_check_tick = 0U,
    .check_count = 0U,
    .repairs_attempted = 0U,
    .repairs_successful = 0U,
    .cycles_detected = 0U,
    .nodes_corrupted = 0U
};

/* Last check timestamp */
static uint32_t g_last_periodic_check = 0U;

/* Static function prototypes */
static bool check_node_chain(const dsrtos_queue_node_t* start, uint32_t max_nodes);
static dsrtos_error_t repair_list(dsrtos_priority_list_t* list);
static dsrtos_error_t rebuild_queue(dsrtos_ready_queue_t* queue);

/**
 * @brief Initialize queue integrity subsystem
 */
dsrtos_error_t dsrtos_queue_integrity_init(void)
{
    (void)memset(&g_integrity_report, 0, sizeof(g_integrity_report));
    g_integrity_report.status = DSRTOS_INTEGRITY_OK;
    g_last_periodic_check = 0U;
    
    DSRTOS_TRACE_INTEGRITY("Queue integrity checking initialized");
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Comprehensive queue integrity check
 */
dsrtos_integrity_status_t dsrtos_queue_check_integrity(
    dsrtos_ready_queue_t* queue)
{
    uint32_t i;
    uint32_t counted_tasks = 0U;
    dsrtos_integrity_status_t status = DSRTOS_INTEGRITY_OK;
    
    if (queue == NULL) {
        return DSRTOS_INTEGRITY_LIST_CORRUPTED;
    }
    
    dsrtos_critical_enter();
    
    g_integrity_report.check_count++;
    
    /* Check magic numbers */
    if ((queue->magic_start != DSRTOS_QUEUE_MAGIC) ||
        (queue->magic_end != DSRTOS_QUEUE_MAGIC)) {
        status = DSRTOS_INTEGRITY_MAGIC_FAIL;
        g_integrity_report.error_count++;
        goto exit;
    }
    
    /* Verify bitmap consistency */
    for (i = 0U; i < DSRTOS_BITMAP_WORDS; i++) {
        if (queue->priority_bitmap[i] != queue->priority_bitmap_mirror[i]) {
            status = DSRTOS_INTEGRITY_BITMAP_MISMATCH;
            g_integrity_report.error_count++;
            goto exit;
        }
    }
    
    /* Check each priority list */
    for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
        const dsrtos_priority_list_t* list = &queue->priority_lists[i];
        dsrtos_queue_node_t* node = list->head;
        uint32_t node_count = 0U;
        
        /* Detect cycles in list */
        if (dsrtos_queue_detect_cycle(list)) {
            status = DSRTOS_INTEGRITY_CYCLE_DETECTED;
            g_integrity_report.cycles_detected++;
            goto exit;
        }
        
        /* Validate all nodes */
        while (node != NULL) {
            if (!dsrtos_node_validate(node)) {
                status = DSRTOS_INTEGRITY_NODE_CORRUPTED;
                g_integrity_report.nodes_corrupted++;
                goto exit;
            }
            
            node_count++;
            
            /* Prevent infinite loops */
            if (node_count > MAX_NODES_PER_LIST) {
                status = DSRTOS_INTEGRITY_LIST_CORRUPTED;
                goto exit;
            }
            
            node = node->next;
        }
        
        /* Verify count consistency */
        if (node_count != list->count) {
            status = DSRTOS_INTEGRITY_COUNT_MISMATCH;
            g_integrity_report.error_count++;
            goto exit;
        }
        
        counted_tasks += node_count;
        
        /* Verify bitmap bit consistency */
        bool has_tasks = (node_count > 0U);
        bool bit_set = dsrtos_priority_bitmap_is_set(queue->priority_bitmap, (uint8_t)i);
        
        if (has_tasks != bit_set) {
            status = DSRTOS_INTEGRITY_BITMAP_MISMATCH;
            g_integrity_report.error_count++;
            goto exit;
        }
    }
    
    /* Verify total task count */
    if (counted_tasks != queue->stats.total_tasks) {
        status = DSRTOS_INTEGRITY_COUNT_MISMATCH;
        g_integrity_report.error_count++;
    }

exit:
    g_integrity_report.status = status;
    g_integrity_report.last_check_tick = dsrtos_get_tick_count();
    
    if (status != DSRTOS_INTEGRITY_OK) {
        DSRTOS_TRACE_ERROR("Queue integrity check failed: %d", status);
        queue->stats.corruptions_detected++;
    }
    
    dsrtos_critical_exit();
    
    return status;
}

/**
 * @brief Repair queue integrity with specified level
 */
dsrtos_error_t dsrtos_queue_repair_integrity(
    dsrtos_ready_queue_t* queue,
    dsrtos_repair_level_t level)
{
    dsrtos_error_t result = DSRTOS_ERROR_CORRUPTED;
    uint32_t i;
    
    if (queue == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    DSRTOS_TRACE_WARNING("Attempting queue repair, level=%d", level);
    
    dsrtos_critical_enter();
    
    g_integrity_report.repairs_attempted++;
    
    switch (level) {
        case DSRTOS_REPAIR_MINIMAL:
            /* Fix magic numbers only */
            queue->magic_start = DSRTOS_QUEUE_MAGIC;
            queue->magic_end = DSRTOS_QUEUE_MAGIC;
            result = DSRTOS_SUCCESS;
            break;
            
        case DSRTOS_REPAIR_MODERATE:
            /* Fix magic and sync bitmaps */
            queue->magic_start = DSRTOS_QUEUE_MAGIC;
            queue->magic_end = DSRTOS_QUEUE_MAGIC;
            
            /* Sync mirror bitmap with primary */
            for (i = 0U; i < DSRTOS_BITMAP_WORDS; i++) {
                queue->priority_bitmap_mirror[i] = queue->priority_bitmap[i];
            }
            result = DSRTOS_SUCCESS;
            break;
            
        case DSRTOS_REPAIR_AGGRESSIVE:
            /* Repair lists and recalculate counts */
            for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
                (void)repair_list(&queue->priority_lists[i]);
            }
            
            /* Rebuild bitmap from lists */
            (void)memset((void*)queue->priority_bitmap, 0, sizeof(queue->priority_bitmap));
            queue->stats.total_tasks = 0U;
            
            for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
                if (queue->priority_lists[i].count > 0U) {
                    dsrtos_priority_bitmap_set(queue->priority_bitmap, (uint8_t)i);
                    queue->stats.total_tasks += queue->priority_lists[i].count;
                }
            }
            
            /* Sync mirror */
            (void)memcpy((void*)queue->priority_bitmap_mirror,
                        (const void*)queue->priority_bitmap,
                        sizeof(queue->priority_bitmap));
            
            result = DSRTOS_SUCCESS;
            break;
            
        case DSRTOS_REPAIR_REBUILD:
            /* Complete rebuild */
            result = rebuild_queue(queue);
            break;
            
        default:
            result = DSRTOS_ERROR_INVALID_PARAMETER;
            break;
    }
    
    /* Verify repair */
    if ((result == DSRTOS_SUCCESS) &&
        (dsrtos_queue_check_integrity(queue) == DSRTOS_INTEGRITY_OK)) {
        g_integrity_report.repairs_successful++;
        queue->stats.repairs_performed++;
        DSRTOS_TRACE_INFO("Queue repair successful");
    } else {
        DSRTOS_TRACE_ERROR("Queue repair failed");
        
        /* Critical failure - panic if aggressive repair fails */
        if (level >= DSRTOS_REPAIR_AGGRESSIVE) {
            dsrtos_panic("Unable to repair critical queue corruption");
        }
    }
    
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Get integrity report
 */
dsrtos_error_t dsrtos_queue_get_integrity_report(
    dsrtos_integrity_report_t* report)
{
    if (report == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    dsrtos_critical_enter();
    *report = g_integrity_report;
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Calculate queue checksum
 */
uint32_t dsrtos_queue_calculate_checksum(const dsrtos_ready_queue_t* queue)
{
    const uint8_t* data;
    uint32_t checksum = 0x5A5A5A5AU;
    size_t i;
    
    if (queue == NULL) {
        return 0U;
    }
    
    data = (const uint8_t*)queue;
    
    /* Calculate rotating XOR checksum */
    for (i = 0U; i < sizeof(dsrtos_ready_queue_t); i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 31);
    }
    
    return checksum;
}

/**
 * @brief Validate queue node
 */
bool dsrtos_node_validate(const dsrtos_queue_node_t* node)
{
    uint32_t calculated_checksum;
    
    if (node == NULL) {
        return false;
    }
    
    /* Check magic numbers */
    if ((node->magic_start != DSRTOS_NODE_MAGIC) ||
        (node->magic_end != DSRTOS_NODE_MAGIC)) {
        return false;
    }
    
    /* Verify task pointer */
    if ((node->task == NULL) ||
        (node->task->magic_number != DSRTOS_TCB_MAGIC)) {
        return false;
    }
    
    /* Calculate and verify checksum */
    calculated_checksum = 0x12345678U;
    const uint8_t* data = (const uint8_t*)node;
    size_t size = sizeof(dsrtos_queue_node_t) - sizeof(uint32_t);
    
    for (size_t i = 0U; i < size; i++) {
        calculated_checksum ^= data[i];
        calculated_checksum = (calculated_checksum << 1) | (calculated_checksum >> 31);
    }
    
    if (calculated_checksum != node->checksum) {
        return false;
    }
    
    return true;
}

/**
 * @brief Detect cycle in priority list
 */
bool dsrtos_queue_detect_cycle(const dsrtos_priority_list_t* list)
{
    dsrtos_queue_node_t* slow;
    dsrtos_queue_node_t* fast;
    
    if ((list == NULL) || (list->head == NULL)) {
        return false;
    }
    
    /* Floyd's cycle detection algorithm */
    slow = list->head;
    fast = list->head;
    
    while ((fast != NULL) && (fast->next != NULL)) {
        slow = slow->next;
        fast = fast->next->next;
        
        if (slow == fast) {
            /* Cycle detected */
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Verify queue consistency
 */
dsrtos_error_t dsrtos_queue_verify_consistency(const dsrtos_ready_queue_t* queue)
{
    if (queue == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Full consistency check */
    dsrtos_integrity_status_t status = dsrtos_queue_check_integrity(
        (dsrtos_ready_queue_t*)queue);
    
    if (status != DSRTOS_INTEGRITY_OK) {
        return DSRTOS_ERROR_CORRUPTED;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Periodic integrity check
 */
void dsrtos_queue_integrity_periodic_check(void)
{
    uint32_t current_tick = dsrtos_get_tick_count();
    extern dsrtos_ready_queue_t g_ready_queue;
    
    /* Check if interval has elapsed */
    if ((current_tick - g_last_periodic_check) >= INTEGRITY_CHECK_INTERVAL) {
        g_last_periodic_check = current_tick;
        
        /* Perform integrity check */
        dsrtos_integrity_status_t status = dsrtos_queue_check_integrity(&g_ready_queue);
        
        if (status != DSRTOS_INTEGRITY_OK) {
            /* Attempt automatic repair */
            dsrtos_error_t result = dsrtos_queue_repair_integrity(
                &g_ready_queue, DSRTOS_REPAIR_MODERATE);
            
            if (result != DSRTOS_SUCCESS) {
                /* Escalate repair level */
                result = dsrtos_queue_repair_integrity(
                    &g_ready_queue, DSRTOS_REPAIR_AGGRESSIVE);
                
                if (result != DSRTOS_SUCCESS) {
                    /* Critical failure */
                    DSRTOS_ASSERT(false, "Queue integrity failure");
                }
            }
        }
    }
}

/*==============================================================================
 * STATIC HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Check node chain integrity
 */
static bool check_node_chain(const dsrtos_queue_node_t* start, uint32_t max_nodes)
{
    const dsrtos_queue_node_t* node = start;
    uint32_t count = 0U;
    
    while (node != NULL) {
        if (!dsrtos_node_validate(node)) {
            return false;
        }
        
        count++;
        if (count > max_nodes) {
            /* Too many nodes or cycle */
            return false;
        }
        
        node = node->next;
    }
    
    return true;
}

/**
 * @brief Repair a priority list
 */
static dsrtos_error_t repair_list(dsrtos_priority_list_t* list)
{
    dsrtos_queue_node_t* node;
    dsrtos_queue_node_t* prev = NULL;
    dsrtos_queue_node_t* next;
    uint32_t valid_count = 0U;
    
    if (list == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    node = list->head;
    list->head = NULL;
    list->tail = NULL;
    
    /* Rebuild list with valid nodes only */
    while (node != NULL) {
        next = node->next;
        
        if (dsrtos_node_validate(node)) {
            /* Valid node - add to repaired list */
            if (list->head == NULL) {
                list->head = node;
                node->prev = NULL;
            } else {
                prev->next = node;
                node->prev = prev;
            }
            prev = node;
            valid_count++;
        }
        
        node = next;
    }
    
    /* Update tail and count */
    list->tail = prev;
    if (list->tail != NULL) {
        list->tail->next = NULL;
    }
    list->count = valid_count;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Rebuild entire queue from scratch
 */
static dsrtos_error_t rebuild_queue(dsrtos_ready_queue_t* queue)
{
    uint32_t i;
    
    if (queue == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear everything */
    (void)memset((void*)queue->priority_bitmap, 0, sizeof(queue->priority_bitmap));
    (void)memset((void*)queue->priority_bitmap_mirror, 0, sizeof(queue->priority_bitmap_mirror));
    
    /* Rebuild each list */
    for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
        (void)repair_list(&queue->priority_lists[i]);
    }
    
    /* Recalculate statistics */
    queue->stats.total_tasks = 0U;
    for (i = 0U; i < DSRTOS_PRIORITY_LEVELS; i++) {
        queue->stats.total_tasks += queue->priority_lists[i].count;
        
        if (queue->priority_lists[i].count > 0U) {
            dsrtos_priority_bitmap_set(queue->priority_bitmap, (uint8_t)i);
            dsrtos_priority_bitmap_set(queue->priority_bitmap_mirror, (uint8_t)i);
        }
    }
    
    /* Restore magic numbers */
    queue->magic_start = DSRTOS_QUEUE_MAGIC;
    queue->magic_end = DSRTOS_QUEUE_MAGIC;
    
    return DSRTOS_SUCCESS;
}
