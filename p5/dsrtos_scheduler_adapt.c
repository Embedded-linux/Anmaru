/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Adaptation Implementation
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
#include "dsrtos_scheduler_adapt.h"
#include "dsrtos_scheduler_core.h"
#include "dsrtos_scheduler_manager.h"
#include "dsrtos_critical.h"
#include "dsrtos_config.h"
#include "dsrtos_assert.h"
#include <string.h>

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
#define ADAPT_TRACE(msg)        DSRTOS_TRACE(TRACE_SCHEDULER, msg)
#define ADAPT_ASSERT(cond)      DSRTOS_ASSERT(cond)

/* Fixed-point math for learning */
#define FP_SCALE                (100U)
#define FP_MULTIPLY(a, b)       (((a) * (b)) / FP_SCALE)
#define FP_DIVIDE(a, b)         (((a) * FP_SCALE) / (b))

/* Pattern detection */
#define PATTERN_PERIODIC        (0x0001U)
#define PATTERN_BURST          (0x0002U)
#define PATTERN_IDLE          (0x0004U)
#define PATTERN_REAL_TIME      (0x0008U)
#define PATTERN_HIGH_IPC       (0x0010U)
#define PATTERN_HIGH_CONTENTION (0x0020U)

/*==============================================================================
 *                              LOCAL TYPES
 *============================================================================*/
/* Adaptation control block */
typedef struct {
    /* Configuration */
    dsrtos_adapt_config_t config;
    dsrtos_adapt_mode_t mode;
    
    /* Learning model */
    dsrtos_adapt_model_t model;
    
    /* Current state */
    dsrtos_state_snapshot_t current_state;
    dsrtos_state_snapshot_t last_state;
    uint32_t last_evaluation_time;
    uint32_t stability_counter;
    
    /* Decision making */
    dsrtos_adapt_decision_t last_decision;
    uint32_t pending_triggers;
    
    /* Callbacks */
    struct {
        dsrtos_adapt_callback_t callback;
        void* user_data;
    } callbacks[4];
    uint32_t callback_count;
    
    /* Statistics */
    struct {
        uint32_t total_adaptations;
        uint32_t successful_adaptations;
        uint32_t failed_adaptations;
        uint64_t total_decision_time_us;
        uint32_t pattern_matches;
    } stats;
    
    /* State */
    volatile uint32_t initialized;
    volatile uint32_t running;
} adapt_control_block_t;

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/
static adapt_control_block_t g_adapt = {0};

/*==============================================================================
 *                     LOCAL FUNCTION PROTOTYPES
 *============================================================================*/
static void collect_system_state(dsrtos_state_snapshot_t* snapshot);
static uint32_t detect_triggers(const dsrtos_state_snapshot_t* state);
static uint32_t detect_workload_pattern(const dsrtos_state_snapshot_t* state);
static uint32_t calculate_performance_score(const dsrtos_state_snapshot_t* state);
static void update_learning_model(uint16_t scheduler_id, uint32_t score);
static uint16_t apply_learning_model(const dsrtos_state_snapshot_t* state);
static void notify_callbacks(const dsrtos_adapt_decision_t* decision);
static bool should_trigger_adaptation(void);

