/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Dynamic Scheduler Manager Implementation (Complete)
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
#include "dsrtos_scheduler_manager.h"
#include "dsrtos_scheduler_core.h"
#include "dsrtos_critical.h"
#include "dsrtos_config.h"
#include "dsrtos_assert.h"
#include "dsrtos_memory.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define MANAGER_TRACE(msg)      DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define MANAGER_ASSERT(cond)    DSRTOS_ASSERT(cond)

/* Decision matrix indices */
#define CPU_LEVEL_IDLE         (0U)
#define CPU_LEVEL_LOW          (1U)
#define CPU_LEVEL_MEDIUM       (2U)
#define CPU_LEVEL_HIGH         (3U)
#define CPU_LEVEL_CRITICAL     (4U)

#define IPC_LEVEL_NONE         (0U)
#define IPC_LEVEL_LOW          (1U)
#define IPC_LEVEL_MEDIUM       (2U)
#define IPC_LEVEL_HIGH         (3U)
#define IPC_LEVEL_EXTREME      (4U)

/* Transition safety checks */
#define MAX_TRANSITION_TIME_MS  (100U)
#define MIN_STABLE_TIME_MS      (50U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Manager control block */
typedef struct {
    /* Policy */
    dsrtos_selection_policy_t policy;
    
    /* Current state */
    dsrtos_workload_type_t current_workload;
    dsrtos_decision_factors_t factors;
    
    /* Transition management */
    dsrtos_transition_state_t transition;
    volatile uint32_t transitioning;
    uint32_t transition_lock;
    
    /* Statistics */
    struct {
        uint32_t total_decisions;
        uint32_t switch_requests;
        uint32_t successful_switches;
        uint32_t failed_switches;
        uint64_t total_decision_time_ns;
        uint32_t max_switch_overhead_us;
        uint32_t rollback_count;
    } stats;
    
    /* Decision matrix */
    uint16_t decision_matrix[DSRTOS_CPU_LEVELS][DSRTOS_IPC_LEVELS];
    
    /* Scheduler compatibility matrix */
    bool compatibility_matrix[DSRTOS_MAX_SCHEDULERS][DSRTOS_MAX_SCHEDULERS];
    
    /* State */
    volatile uint32_t initialized;
} scheduler_manager_cb_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static scheduler_manager_cb_t g_manager = {0};

/* Default decision matrix */
static const uint16_t g_default_decision_matrix[DSRTOS_CPU_LEVELS][DSRTOS_IPC_LEVELS] = {
    /* IPC:     NONE,  LOW,   MED,   HIGH,  EXTREME */
    /* IDLE */  {DSRTOS_SCHED_ID_ROUND_ROBIN, DSRTOS_SCHED_ID_ROUND_ROBIN, 
                 DSRTOS_SCHED_ID_STATIC_PRIORITY, DSRTOS_SCHED_ID_STATIC_PRIORITY, 
                 DSRTOS_SCHED_ID_PRIORITY_INHERITANCE},
    /* LOW */   {DSRTOS_SCHED_ID_ROUND_ROBIN, DSRTOS_SCHED_ID_STATIC_PRIORITY, 
                 DSRTOS_SCHED_ID_STATIC_PRIORITY, DSRTOS_SCHED_ID_CFS, 
                 DSRTOS_SCHED_ID_ADAPTIVE},
    /* MED */   {DSRTOS_SCHED_ID_STATIC_PRIORITY, DSRTOS_SCHED_ID_STATIC_PRIORITY, 
                 DSRTOS_SCHED_ID_CFS, DSRTOS_SCHED_ID_CFS, 
                 DSRTOS_SCHED_ID_ADAPTIVE},
    /* HIGH */  {DSRTOS_SCHED_ID_EDF, DSRTOS_SCHED_ID_EDF, 
                 DSRTOS_SCHED_ID_CFS, DSRTOS_SCHED_ID_ADAPTIVE, 
                 DSRTOS_SCHED_ID_ADAPTIVE},
    /* CRIT */  {DSRTOS_SCHED_ID_EDF, DSRTOS_SCHED_ID_EDF, 
                 DSRTOS_SCHED_ID_ADAPTIVE, DSRTOS_SCHED_ID_ADAPTIVE, 
                 DSRTOS_SCHED_ID_ADAPTIVE}
};

