/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Pluggable Scheduler Core - Framework Implementation
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
#include "dsrtos_scheduler_core.h"
#include "dsrtos_critical.h"
#include "dsrtos_memory.h"
#include "dsrtos_config.h"
#include "dsrtos_assert.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define SCHED_CORE_TRACE(msg)    DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define SCHED_CORE_ASSERT(cond)  DSRTOS_ASSERT(cond)

/* Scheduler switch states */
#define SWITCH_STATE_IDLE        (0U)
#define SWITCH_STATE_PREPARING   (1U)
#define SWITCH_STATE_SWITCHING   (2U)
#define SWITCH_STATE_COMPLETING  (3U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Scheduler registry entry */
typedef struct {
    dsrtos_scheduler_plugin_t* plugin;
    void* context;
    dsrtos_scheduler_state_t state;
    uint32_t activation_count;
    uint64_t total_active_time;
} scheduler_registry_entry_t;

/* Core scheduler control block */
typedef struct {
    /* Registry */
    scheduler_registry_entry_t registry[DSRTOS_MAX_SCHEDULERS];
    uint32_t registered_count;
    
    /* Active scheduler */
    uint16_t active_scheduler_id;
    dsrtos_scheduler_plugin_t* active_scheduler;
    void* active_context;
    
    /* Switching control */
    struct {
        volatile uint32_t state;
        uint16_t target_scheduler_id;
        uint32_t switch_start_time;
        dsrtos_scheduler_switch_request_t pending_request;
    } switching;
    
    /* Global context */
    dsrtos_scheduler_context_t context;
    
    /* Statistics */
    struct {
        uint32_t total_switches;
        uint32_t failed_switches;
        uint32_t adaptation_triggers;
        uint64_t total_scheduling_time;
    } stats;
    
    /* State */
    volatile uint32_t initialized;
    volatile uint32_t running;
    uint32_t critical_nesting;
} scheduler_core_cb_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static scheduler_core_cb_t g_scheduler_core = {0};

/*==============================================================================
 *                     LOCAL FUNCTION PROTOTYPES
 *============================================================================*/
static scheduler_registry_entry_t* find_registry_entry(uint16_t scheduler_id);
static dsrtos_error_t validate_scheduler_plugin(
    const dsrtos_scheduler_plugin_t* plugin);