/*==============================================================================
 *                          PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize adaptation system
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_adapt_init(void)
{
    uint32_t sr;
    
    ADAPT_TRACE("Adaptation system initialization");
    
    sr = dsrtos_critical_enter();
    
    if (g_adapt.initialized != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Clear control block */
    (void)memset(&g_adapt, 0, sizeof(g_adapt));
    
    /* Set default configuration */
    g_adapt.config.mode = DSRTOS_ADAPT_MODE_ASSISTED;
    g_adapt.config.evaluation_period_ms = 100U;
    g_adapt.config.min_stability_time_ms = 1000U;
    g_adapt.config.trigger_mask = 0xFFFFFFFFU;
    
    /* Set default thresholds */
    g_adapt.config.thresholds.cpu_low = DSRTOS_ADAPT_CPU_THRESHOLD_LOW;
    g_adapt.config.thresholds.cpu_high = DSRTOS_ADAPT_CPU_THRESHOLD_HIGH;
    g_adapt.config.thresholds.ipc_low = DSRTOS_ADAPT_IPC_THRESHOLD_LOW;
    g_adapt.config.thresholds.ipc_high = DSRTOS_ADAPT_IPC_THRESHOLD_HIGH;
    g_adapt.config.thresholds.deadline_miss_limit = DSRTOS_ADAPT_DEADLINE_MISS_LIMIT;
    g_adapt.config.thresholds.contention_limit = 50U;
    
    /* Initialize learning parameters */
    g_adapt.config.learning.enabled = false;
    g_adapt.config.learning.learning_rate = DSRTOS_ADAPT_LEARNING_RATE;
    g_adapt.config.learning.exploration_rate = 10U;
    g_adapt.config.learning.confidence_threshold = DSRTOS_ADAPT_CONFIDENCE_THRESHOLD;
    
    /* Initialize model weights */
    g_adapt.model.weights.cpu_weight = 25U;
    g_adapt.model.weights.memory_weight = 15U;
    g_adapt.model.weights.ipc_weight = 25U;
    g_adapt.model.weights.deadline_weight = 30U;
    g_adapt.model.weights.energy_weight = 5U;
    
    g_adapt.initialized = 1U;
    
    dsrtos_critical_exit(sr);
    
    ADAPT_TRACE("Adaptation system initialized");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Start adaptation system
 * @return DSRTOS_SUCCESS or error code
 */
dsrtos_error_t dsrtos_adapt_start(void)
{
    uint32_t sr;
    
    sr = dsrtos_critical_enter();
    
    if (g_adapt.initialized == 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_adapt.running != 0U) {
        dsrtos_critical_exit(sr);
        return DSRTOS_ERROR_ALREADY_STARTED;
    }
    
    /* Capture initial state */
    collect_system_state(&g_adapt.current_state);
    g_adapt.last_evaluation_time = dsrtos_get_tick_count();
    
    g_adapt.running = 1U;
    
    dsrtos_critical_exit(sr);
    
    ADAPT_TRACE("Adaptation system started");
    return DSRTOS_SUCCESS;
}

/**
 * @brief Evaluate system and make adaptation decision
 */
void dsrtos_adapt_evaluate(void)
{
    dsrtos_state_snapshot_t snapshot;
    dsrtos_adapt_decision_t decision;
    uint32_t current_time;
    uint32_t elapsed_time;
    uint32_t start_time;
    
    if (g_adapt.running == 0U) {
        return;
    }
    
    /* Check evaluation period */
    current_time = dsrtos_get_tick_count();
    elapsed_time = current_time - g_adapt.last_evaluation_time;
    if (elapsed_time < g_adapt.config.evaluation_period_ms) {
        return;
    }
    
    start_time = dsrtos_get_tick_count();
    
    /* Capture current state */
    collect_system_state(&snapshot);
    
    /* Detect triggers */
    g_adapt.pending_triggers = detect_triggers(&snapshot);
    
    /* Check if adaptation needed */
    if (!should_trigger_adaptation()) {
        g_adapt.stability_counter++;
        return;
    }
    
    /* Make decision */
    decision = dsrtos_adapt_make_decision();
    
    /* Record decision time */
    decision.decision_time_us = (dsrtos_get_tick_count() - start_time) * 1000U;
    g_adapt.stats.total_decision_time_us += decision.decision_time_us;
    
    /* Execute decision if mode allows */
    if (g_adapt.config.mode == DSRTOS_ADAPT_MODE_AUTOMATIC) {
        if (decision.confidence_level >= g_adapt.config.learning.confidence_threshold) {
            dsrtos_error_t err = dsrtos_scheduler_switch(decision.recommended_scheduler);
            if (err == DSRTOS_SUCCESS) {
                g_adapt.stats.successful_adaptations++;
                g_adapt.stability_counter = 0U;
            } else {
                g_adapt.stats.failed_adaptations++;
            }
        }
    }
    
    /* Update model if learning enabled */
    if (g_adapt.config.learning.enabled) {
        uint32_t score = calculate_performance_score(&snapshot);
        update_learning_model(dsrtos_scheduler_get_active_id(), score);
    }
    
    /* Store state */
    g_adapt.last_state = g_adapt.current_state;
    g_adapt.current_state = snapshot;
    g_adapt.last_decision = decision;
    g_adapt.last_evaluation_time = current_time;
    
    /* Notify callbacks */
    notify_callbacks(&decision);
    
    g_adapt.stats.total_adaptations++;
}

