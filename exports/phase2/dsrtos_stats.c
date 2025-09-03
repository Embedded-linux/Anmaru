/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Statistics Engine Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_stats.h"
#include "dsrtos_kernel_init.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include <string.h>
#include "core_cm4.h"

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define STATS_MAGIC             0x53544154U  /* 'STAT' */
#define STATS_MAX_ENTRIES       256U
#define STATS_NAME_MAX_LEN      32U

/*=============================================================================
 * PRIVATE TYPES
 *============================================================================*/
typedef struct {
    char name[STATS_NAME_MAX_LEN];
    dsrtos_stats_entry_t entry;
    bool allocated;
} stats_record_t;

typedef struct {
    /* Statistics storage */
    stats_record_t entries[STATS_MAX_ENTRIES];
    uint32_t entry_count;
    
    /* Category statistics */
    dsrtos_kernel_stats_t kernel_stats;
    dsrtos_task_stats_t task_stats;
    dsrtos_memory_stats_t memory_stats;
    dsrtos_scheduler_stats_t scheduler_stats;
    
    /* Configuration */
    dsrtos_stats_config_t config;
    
    /* State */
    uint32_t magic;
    bool initialized;
    uint64_t last_idle_ticks;
    uint64_t last_busy_ticks;
} stats_manager_t;

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
static stats_manager_t g_stats_mgr __attribute__((aligned(32))) = {
    .entries = {{0}},
    .entry_count = 0U,
    .kernel_stats = {0},
    .task_stats = {0},
    .memory_stats = {0},
    .scheduler_stats = {0},
    .config = {
        .enable_kernel_stats = true,
        .enable_task_stats = true,
        .enable_memory_stats = true,
        .enable_scheduler_stats = true,
        .sample_rate_ms = 100U,
        .history_size = 100U
    },
    .magic = 0U,
    .initialized = false,
    .last_idle_ticks = 0U,
    .last_busy_ticks = 0U
};

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static stats_record_t* stats_find_entry(dsrtos_stats_category_t category,
                                        const char* name);
static stats_record_t* stats_allocate_entry(void);
static void stats_update_cpu_usage(void);
static uint64_t stats_get_ticks(void);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize statistics engine
 */
dsrtos_error_t dsrtos_stats_init(const dsrtos_stats_config_t* config)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    else {
        /* Clear statistics manager */
        memset(&g_stats_mgr, 0, sizeof(g_stats_mgr));
        
        /* Apply configuration */
        if (config != NULL) {
            g_stats_mgr.config = *config;
        }
        
        /* Initialize manager */
        g_stats_mgr.magic = STATS_MAGIC;
        g_stats_mgr.initialized = true;
        
        /* Register with kernel */
        result = dsrtos_kernel_register_service(
            DSRTOS_SERVICE_STATISTICS,
            &g_stats_mgr
        );
    }
    
    return result;
}

/**
 * @brief Update a statistic counter
 */