static dsrtos_error_t prepare_scheduler_switch(uint16_t new_scheduler_id);
static dsrtos_error_t perform_scheduler_switch(void);
static void update_scheduler_metrics(void);
static void analyze_workload_patterns(void);
static uint16_t calculate_optimal_scheduler(void);
static uint32_t calculate_plugin_checksum(const dsrtos_scheduler_plugin_t* plugin);

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize the scheduler core framework
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_core_init(void)
{
    uint32_t sr;
    
    SCHED_CORE_TRACE("Scheduler core initialization");
    
    sr = dsrtos_critical_enter();
    
    /* Check if already initialized */
    if (g_scheduler_core.initialized != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Clear the control block */
    (void)memset(&g_scheduler_core, 0, sizeof(g_scheduler_core));
    
    /* Initialize context */
    g_scheduler_core.context.state = DSRTOS_SCHED_STATE_INACTIVE;
    g_scheduler_core.context.selection.switch_hysteresis_ms = 100U;
    g_scheduler_core.context.selection.stability_counter = 0U;
    
    /* Mark as initialized */
    g_scheduler_core.initialized = 1U;
    
    dsrtos_critical_exit(sr);
    
    SCHED_CORE_TRACE("Scheduler core initialized successfully");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Start the scheduler core
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_core_start(void)
{
    uint32_t sr;
    dsrtos_error_t err;
    
    SCHED_CORE_TRACE("Starting scheduler core");
    
    sr = dsrtos_critical_enter();
    
    /* Check initialization */
    if (g_scheduler_core.initialized == 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* Check if already running */
    if (g_scheduler_core.running != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_STARTED;
    }
    
    /* Check if we have an active scheduler */
    if (g_scheduler_core.active_scheduler == NULL) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NO_SCHEDULER;
    }
    
    /* Start the active scheduler */
    if (g_scheduler_core.active_scheduler->start != NULL) {
        err = g_scheduler_core.active_scheduler->start(
            g_scheduler_core.active_context);
        if (err != DSRTOS_SUCCESS) {
            dsrtos_critical_exit(sr);
            return err;
        }
    }
    
    /* Update state */
    g_scheduler_core.running = 1U;
    g_scheduler_core.context.state = DSRTOS_SCHED_STATE_ACTIVE;
    
    dsrtos_critical_exit(sr);
    
    SCHED_CORE_TRACE("Scheduler core started");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Register a scheduler plugin
 * @param plugin Pointer to scheduler plugin
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_register(dsrtos_scheduler_plugin_t* plugin)
{
    uint32_t sr;
    dsrtos_error_t err;
    scheduler_registry_entry_t* entry;
    void* context = NULL;
    
    /* Validate plugin */
    err = validate_scheduler_plugin(plugin);
    if (err != DSRTOS_SUCCESS) {
        return err;
    }
    
    sr = dsrtos_critical_enter();
    
    /* Check if already registered */
    entry = find_registry_entry(plugin->id);
    if (entry != NULL) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_EXISTS;
    }
    
    /* Check registry space */
    if (g_scheduler_core.registered_count >= DSRTOS_MAX_SCHEDULERS) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NO_RESOURCES;
    }
    
    /* Initialize the plugin */
    if (plugin->init != NULL) {
        err = plugin->init(&context);
        if (err != DSRTOS_SUCCESS) {
            dsrtos_critical_exit(sr);
            return err;
        }
    }
    
    /* Find free entry */
    for (uint32_t i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        if (g_scheduler_core.registry[i].plugin == NULL) {
            entry = &g_scheduler_core.registry[i];
            break;
        }
    }
    
    SCHED_CORE_ASSERT(entry != NULL);
    
    /* Register the plugin */
    entry->plugin = plugin;
    entry->context = context;
    entry->state = DSRTOS_SCHED_STATE_INACTIVE;
    entry->activation_count = 0U;
    entry->total_active_time = 0U;
    
    g_scheduler_core.registered_count++;
    
    dsrtos_critical_exit(sr);
    
    SCHED_CORE_TRACE("Scheduler registered");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Activate a registered scheduler
 * @param scheduler_id Scheduler ID to activate
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_activate(uint16_t scheduler_id)
{
    uint32_t sr;
    scheduler_registry_entry_t* entry;
    dsrtos_error_t err;
    
    SCHED_CORE_TRACE("Activating scheduler");
    
    sr = dsrtos_critical_enter();
    
    /* Find the scheduler */
    entry = find_registry_entry(scheduler_id);
    if (entry == NULL) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Check if already active */
    if (g_scheduler_core.active_scheduler_id == scheduler_id) {
        dsrtos_critical_exit(sr);
        return DSRTOS_SUCCESS;
    }
    
    /* If another scheduler is active, switch */
    if (g_scheduler_core.active_scheduler != NULL) {
        dsrtos_critical_exit(sr);
        return dsrtos_scheduler_switch(scheduler_id);
    }
    
    /* Activate the scheduler */
    g_scheduler_core.active_scheduler_id = scheduler_id;
    g_scheduler_core.active_scheduler = entry->plugin;
    g_scheduler_core.active_context = entry->context;
    g_scheduler_core.context.active_scheduler = entry->plugin;
    
    /* Update entry state */
    entry->state = DSRTOS_SCHED_STATE_ACTIVE;
    entry->activation_count++;
    
    /* Start if core is running */
    if (g_scheduler_core.running != 0U) {
        if (entry->plugin->start != NULL) {
            err = entry->plugin->start(entry->context);
            if (err != DSRTOS_SUCCESS) {
                g_scheduler_core.active_scheduler = NULL;
                g_scheduler_core.active_scheduler_id = DSRTOS_SCHED_ID_NONE;
                entry->state = DSRTOS_SCHED_STATE_ERROR;
                dsrtos_critical_exit(sr);
                return err;
            }
        }
    }
    
    dsrtos_critical_exit(sr);
    
    SCHED_CORE_TRACE("Scheduler activated");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Switch to a different scheduler
 * @param new_scheduler_id Target scheduler ID
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_switch(uint16_t new_scheduler_id)
{
    uint32_t sr;
    dsrtos_error_t err;
    
    SCHED_CORE_TRACE("Scheduler switch requested");
    
    sr = dsrtos_critical_enter();
    
    /* Check if switch is in progress */
    if (g_scheduler_core.switching.state != SWITCH_STATE_IDLE) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_BUSY;
    }
    
    /* Validate target scheduler exists */
    if (find_registry_entry(new_scheduler_id) == NULL) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Check if same scheduler */
    if (g_scheduler_core.active_scheduler_id == new_scheduler_id) {
        dsrtos_critical_exit(sr);
        return DSRTOS_SUCCESS;
    }
    
    /* Prepare for switch */
    err = prepare_scheduler_switch(new_scheduler_id);
    if (err != DSRTOS_SUCCESS) {
        dsrtos_critical_exit(sr);
        return err;
    }
    
    /* Perform the switch */
    err = perform_scheduler_switch();
    
    dsrtos_critical_exit(sr);
    
    if (err == DSRTOS_SUCCESS) {
        SCHED_CORE_TRACE("Scheduler switch completed");
        g_scheduler_core.stats.total_switches++;
    } else {
        SCHED_CORE_TRACE("Scheduler switch failed");
        g_scheduler_core.stats.failed_switches++;
    }
    
    return err;
}

/**
 * @brief Perform scheduler adaptation based on current workload
 */
void dsrtos_scheduler_adapt(void)
{
    uint16_t recommended_id;
    dsrtos_scheduler_plugin_t* current;
    
    /* Check if adaptation is enabled */
    if (g_scheduler_core.running == 0U) {
        return;
    }
    
    /* Update metrics */
    update_scheduler_metrics();
    
    /* Analyze workload */
    analyze_workload_patterns();
    
    /* Get current scheduler */
    current = g_scheduler_core.active_scheduler;
    if (current == NULL) {
        return;
    }
    
    /* Check if current scheduler recommends switching */
    if (current->ops.should_switch_scheduler != NULL) {
        if (current->ops.should_switch_scheduler(
                g_scheduler_core.active_context)) {
            
            /* Get recommendation */
            if (current->ops.recommend_next_scheduler != NULL) {
                recommended_id = current->ops.recommend_next_scheduler(
                    g_scheduler_core.active_context);
            } else {
                recommended_id = calculate_optimal_scheduler();
            }
            
            /* Check hysteresis */
            uint32_t current_time = dsrtos_get_tick_count();
            uint32_t time_since_switch = current_time - 
                g_scheduler_core.context.selection.last_switch_time;
            
            if (time_since_switch > 
                g_scheduler_core.context.selection.switch_hysteresis_ms) {
                
                /* Request switch */
                dsrtos_scheduler_switch_request_t request = {
                    .target_scheduler_id = recommended_id,
                    .switch_reason = 0x1000, /* Adaptation */
                    .switch_time_ms = 0,
                    .switch_context = NULL
                };
                
                (void)dsrtos_scheduler_switch_request(&request);
                g_scheduler_core.stats.adaptation_triggers++;
            }
        }
    }
}

/**
 * @brief Get the current scheduler context
 * @return Pointer to scheduler context
 */
dsrtos_scheduler_context_t* dsrtos_scheduler_get_context(void)
{
    return &g_scheduler_core.context;
}

/*==============================================================================
 *                          LOCAL FUNCTIONS
 *============================================================================*/

/**
 * @brief Find a scheduler in the registry
 * @param scheduler_id Scheduler ID to find
 * @return Registry entry or NULL
 */
static scheduler_registry_entry_t* find_registry_entry(uint16_t scheduler_id)
{
    for (uint32_t i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        if ((g_scheduler_core.registry[i].plugin != NULL) &&
            (g_scheduler_core.registry[i].plugin->id == scheduler_id)) {
            return &g_scheduler_core.registry[i];
        }
    }
    return NULL;
}

/**
 * @brief Validate a scheduler plugin
 * @param plugin Plugin to validate
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t validate_scheduler_plugin(
    const dsrtos_scheduler_plugin_t* plugin)
{
    /* Check NULL */
    if (plugin == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Check magic number */
    if (plugin->magic != DSRTOS_SCHEDULER_MAGIC) {
        return DSRTOS_ERROR_INVALID_MAGIC;
    }
    
    /* Check mandatory operations */
    if ((plugin->ops.select_next_task == NULL) ||
        (plugin->ops.enqueue_task == NULL) ||
        (plugin->ops.dequeue_task == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate checksum */
    uint32_t checksum = calculate_plugin_checksum(plugin);
    if (checksum != plugin->checksum) {
        return DSRTOS_ERROR_CHECKSUM;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Prepare for scheduler switch
 * @param new_scheduler_id Target scheduler ID
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t prepare_scheduler_switch(uint16_t new_scheduler_id)
{
    scheduler_registry_entry_t* new_entry;
    scheduler_registry_entry_t* old_entry;
    
    /* Get entries */
    new_entry = find_registry_entry(new_scheduler_id);
    old_entry = find_registry_entry(g_scheduler_core.active_scheduler_id);
    
    if (new_entry == NULL) {
        return DSRTOS_ERROR_NOT_FOUND;
    }
    
    /* Update switch state */
    g_scheduler_core.switching.state = SWITCH_STATE_PREPARING;
    g_scheduler_core.switching.target_scheduler_id = new_scheduler_id;
    g_scheduler_core.switching.switch_start_time = dsrtos_get_tick_count();
    
    /* Suspend old scheduler */
    if ((old_entry != NULL) && (old_entry->plugin->suspend != NULL)) {
        (void)old_entry->plugin->suspend(old_entry->context);
        old_entry->state = DSRTOS_SCHED_STATE_SUSPENDED;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Perform the actual scheduler switch
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t perform_scheduler_switch(void)
{
    scheduler_registry_entry_t* new_entry;
    scheduler_registry_entry_t* old_entry;
    dsrtos_error_t err;
    
    /* Update state */
    g_scheduler_core.switching.state = SWITCH_STATE_SWITCHING;
    
    /* Get entries */
    new_entry = find_registry_entry(g_scheduler_core.switching.target_scheduler_id);
    old_entry = find_registry_entry(g_scheduler_core.active_scheduler_id);
    
    /* Stop old scheduler */
    if ((old_entry != NULL) && (old_entry->plugin->stop != NULL)) {
        (void)old_entry->plugin->stop(old_entry->context);
        old_entry->state = DSRTOS_SCHED_STATE_INACTIVE;
    }
    
    /* Update active scheduler */
    g_scheduler_core.active_scheduler_id = new_entry->plugin->id;
    g_scheduler_core.active_scheduler = new_entry->plugin;
    g_scheduler_core.active_context = new_entry->context;
    g_scheduler_core.context.active_scheduler = new_entry->plugin;
    
    /* Start new scheduler */
    if (new_entry->plugin->start != NULL) {
        err = new_entry->plugin->start(new_entry->context);
        if (err != DSRTOS_SUCCESS) {
            /* Rollback */
            if (old_entry != NULL) {
                g_scheduler_core.active_scheduler_id = old_entry->plugin->id;
                g_scheduler_core.active_scheduler = old_entry->plugin;
                g_scheduler_core.active_context = old_entry->context;
                if (old_entry->plugin->start != NULL) {
                    (void)old_entry->plugin->start(old_entry->context);
                }
            }
            g_scheduler_core.switching.state = SWITCH_STATE_IDLE;
            return err;
        }
    }
    
    /* Update states */
    new_entry->state = DSRTOS_SCHED_STATE_ACTIVE;
    new_entry->activation_count++;
    
    /* Complete switch */
    g_scheduler_core.switching.state = SWITCH_STATE_COMPLETING;
    g_scheduler_core.context.selection.last_switch_time = dsrtos_get_tick_count();
    g_scheduler_core.context.selection.switch_count++;
    
    /* Resume new scheduler */
    if (new_entry->plugin->resume != NULL) {
        (void)new_entry->plugin->resume(new_entry->context);
    }
    
    g_scheduler_core.switching.state = SWITCH_STATE_IDLE;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Update scheduler metrics
 */
static void update_scheduler_metrics(void)
{
    dsrtos_scheduler_metrics_t* metrics = &g_scheduler_core.context.metrics;
    dsrtos_scheduler_plugin_t* scheduler = g_scheduler_core.active_scheduler;
    
    if ((scheduler != NULL) && (scheduler->get_statistics != NULL)) {
        (void)scheduler->get_statistics(g_scheduler_core.active_context, metrics);
    }
}

/**
 * @brief Analyze workload patterns for adaptation
 */
static void analyze_workload_patterns(void)
{
    dsrtos_scheduler_context_t* ctx = &g_scheduler_core.context;
    
    /* Calculate CPU usage */
    if (ctx->metrics.total_scheduling_time_ns > 0U) {
        uint64_t total_time = ctx->metrics.total_scheduling_time_ns + 
                             ctx->metrics.total_idle_time_ns;
        if (total_time > 0U) {
            ctx->resources.cpu_usage_percent = 
                (uint8_t)((ctx->metrics.total_scheduling_time_ns * 100U) / total_time);
        }
    }
    
    /* Update stability counter */
    if (ctx->metrics.deadline_misses == 0U) {
        if (ctx->selection.stability_counter < UINT32_MAX) {
            ctx->selection.stability_counter++;
        }
    } else {
        ctx->selection.stability_counter = 0U;
    }
}

/**
 * @brief Calculate optimal scheduler based on current metrics
 * @return Recommended scheduler ID
 */
static uint16_t calculate_optimal_scheduler(void)
{
    dsrtos_scheduler_context_t* ctx = &g_scheduler_core.context;
    
    /* Decision matrix based on workload characteristics */
    
    /* High real-time requirements */
    if (ctx->rt_constraints.hard_deadlines > 0U) {
        if (ctx->rt_constraints.deadline_misses > 0U) {
            return DSRTOS_SCHED_ID_EDF;  /* Use EDF for deadline management */
        }
        return DSRTOS_SCHED_ID_RATE_MONOTONIC;
    }
    
    /* High CPU usage */
    if (ctx->resources.cpu_usage_percent > 80U) {
        if (ctx->ipc_stats.ipc_rate > 1000U) {
            return DSRTOS_SCHED_ID_ADAPTIVE;  /* Complex workload */
        }
        return DSRTOS_SCHED_ID_CFS;  /* Fair scheduling */
    }
    
    /* Resource contention */
    if (ctx->resources.resource_contention > 50U) {
        return DSRTOS_SCHED_ID_PRIORITY_INHERITANCE;
    }
    
    /* Low CPU usage - simple scheduling */
    if (ctx->resources.cpu_usage_percent < 30U) {
        return DSRTOS_SCHED_ID_ROUND_ROBIN;
    }
    
    /* Default to static priority */
    return DSRTOS_SCHED_ID_STATIC_PRIORITY;
}

/**
 * @brief Calculate plugin checksum
 * @param plugin Plugin to calculate checksum for
 * @return Calculated checksum
 */
static uint32_t calculate_plugin_checksum(const dsrtos_scheduler_plugin_t* plugin)
{
    const uint8_t* data = (const uint8_t*)plugin;
    uint32_t checksum = 0x12345678U;
    size_t size = sizeof(dsrtos_scheduler_plugin_t) - sizeof(uint32_t);
    
    for (size_t i = 0U; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
    }
    
    return checksum;
}