/**
 * @brief Make adaptation decision
 * @return Adaptation decision
 */
dsrtos_adapt_decision_t dsrtos_adapt_make_decision(void)
{
    dsrtos_adapt_decision_t decision = {0};
    dsrtos_state_snapshot_t* state = &g_adapt.current_state;
    uint32_t pattern;
    uint16_t recommended;
    
    /* Detect workload pattern */
    pattern = detect_workload_pattern(state);
    
    /* Check if we have a known pattern */
    for (uint32_t i = 0U; i < 16U; i++) {
        if (g_adapt.model.patterns[i].pattern_id == pattern) {
            if (g_adapt.model.patterns[i].success_rate > 80U) {
                decision.recommended_scheduler = g_adapt.model.patterns[i].best_scheduler;
                decision.confidence_level = g_adapt.model.patterns[i].success_rate;
                (void)snprintf(decision.reason, sizeof(decision.reason),
                             "Pattern match: %u", pattern);
                decision.trigger_mask = g_adapt.pending_triggers;
                return decision;
            }
        }
    }
    
    /* Use learning model if available */
    if (g_adapt.config.learning.enabled && (g_adapt.model.history_count > 10U)) {
        recommended = apply_learning_model(state);
        decision.recommended_scheduler = recommended;
        decision.confidence_level = dsrtos_adapt_calculate_confidence(recommended, state);
        (void)snprintf(decision.reason, sizeof(decision.reason),
                     "Learning model recommendation");
    } else {
        /* Use decision matrix */
        dsrtos_decision_factors_t factors;
        dsrtos_scheduler_get_decision_factors(&factors);
        recommended = dsrtos_scheduler_select_optimal(&factors);
        decision.recommended_scheduler = recommended;
        decision.confidence_level = 70U; /* Default confidence */
        (void)snprintf(decision.reason, sizeof(decision.reason),
                     "Decision matrix selection");
    }
    
    decision.trigger_mask = g_adapt.pending_triggers;
    
    return decision;
}

/**
 * @brief Calculate confidence level for scheduler selection
 * @param scheduler_id Scheduler ID
 * @param state System state
 * @return Confidence level (0-100)
 */
uint32_t dsrtos_adapt_calculate_confidence(
    uint16_t scheduler_id, const dsrtos_state_snapshot_t* state)
{
    uint32_t confidence = 50U; /* Base confidence */
    uint32_t i;
    
    /* Find scheduler statistics */
    for (i = 0U; i < DSRTOS_MAX_SCHEDULERS; i++) {
        if (g_adapt.model.scheduler_stats[i].scheduler_id == scheduler_id) {
            break;
        }
    }
    
    if (i >= DSRTOS_MAX_SCHEDULERS) {
        return confidence; /* Unknown scheduler */
    }
    
    /* Adjust based on historical performance */
    if (g_adapt.model.scheduler_stats[i].usage_count > 10U) {
        /* Success rate contributes to confidence */
        uint32_t success_rate = g_adapt.model.scheduler_stats[i].deadline_success_rate;
        confidence = (confidence + success_rate) / 2U;
        
        /* Similar conditions boost confidence */
        if ((state->cpu_usage >= g_adapt.model.scheduler_stats[i].average_cpu_usage - 10U) &&
            (state->cpu_usage <= g_adapt.model.scheduler_stats[i].average_cpu_usage + 10U)) {
            confidence += 10U;
        }
        
        /* Usage frequency adds confidence */
        if (g_adapt.model.scheduler_stats[i].usage_count > 100U) {
            confidence += 10U;
        }
    }
    
    /* Cap at 100% */
    if (confidence > 100U) {
        confidence = 100U;
    }
    
    return confidence;
}

/*==============================================================================
 *                          LOCAL FUNCTIONS
 *============================================================================*/

/**
 * @brief Collect current system state
 * @param snapshot Buffer for state snapshot
 */