dsrtos_error_t dsrtos_stats_update(dsrtos_stats_category_t category,
                                   const char* name,
                                   uint32_t value)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (name == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        /* Find or create entry */
        stats_record_t* record = stats_find_entry(category, name);
        if (record == NULL) {
            record = stats_allocate_entry();
            if (record != NULL) {
                strncpy(record->name, name, STATS_NAME_MAX_LEN - 1U);
                record->entry.category = category;
                record->entry.type = DSRTOS_STATS_TYPE_COUNTER;
                record->entry.name = record->name;
            }
        }
        
        if (record != NULL) {
            record->entry.value.counter += value;
            record->entry.update_count++;
            record->entry.timestamp = stats_get_ticks();
        }
        else {
            result = DSRTOS_ERROR_NO_MEMORY;
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Set a gauge value
 */
dsrtos_error_t dsrtos_stats_set_gauge(dsrtos_stats_category_t category,
                                      const char* name,
                                      int32_t value)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (name == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        /* Find or create entry */
        stats_record_t* record = stats_find_entry(category, name);
        if (record == NULL) {
            record = stats_allocate_entry();
            if (record != NULL) {
                strncpy(record->name, name, STATS_NAME_MAX_LEN - 1U);
                record->entry.category = category;
                record->entry.type = DSRTOS_STATS_TYPE_GAUGE;
                record->entry.name = record->name;
            }
        }
        
        if (record != NULL) {
            record->entry.value.gauge = value;
            record->entry.update_count++;
            record->entry.timestamp = stats_get_ticks();
        }
        else {
            result = DSRTOS_ERROR_NO_MEMORY;
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Get kernel statistics
 */
dsrtos_error_t dsrtos_stats_get_kernel(dsrtos_kernel_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        /* Update CPU usage */
        stats_update_cpu_usage();
        
        /* Copy statistics */
        *stats = g_stats_mgr.kernel_stats;
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Get task statistics
 */
dsrtos_error_t dsrtos_stats_get_task(dsrtos_task_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        *stats = g_stats_mgr.task_stats;
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Get memory statistics
 */
dsrtos_error_t dsrtos_stats_get_memory(dsrtos_memory_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        *stats = g_stats_mgr.memory_stats;
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Get scheduler statistics
 */
dsrtos_error_t dsrtos_stats_get_scheduler(dsrtos_scheduler_stats_t* stats)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (stats == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        *stats = g_stats_mgr.scheduler_stats;
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Reset statistics
 */
dsrtos_error_t dsrtos_stats_reset(dsrtos_stats_category_t category)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        if (category == DSRTOS_STATS_CAT_MAX) {
            /* Reset all statistics */
            memset(&g_stats_mgr.kernel_stats, 0, sizeof(g_stats_mgr.kernel_stats));
            memset(&g_stats_mgr.task_stats, 0, sizeof(g_stats_mgr.task_stats));
            memset(&g_stats_mgr.memory_stats, 0, sizeof(g_stats_mgr.memory_stats));
            memset(&g_stats_mgr.scheduler_stats, 0, sizeof(g_stats_mgr.scheduler_stats));
            
            /* Reset entries */
            for (uint32_t i = 0U; i < STATS_MAX_ENTRIES; i++) {
                if (g_stats_mgr.entries[i].allocated) {
                    memset(&g_stats_mgr.entries[i].entry, 0, 
                          sizeof(dsrtos_stats_entry_t));
                }
            }
        }
        else {
            /* Reset specific category */
            switch (category) {
                case DSRTOS_STATS_CAT_KERNEL:
                    memset(&g_stats_mgr.kernel_stats, 0, 
                          sizeof(g_stats_mgr.kernel_stats));
                    break;
                case DSRTOS_STATS_CAT_TASK:
                    memset(&g_stats_mgr.task_stats, 0, 
                          sizeof(g_stats_mgr.task_stats));
                    break;
                case DSRTOS_STATS_CAT_MEMORY:
                    memset(&g_stats_mgr.memory_stats, 0, 
                          sizeof(g_stats_mgr.memory_stats));
                    break;
                case DSRTOS_STATS_CAT_SCHEDULER:
                    memset(&g_stats_mgr.scheduler_stats, 0, 
                          sizeof(g_stats_mgr.scheduler_stats));
                    break;
		    case DSRTOS_STATS_CAT_INTERRUPT:
		case DSRTOS_STATS_CAT_TIMER:
		case DSRTOS_STATS_CAT_DRIVER:
		case DSRTOS_STATS_CAT_CUSTOM:
		case DSRTOS_STATS_CAT_MAX:
                	default:
                    		break;
            }
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Calculate CPU usage percentage
 */
uint8_t dsrtos_stats_cpu_usage(void)
{
    uint8_t usage = 0U;
    
    if (g_stats_mgr.initialized) {
        dsrtos_critical_enter();
        stats_update_cpu_usage();
        usage = g_stats_mgr.kernel_stats.cpu_usage_percent;
        dsrtos_critical_exit();
    }
    
    return usage;
}

/**
 * @brief Get statistics entry by name
 */
dsrtos_error_t dsrtos_stats_get_entry(dsrtos_stats_category_t category,
                                      const char* name,
                                      dsrtos_stats_entry_t* entry)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((name == NULL) || (entry == NULL)) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        stats_record_t* record = stats_find_entry(category, name);
        if (record != NULL) {
            *entry = record->entry;
        }
        else {
            result = DSRTOS_ERROR_NOT_FOUND;
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Register custom statistic
 */
dsrtos_error_t dsrtos_stats_register(const dsrtos_stats_entry_t* entry)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((entry == NULL) || (entry->name == NULL)) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        dsrtos_critical_enter();
        
        stats_record_t* record = stats_allocate_entry();
        if (record != NULL) {
            strncpy(record->name, entry->name, STATS_NAME_MAX_LEN - 1U);
            record->entry = *entry;
            record->entry.name = record->name;
        }
        else {
            result = DSRTOS_ERROR_NO_MEMORY;
        }
        
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Export statistics snapshot
 */
dsrtos_error_t dsrtos_stats_export(void* buffer, size_t* size)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if ((buffer == NULL) || (size == NULL)) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_stats_mgr.initialized) {
        result = DSRTOS_ERROR_NOT_INITIALIZED;
    }
    else {
        /* This would export statistics in a defined format */
        /* Implementation depends on requirements */
        *size = 0U;
    }
    
    return result;
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Find statistics entry
 */
static stats_record_t* stats_find_entry(dsrtos_stats_category_t category,
                                        const char* name)
{
    stats_record_t* record = NULL;
    
    for (uint32_t i = 0U; i < STATS_MAX_ENTRIES; i++) {
        if (g_stats_mgr.entries[i].allocated &&
            (g_stats_mgr.entries[i].entry.category == category) &&
            (strncmp(g_stats_mgr.entries[i].name, name, STATS_NAME_MAX_LEN) == 0)) {
            record = &g_stats_mgr.entries[i];
            break;
        }
    }
    
    return record;
}

/**
 * @brief Allocate statistics entry
 */
static stats_record_t* stats_allocate_entry(void)
{
    stats_record_t* record = NULL;
    
    if (g_stats_mgr.entry_count < STATS_MAX_ENTRIES) {
        for (uint32_t i = 0U; i < STATS_MAX_ENTRIES; i++) {
            if (!g_stats_mgr.entries[i].allocated) {
                record = &g_stats_mgr.entries[i];
                record->allocated = true;
                g_stats_mgr.entry_count++;
                break;
            }
        }
    }
    
    return record;
}

/**
 * @brief Update CPU usage
 */
static void stats_update_cpu_usage(void)
{
    dsrtos_kernel_t* kernel = dsrtos_kernel_get_kcb();
    
    if (kernel != NULL) {
        uint64_t total_ticks = kernel->uptime_ticks;
        uint64_t idle_ticks = kernel->idle_ticks;
        
        if (total_ticks > 0U) {
            uint64_t busy_ticks = total_ticks - idle_ticks;
            g_stats_mgr.kernel_stats.cpu_usage_percent = 
                (uint8_t)((busy_ticks * 100U) / total_ticks);
            
            if (g_stats_mgr.kernel_stats.cpu_usage_percent > 
                g_stats_mgr.kernel_stats.peak_cpu_usage) {
                g_stats_mgr.kernel_stats.peak_cpu_usage = 
                    g_stats_mgr.kernel_stats.cpu_usage_percent;
            }
        }
        
        g_stats_mgr.kernel_stats.uptime_ticks = total_ticks;
        g_stats_mgr.kernel_stats.idle_ticks = idle_ticks;
        g_stats_mgr.kernel_stats.busy_ticks = total_ticks - idle_ticks;
    }
}

/**
 * @brief Get current system ticks
 */
static uint64_t stats_get_ticks(void)
{
    dsrtos_kernel_t* kernel = dsrtos_kernel_get_kcb();
    return (kernel != NULL) ? kernel->uptime_ticks : 0U;
}