/*==============================================================================
 *                     LOCAL FUNCTION PROTOTYPES
 *============================================================================*/
static void collect_decision_factors(dsrtos_decision_factors_t* factors);
static uint8_t calculate_cpu_level(uint8_t cpu_usage);
static uint8_t calculate_ipc_level(uint32_t ipc_rate);
static dsrtos_workload_type_t classify_workload(
    const dsrtos_decision_factors_t* factors);
static uint32_t calculate_decision_score(
    uint16_t scheduler_id, const dsrtos_decision_factors_t* factors);
static bool validate_transition(uint16_t from_id, uint16_t to_id);
static dsrtos_error_t prepare_task_migration(uint16_t from_id, uint16_t to_id);
static dsrtos_error_t execute_task_migration(void);
static void rollback_task_migration(void);
static void update_compatibility_matrix(void);

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize the scheduler manager
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_manager_init(void)
{
    uint32_t sr;
    
    MANAGER_TRACE("Scheduler manager initialization");
    
    sr = dsrtos_critical_enter();
    
    if (g_manager.initialized != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Clear control block */
    (void)memset(&g_manager, 0, sizeof(g_manager));
    
    /* Set default policy */
    g_manager.policy.cpu_threshold_high = DSRTOS_HIGH_CPU_THRESHOLD;
    g_manager.policy.cpu_threshold_low = DSRTOS_LOW_CPU_THRESHOLD;
    g_manager.policy.ipc_threshold_high = DSRTOS_HIGH_IPC_RATE;
    g_manager.policy.contention_threshold = DSRTOS_HIGH_CONTENTION;
    g_manager.policy.deadline_miss_threshold = 5U;
    g_manager.policy.switch_hysteresis_ms = DSRTOS_SWITCH_HYSTERESIS_MS;
    g_manager.policy.stability_threshold = 10U;
    
    /* Set default weights */
    g_manager.policy.cpu_weight = 30U;
    g_manager.policy.ipc_weight = 25U;
    g_manager.policy.deadline_weight = 35U;
    g_manager.policy.contention_weight = 10U;
    
    /* Set preferred schedulers */
    g_manager.policy.preferred_rt_scheduler = DSRTOS_SCHED_ID_EDF;
    g_manager.policy.preferred_general_scheduler = DSRTOS_SCHED_ID_CFS;
    g_manager.policy.fallback_scheduler = DSRTOS_SCHED_ID_ROUND_ROBIN;
    
    /* Copy default decision matrix */
    (void)memcpy(g_manager.decision_matrix, g_default_decision_matrix,
                 sizeof(g_default_decision_matrix));
    
    /* Initialize compatibility matrix */
    update_compatibility_matrix();
    
    g_manager.initialized = 1U;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Scheduler manager initialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Deinitialize the scheduler manager
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_manager_deinit(void)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    if (g_manager.initialized == 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* Check if transition in progress */
    if (g_manager.transitioning != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_BUSY;
    }
    
    /* Clear control block */
    (void)memset(&g_manager, 0, sizeof(g_manager));
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Scheduler manager deinitialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Set scheduler selection policy
 * @param policy Policy to set
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_set_policy(const dsrtos_selection_policy_t* policy)
{
    uint32_t sr;
    
    if (policy == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    sr = dsrtos_critical_enter();
    
    /* Validate policy parameters */
    if ((policy->cpu_threshold_high <= policy->cpu_threshold_low) ||
        (policy->cpu_threshold_high > 100U)) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy new policy */
    g_manager.policy = *policy;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Policy updated");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get current scheduler selection policy
 * @param policy Buffer for policy
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_get_policy(dsrtos_selection_policy_t* policy)
{
    if (policy == NULL) {
        return DSRTOS_ERROR_INVALID_PARAMETER;
    }
    
    *policy = g_manager.policy;
    return DSRTOS_SUCCESS;
}

/**
 * @brief Reset policy to defaults
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_reset_policy(void)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    /* Reset to defaults */
    g_manager.policy.cpu_threshold_high = DSRTOS_HIGH_CPU_THRESHOLD;
    g_manager.policy.cpu_threshold_low = DSRTOS_LOW_CPU_THRESHOLD;
    g_manager.policy.ipc_threshold_high = DSRTOS_HIGH_IPC_RATE;
    g_manager.policy.contention_threshold = DSRTOS_HIGH_CONTENTION;
    g_manager.policy.deadline_miss_threshold = 5U;
    g_manager.policy.switch_hysteresis_ms = DSRTOS_SWITCH_HYSTERESIS_MS;
    g_manager.policy.stability_threshold = 10U;
    
    dsrtos_critical_exit(sr);
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Analyze current workload type
 * @return Workload classification
 */
dsrtos_workload_type_t dsrtos_scheduler_analyze_workload(void)
{
    dsrtos_decision_factors_t factors;
    
    /* Collect current factors */
    collect_decision_factors(&factors);
    
    /* Classify workload */
    g_manager.current_workload = classify_workload(&factors);
    
    return g_manager.current_workload;
}

/**
 * @brief Get current decision factors
 * @param factors Buffer to store factors
 */
void dsrtos_scheduler_get_decision_factors(dsrtos_decision_factors_t* factors)
{
    if (factors != NULL) {
        collect_decision_factors(factors);
        *factors = g_manager.factors;
    }
}

/**
 * @brief Select optimal scheduler based on factors
 * @param factors Decision factors
 * @return Recommended scheduler ID
 */
uint16_t dsrtos_scheduler_select_optimal(const dsrtos_decision_factors_t* factors)
{
    uint8_t cpu_level;
    uint8_t ipc_level;
    uint16_t matrix_selection;
    uint32_t best_score = 0U;
    uint16_t best_scheduler = DSRTOS_SCHED_ID_ROUND_ROBIN;
    
    g_manager.stats.total_decisions++;
    
    /* Use provided factors or collect current */
    if (factors == NULL) {
        collect_decision_factors(&g_manager.factors);
        factors = &g_manager.factors;
    }
    
    /* Check for hard real-time requirements */
    if ((factors->deadline_miss_rate > g_manager.policy.deadline_miss_threshold) ||
        (factors->worst_case_latency > 1000U)) {
        return g_manager.policy.preferred_rt_scheduler;
    }
    
    /* Check for high contention */
    if (factors->resource_contention > g_manager.policy.contention_threshold) {
        return DSRTOS_SCHED_ID_PRIORITY_INHERITANCE;
    }
    
    /* Use decision matrix */
    cpu_level = calculate_cpu_level(factors->cpu_usage);
    ipc_level = calculate_ipc_level(factors->ipc_rate);
    matrix_selection = g_manager.decision_matrix[cpu_level][ipc_level];
    
    /* Validate matrix selection with scoring */
    for (uint16_t id = DSRTOS_SCHED_ID_ROUND_ROBIN; 
         id <= DSRTOS_SCHED_ID_ADAPTIVE; id++) {
        uint32_t score = calculate_decision_score(id, factors);
        if (score > best_score) {
            best_score = score;
            best_scheduler = id;
        }
    }
    
    /* Use matrix selection if close to best score */
    uint32_t matrix_score = calculate_decision_score(matrix_selection, factors);
    if ((matrix_score * 100U) >= (best_score * 90U)) {
        return matrix_selection;
    }
    
    return best_scheduler;
}

/**
 * @brief Check if scheduler switch is needed
 * @return true if switch recommended
 */
bool dsrtos_scheduler_should_switch(void)
{
    dsrtos_scheduler_context_t* ctx;
    uint16_t current_id;
    uint16_t recommended_id;
    uint32_t current_time;
    uint32_t time_since_switch;
    
    /* Get current context */
    ctx = dsrtos_scheduler_get_context();
    if (ctx == NULL) {
        return false;
    }
    
    /* Check if already switching */
    if (g_manager.transitioning != 0U) {
        return false;
    }
    
    /* Get current scheduler */
    current_id = dsrtos_scheduler_get_active_id();
    if (current_id == DSRTOS_SCHED_ID_NONE) {
        return false;
    }
    
    /* Collect factors */
    collect_decision_factors(&g_manager.factors);
    
    /* Get recommendation */
    recommended_id = dsrtos_scheduler_select_optimal(&g_manager.factors);
    
    /* Check if different */
    if (recommended_id == current_id) {
        return false;
    }
    
    /* Check hysteresis */
    current_time = dsrtos_get_tick_count();
    time_since_switch = current_time - ctx->selection.last_switch_time;
    if (time_since_switch < g_manager.policy.switch_hysteresis_ms) {
        return false;
    }
    
    /* Check stability */
    if (ctx->selection.stability_counter < g_manager.policy.stability_threshold) {
        return false;
    }
    
    return true;
}

/**
 * @brief Request scheduler switch
 * @param target_id Target scheduler ID
 * @param reason Switch reason
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_request_switch(uint16_t target_id, uint32_t reason)
{
    dsrtos_scheduler_switch_request_t request;
    
    g_manager.stats.switch_requests++;
    
    /* Build request */
    request.target_scheduler_id = target_id;
    request.switch_reason = reason;
    request.switch_time_ms = 0U; /* Immediate */
    request.switch_context = NULL;
    
    /* Submit request */
    return dsrtos_scheduler_switch_request(&request);
}

/**
 * @brief Begin scheduler transition
 * @param from_id Source scheduler ID
 * @param to_id Target scheduler ID
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_begin_transition(
    uint16_t from_id, uint16_t to_id)
{
    uint32_t sr;
    dsrtos_error_t err;
    
    MANAGER_TRACE("Beginning scheduler transition");
    
    sr = dsrtos_critical_enter();
    
    /* Check if already transitioning */
    if (g_manager.transitioning != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_BUSY;
    }
    
    /* Validate transition */
    if (!validate_transition(from_id, to_id)) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Initialize transition state */
    g_manager.transition.from_scheduler_id = from_id;
    g_manager.transition.to_scheduler_id = to_id;
    g_manager.transition.transition_start_time = dsrtos_get_tick_count();
    g_manager.transition.expected_duration_ms = 10U;
    g_manager.transition.tasks_to_migrate = 0U;
    g_manager.transition.tasks_migrated = 0U;
    g_manager.transition.migration_list = NULL;
    g_manager.transition.sync_point_reached = 0U;
    g_manager.transition.rollback_possible = 1U;
    
    /* Prepare task migration */
    err = prepare_task_migration(from_id, to_id);
    if (err != DSRTOS_SUCCESS) {
        dsrtos_critical_exit(sr);
        return err;
    }
    
    /* Mark as transitioning */
    g_manager.transitioning = 1U;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Transition started");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Migrate tasks between schedulers
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_migrate_tasks(void)
{
    uint32_t sr;
    dsrtos_error_t err;
    
    MANAGER_TRACE("Migrating tasks");
    
    sr = dsrtos_critical_enter();
    
    /* Check state */
    if (g_manager.transitioning == 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Execute migration */
    err = execute_task_migration();
    if (err != DSRTOS_SUCCESS) {
        /* Rollback if possible */
        if (g_manager.transition.rollback_possible != 0U) {
            rollback_task_migration();
            g_manager.stats.rollback_count++;
        }
        g_manager.transitioning = 0U;
        dsrtos_critical_exit(sr);
        return err;
    }
    
    /* Mark sync point */
    g_manager.transition.sync_point_reached = 1U;
    g_manager.transition.rollback_possible = 0U;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Tasks migrated successfully");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Complete scheduler transition
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_complete_transition(void)
{
    uint32_t sr;
    uint32_t transition_time;
    
    MANAGER_TRACE("Completing transition");
    
    sr = dsrtos_critical_enter();
    
    /* Check state */
    if ((g_manager.transitioning == 0U) ||
        (g_manager.transition.sync_point_reached == 0U)) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Calculate transition time */
    transition_time = dsrtos_get_tick_count() - 
                     g_manager.transition.transition_start_time;
    
    /* Update statistics */
    if (transition_time > g_manager.stats.max_switch_overhead_us) {
        g_manager.stats.max_switch_overhead_us = transition_time;
    }
    g_manager.stats.successful_switches++;
    
    /* Clear transition state */
    (void)memset(&g_manager.transition, 0, sizeof(g_manager.transition));
    g_manager.transitioning = 0U;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Transition completed");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Rollback failed transition
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_scheduler_rollback_transition(void)
{
    uint32_t sr;
    
    MANAGER_TRACE("Rolling back transition");
    
    sr = dsrtos_critical_enter();
    
    /* Check if rollback possible */
    if ((g_manager.transitioning == 0U) ||
        (g_manager.transition.rollback_possible == 0U)) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Execute rollback */
    rollback_task_migration();
    
    /* Update statistics */
    g_manager.stats.failed_switches++;
    g_manager.stats.rollback_count++;
    
    /* Clear transition state */
    (void)memset(&g_manager.transition, 0, sizeof(g_manager.transition));
    g_manager.transitioning = 0U;
    
    dsrtos_critical_exit(sr);
    
    MANAGER_TRACE("Transition rolled back");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get transition state
 * @param state Buffer for state
 */
void dsrtos_scheduler_get_transition_state(dsrtos_transition_state_t* state)
{
    if (state != NULL) {
        *state = g_manager.transition;
    }
}

/**
 * @brief Update scheduler metrics
 */
void dsrtos_scheduler_update_metrics(void)
{
    dsrtos_scheduler_context_t* ctx;
    
    /* Get context */
    ctx = dsrtos_scheduler_get_context();
    if (ctx == NULL) {
        return;
    }
    
    /* Collect current factors */
    collect_decision_factors(&g_manager.factors);
    
    /* Update workload classification */
    g_manager.current_workload = classify_workload(&g_manager.factors);
}

/**
 * @brief Get switch overhead
 * @return Switch overhead in microseconds
 */
uint32_t dsrtos_scheduler_get_switch_overhead(void)
{
    return g_manager.stats.max_switch_overhead_us;
}

/**
 * @brief Get decision time
 * @return Average decision time in nanoseconds
 */
uint32_t dsrtos_scheduler_get_decision_time(void)
{
    if (g_manager.stats.total_decisions > 0U) {
        return (uint32_t)(g_manager.stats.total_decision_time_ns / 
                         g_manager.stats.total_decisions);
    }
    return 0U;
}

/**
 * @brief Validate scheduler switch
 * @param from_id Source scheduler
 * @param to_id Target scheduler
 * @return true if valid
 */
bool dsrtos_scheduler_validate_switch(uint16_t from_id, uint16_t to_id)
{
    /* Check scheduler IDs */
    if ((from_id >= DSRTOS_MAX_SCHEDULERS) ||
        (to_id >= DSRTOS_MAX_SCHEDULERS)) {
        return false;
    }
    
    /* Check compatibility */
    if (!g_manager.compatibility_matrix[from_id][to_id]) {
        return false;
    }
    
    /* Check if schedulers are registered */
    if ((dsrtos_scheduler_get(from_id) == NULL) ||
        (dsrtos_scheduler_get(to_id) == NULL)) {
        return false;
    }
    
    return true;
}

/**
 * @brief Check scheduler compatibility with workload
 * @param scheduler_id Scheduler ID
 * @param workload Workload type
 * @return true if compatible
 */
bool dsrtos_scheduler_is_compatible(
    uint16_t scheduler_id, dsrtos_workload_type_t workload)
{
    /* Check scheduler capabilities against workload */
    switch (workload) {
        case DSRTOS_WORKLOAD_IDLE:
            return (scheduler_id == DSRTOS_SCHED_ID_ROUND_ROBIN);
            
        case DSRTOS_WORKLOAD_PERIODIC:
            return (scheduler_id == DSRTOS_SCHED_ID_RATE_MONOTONIC) ||
                   (scheduler_id == DSRTOS_SCHED_ID_EDF);
            
        case DSRTOS_WORKLOAD_REAL_TIME:
            return (scheduler_id == DSRTOS_SCHED_ID_EDF) ||
                   (scheduler_id == DSRTOS_SCHED_ID_DEADLINE_MONOTONIC);
            
        case DSRTOS_WORKLOAD_INTERACTIVE:
            return (scheduler_id == DSRTOS_SCHED_ID_CFS) ||
                   (scheduler_id == DSRTOS_SCHED_ID_STATIC_PRIORITY);
            
        case DSRTOS_WORKLOAD_ADAPTIVE:
            return (scheduler_id == DSRTOS_SCHED_ID_ADAPTIVE);
            
        default:
            return true; /* Default: all schedulers compatible */
    }
}

/*==============================================================================
 *                          LOCAL FUNCTIONS
 *============================================================================*/

/**
 * @brief Collect decision factors from system
 * @param factors Buffer for factors
 */
static void collect_decision_factors(dsrtos_decision_factors_t* factors)
{
    dsrtos_scheduler_context_t* ctx;
    
    MANAGER_ASSERT(factors != NULL);
    
    /* Get scheduler context */
    ctx = dsrtos_scheduler_get_context();
    if (ctx == NULL) {
        return;
    }
    
    /* CPU metrics */
    factors->cpu_usage = ctx->resources.cpu_usage_percent;
    factors->cpu_variance = 0U; /* TODO: Calculate variance */
    factors->context_switch_rate = ctx->metrics.context_switches;
    
    /* Memory metrics */
    factors->memory_pressure = 100U - (ctx->resources.memory_free * 100U / 
                                       (192U * 1024U)); /* STM32F4 SRAM size */
    factors->fragmentation = ctx->resources.memory_fragmentation;
    
    /* IPC metrics */
    factors->ipc_rate = ctx->ipc_stats.ipc_rate;
    factors->ipc_latency = ctx->ipc_stats.ipc_blocking_time;
    factors->message_queue_depth = ctx->ipc_stats.ipc_queue_depth;
    
    /* Real-time metrics */
    factors->deadline_miss_rate = ctx->metrics.deadline_misses;
    factors->jitter = 0U; /* TODO: Calculate jitter */
    factors->worst_case_latency = ctx->metrics.max_decision_time_ns / 1000U;
    
    /* Resource metrics */
    factors->resource_contention = ctx->resources.resource_contention;
    factors->priority_inversions = ctx->metrics.priority_inversions;
    
    /* Task metrics */
    factors->task_count = ctx->ready_task_count;
    factors->ready_tasks = ctx->ready_task_count;
    factors->blocked_tasks = 0U; /* TODO: Track blocked tasks */
    
    /* Update manager's copy */
    g_manager.factors = *factors;
}

/**
 * @brief Calculate CPU usage level
 * @param cpu_usage CPU usage percentage
 * @return CPU level index
 */
static uint8_t calculate_cpu_level(uint8_t cpu_usage)
{
    if (cpu_usage < 20U) {
        return CPU_LEVEL_IDLE;
    } else if (cpu_usage < 40U) {
        return CPU_LEVEL_LOW;
    } else if (cpu_usage < 60U) {
        return CPU_LEVEL_MEDIUM;
    } else if (cpu_usage < 80U) {
        return CPU_LEVEL_HIGH;
    } else {
        return CPU_LEVEL_CRITICAL;
    }
}

/**
 * @brief Calculate IPC rate level
 * @param ipc_rate IPC messages per second
 * @return IPC level index
 */
static uint8_t calculate_ipc_level(uint32_t ipc_rate)
{
    if (ipc_rate == 0U) {
        return IPC_LEVEL_NONE;
    } else if (ipc_rate < 100U) {
        return IPC_LEVEL_LOW;
    } else if (ipc_rate < 500U) {
        return IPC_LEVEL_MEDIUM;
    } else if (ipc_rate < 1000U) {
        return IPC_LEVEL_HIGH;
    } else {
        return IPC_LEVEL_EXTREME;
    }
}

/**
 * @brief Classify workload type
 * @param factors Decision factors
 * @return Workload classification
 */
static dsrtos_workload_type_t classify_workload(
    const dsrtos_decision_factors_t* factors)
{
    /* Idle workload */
    if (factors->cpu_usage < 10U) {
        return DSRTOS_WORKLOAD_IDLE;
    }
    
    /* Real-time workload */
    if ((factors->deadline_miss_rate > 0U) || 
        (factors->worst_case_latency > 100U)) {
        return DSRTOS_WORKLOAD_REAL_TIME;
    }
    
    /* Interactive workload */
    if ((factors->ipc_rate > 500U) && (factors->cpu_usage < 60U)) {
        return DSRTOS_WORKLOAD_INTERACTIVE;
    }
    
    /* Batch workload */
    if ((factors->cpu_usage > 70U) && (factors->ipc_rate < 100U)) {
        return DSRTOS_WORKLOAD_BATCH;
    }
    
    /* Adaptive workload */
    if (factors->cpu_variance > 30U) {
        return DSRTOS_WORKLOAD_ADAPTIVE;
    }
    
    /* Default: mixed workload */
    return DSRTOS_WORKLOAD_MIXED;
}

/**
 * @brief Calculate scheduler score for given factors
 * @param scheduler_id Scheduler to score
 * @param factors Decision factors
 * @return Score (0-100)
 */
static uint32_t calculate_decision_score(
    uint16_t scheduler_id, const dsrtos_decision_factors_t* factors)
{
    uint32_t score = 50U; /* Base score */
    
    /* Score based on scheduler characteristics */
    switch (scheduler_id) {
        case DSRTOS_SCHED_ID_ROUND_ROBIN:
            /* Good for low CPU, fair distribution */
            if (factors->cpu_usage < 30U) {
                score += 20U;
            }
            if (factors->priority_inversions == 0U) {
                score += 10U;
            }
            break;
            
        case DSRTOS_SCHED_ID_STATIC_PRIORITY:
            /* Good for mixed priorities */
            if ((factors->cpu_usage > 30U) && (factors->cpu_usage < 70U)) {
                score += 20U;
            }
            if (factors->ready_tasks > 5U) {
                score += 10U;
            }
            break;
            
        case DSRTOS_SCHED_ID_EDF:
            /* Best for hard real-time */
            if (factors->deadline_miss_rate > 0U) {
                score += 40U;
            }
            if (factors->worst_case_latency > 100U) {
                score += 20U;
            }
            break;
            
        case DSRTOS_SCHED_ID_CFS:
            /* Good for interactive/fairness */
            if ((factors->ipc_rate > 500U) || (factors->task_count > 20U)) {
                score += 30U;
            }
            break;
            
        case DSRTOS_SCHED_ID_PRIORITY_INHERITANCE:
            /* Best for resource contention */
            if (factors->resource_contention > 50U) {
                score += 40U;
            }
            if (factors->priority_inversions > 0U) {
                score += 30U;
            }
            break;
            
        case DSRTOS_SCHED_ID_ADAPTIVE:
            /* Good for dynamic workloads */
            if (factors->cpu_variance > 30U) {
                score += 30U;
            }
            break;
            
        default:
            break;
    }
    
    /* Apply policy weights */
    score = (score * g_manager.policy.cpu_weight * factors->cpu_usage) / 10000U +
            (score * g_manager.policy.ipc_weight * 
             (factors->ipc_rate > 0U ? 100U : 0U)) / 10000U +
            (score * g_manager.policy.deadline_weight * 
             (factors->deadline_miss_rate > 0U ? 100U : 0U)) / 10000U +
            (score * g_manager.policy.contention_weight * 
             factors->resource_contention) / 10000U;
    
    /* Cap at 100 */
    if (score > 100U) {
        score = 100U;
    }
    
    return score;
}

/**
 * @brief Validate transition between schedulers
 * @param from_id Source scheduler
 * @param to_id Target scheduler
 * @return true if valid
 */
static bool validate_transition(uint16_t from_id, uint16_t to_id)
{
    /* Same scheduler - no transition needed */
    if (from_id == to_id) {
        return false;
    }
    
    /* Check basic validity */
    return dsrtos_scheduler_validate_switch(from_id, to_id);
}

/**
 * @brief Prepare for task migration
 * @param from_id Source scheduler
 * @param to_id Target scheduler
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t prepare_task_migration(uint16_t from_id, uint16_t to_id)
{
    dsrtos_scheduler_context_t* ctx;
    dsrtos_tcb_t* task;
    uint32_t count = 0U;
    
    /* Get context */
    ctx = dsrtos_scheduler_get_context();
    if (ctx == NULL) {
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Count tasks to migrate */
    task = ctx->ready_queue_head;
    while (task != NULL) {
        count++;
        task = task->next;
    }
    
    /* Store migration info */
    g_manager.transition.tasks_to_migrate = count;
    g_manager.transition.migration_list = ctx->ready_queue_head;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Execute task migration
 * @return DSRTOS_SUCCESS or error code
 */
static dsrtos_error_t execute_task_migration(void)
{
    dsrtos_tcb_t* task;
    dsrtos_tcb_t* next;
    uint32_t migrated = 0U;
    
    /* Get task list */
    task = g_manager.transition.migration_list;
    
    /* Migrate each task */
    while (task != NULL) {
        next = task->next;
        
        /* TODO: Actual migration logic here */
        /* This would involve removing from old scheduler queues */
        /* and adding to new scheduler queues */
        
        migrated++;
        task = next;
    }
    
    /* Update count */
    g_manager.transition.tasks_migrated = migrated;
    
    /* Verify all tasks migrated */
    if (migrated != g_manager.transition.tasks_to_migrate) {
        return DSRTOS_ERROR_INCOMPLETE;
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Rollback task migration
 */
static void rollback_task_migration(void)
{
    /* TODO: Implement rollback logic */
    /* This would restore tasks to original scheduler */
    
    MANAGER_TRACE("Migration rolled back");
}

/**
 * @brief Update scheduler compatibility matrix
 */
static void update_compatibility_matrix(void)
{
    /* Initialize all as compatible by default */
    for (uint32_t i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        for (uint32_t j = 0U; j < DSRTOS_MAX_SCHEDULERS; j++) {
            g_manager.compatibility_matrix[i][j] = true;
        }
    }
    
    /* Define incompatible transitions */
    /* Example: Cannot switch directly from Rate Monotonic to CFS */
    g_manager.compatibility_matrix[DSRTOS_SCHED_ID_RATE_MONOTONIC]
                                  [DSRTOS_SCHED_ID_CFS] = false;
    g_manager.compatibility_matrix[DSRTOS_SCHED_ID_CFS]
                                  [DSRTOS_SCHED_ID_RATE_MONOTONIC] = false;
}