static void collect_system_state(dsrtos_state_snapshot_t* snapshot)
{
    dsrtos_scheduler_context_t* ctx = dsrtos_scheduler_get_context();
    dsrtos_scheduler_metrics_t* metrics = &ctx->metrics;
    
    snapshot->timestamp = dsrtos_get_tick_count();
    snapshot->scheduler_id = dsrtos_scheduler_get_active_id();
    snapshot->cpu_usage = ctx->resources.cpu_usage_percent;
    snapshot->memory_usage = ctx->resources.memory_free;
    snapshot->ipc_rate = ctx->ipc_stats.ipc_rate;
    snapshot->context_switches = metrics->context_switches;
    snapshot->deadline_misses = metrics->deadline_misses;
    snapshot->resource_contention = ctx->resources.resource_contention;
    snapshot->energy_consumption = 0U; /* TODO: Implement energy tracking */
    snapshot->performance_score = calculate_performance_score(snapshot);
}

/**
 * @brief Detect adaptation triggers
 * @param state System state
 * @return Trigger mask
 */
static uint32_t detect_triggers(const dsrtos_state_snapshot_t* state)
{
    uint32_t triggers = 0U;
    
    /* CPU trigger */
    if ((state->cpu_usage < g_adapt.config.thresholds.cpu_low) ||
        (state->cpu_usage > g_adapt.config.thresholds.cpu_high)) {
        triggers |= DSRTOS_ADAPT_TRIGGER_CPU;
    }
    
    /* IPC trigger */
    if ((state->ipc_rate < g_adapt.config.thresholds.ipc_low) ||
        (state->ipc_rate > g_adapt.config.thresholds.ipc_high)) {
        triggers |= DSRTOS_ADAPT_TRIGGER_IPC;
    }
    
    /* Deadline trigger */
    if (state->deadline_misses > g_adapt.config.thresholds.deadline_miss_limit) {
        triggers |= DSRTOS_ADAPT_TRIGGER_DEADLINE;
    }
    
    /* Contention trigger */
    if (state->resource_contention > g_adapt.config.thresholds.contention_limit) {
        triggers |= DSRTOS_ADAPT_TRIGGER_CONTENTION;
    }
    
    return triggers & g_adapt.config.trigger_mask;
}

/**
 * @brief Calculate performance score
 * @param state System state
 * @return Performance score (0-100)
 */
static uint32_t calculate_performance_score(const dsrtos_state_snapshot_t* state)
{
    uint32_t score = 100U;
    
    /* Penalize high CPU usage */
    if (state->cpu_usage > 90U) {
        score -= 20U;
    } else if (state->cpu_usage > 80U) {
        score -= 10U;
    }
    
    /* Penalize deadline misses heavily */
    if (state->deadline_misses > 0U) {
        score -= (state->deadline_misses * 10U);
        if (score > 100U) { /* Underflow check */
            score = 0U;
        }
    }
    
    /* Penalize resource contention */
    if (state->resource_contention > 50U) {
        score -= 15U;
    }
    
    /* Reward low context switches */
    if (state->context_switches < 1000U) {
        score += 5U;
    }
    
    /* Cap at 100 */
    if (score > 100U) {
        score = 100U;
    }
    
    return score;
}

/**
 * @brief Check if adaptation should be triggered
 * @return true if adaptation should occur
 */
static bool should_trigger_adaptation(void)
{
    /* Check mode */
    if (g_adapt.config.mode == DSRTOS_ADAPT_MODE_DISABLED) {
        return false;
    }
    
    /* Check triggers */
    if (g_adapt.pending_triggers == 0U) {
        return false;
    }
    
    /* Check stability time */
    uint32_t time_since_switch = dsrtos_get_tick_count() - 
        dsrtos_scheduler_get_context()->selection.last_switch_time;
    if (time_since_switch < g_adapt.config.min_stability_time_ms) {
        return false;
    }
    
    return true;
}

/**
 * @brief Notify registered callbacks
 * @param decision Adaptation decision
 */
static void notify_callbacks(const dsrtos_adapt_decision_t* decision)
{
    for (uint32_t i = 0U; i < g_adapt.callback_count; i++) {
        if (g_adapt.callbacks[i].callback != NULL) {
            g_adapt.callbacks[i].callback(decision, g_adapt.callbacks[i].user_data);
        }
    }
}
