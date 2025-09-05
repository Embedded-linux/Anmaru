/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 5: Scheduler Adaptation Interface
 *
 * Copyright (c) 2024 DSRTOS
 * All rights reserved.
 *
 * Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 * MISRA-C:2012 compliant
 */

#ifndef DSRTOS_SCHEDULER_ADAPT_H
#define DSRTOS_SCHEDULER_ADAPT_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 *                                 INCLUDES
 *============================================================================*/
#include "dsrtos_types.h"
#include "dsrtos_scheduler_core.h"

/*==============================================================================
 *                                  MACROS
 *============================================================================*/
/* Adaptation thresholds */
#define DSRTOS_ADAPT_CPU_THRESHOLD_LOW      (30U)
#define DSRTOS_ADAPT_CPU_THRESHOLD_HIGH     (80U)
#define DSRTOS_ADAPT_IPC_THRESHOLD_LOW      (100U)
#define DSRTOS_ADAPT_IPC_THRESHOLD_HIGH     (1000U)
#define DSRTOS_ADAPT_DEADLINE_MISS_LIMIT    (5U)
#define DSRTOS_ADAPT_STABILITY_THRESHOLD    (100U)

/* Adaptation triggers */
#define DSRTOS_ADAPT_TRIGGER_CPU            (1U << 0)
#define DSRTOS_ADAPT_TRIGGER_MEMORY         (1U << 1)
#define DSRTOS_ADAPT_TRIGGER_IPC            (1U << 2)
#define DSRTOS_ADAPT_TRIGGER_DEADLINE       (1U << 3)
#define DSRTOS_ADAPT_TRIGGER_CONTENTION     (1U << 4)
#define DSRTOS_ADAPT_TRIGGER_ENERGY         (1U << 5)
#define DSRTOS_ADAPT_TRIGGER_MANUAL         (1U << 6)
#define DSRTOS_ADAPT_TRIGGER_PERIODIC       (1U << 7)

/* Learning parameters */
#define DSRTOS_ADAPT_HISTORY_SIZE           (32U)
#define DSRTOS_ADAPT_LEARNING_RATE          (10U)  /* 0.1 in fixed point */
#define DSRTOS_ADAPT_CONFIDENCE_THRESHOLD   (80U)  /* 80% confidence */

/*==============================================================================
 *                                  TYPES
 *============================================================================*/
/* Adaptation modes */
typedef enum {
    DSRTOS_ADAPT_MODE_DISABLED = 0,
    DSRTOS_ADAPT_MODE_MANUAL,
    DSRTOS_ADAPT_MODE_ASSISTED,
    DSRTOS_ADAPT_MODE_AUTOMATIC,
    DSRTOS_ADAPT_MODE_LEARNING
} dsrtos_adapt_mode_t;

/* System state snapshot */
typedef struct {
    uint32_t timestamp;
    uint16_t scheduler_id;
    uint8_t cpu_usage;
    uint32_t memory_usage;
    uint32_t ipc_rate;
    uint32_t context_switches;
    uint32_t deadline_misses;
    uint32_t resource_contention;
    uint32_t energy_consumption;
    uint32_t performance_score;
} dsrtos_state_snapshot_t;

/* Adaptation decision */
typedef struct {
    uint16_t recommended_scheduler;
    uint32_t confidence_level;
    uint32_t trigger_mask;
    uint32_t decision_time_us;
    char reason[64];
} dsrtos_adapt_decision_t;

/* Learning model */
typedef struct {
    /* History buffer */
    dsrtos_state_snapshot_t history[DSRTOS_ADAPT_HISTORY_SIZE];
    uint32_t history_index;
    uint32_t history_count;
    
    /* Performance metrics per scheduler */
    struct {
        uint16_t scheduler_id;
        uint32_t usage_count;
        uint64_t total_runtime_ns;
        uint32_t average_cpu_usage;
        uint32_t deadline_success_rate;
        uint32_t energy_efficiency;
        uint32_t overall_score;
    } scheduler_stats[DSRTOS_MAX_SCHEDULERS];
    
    /* Weights for decision factors */
    struct {
        uint16_t cpu_weight;
        uint16_t memory_weight;
        uint16_t ipc_weight;
        uint16_t deadline_weight;
        uint16_t energy_weight;
    } weights;
    
    /* Pattern recognition */
    struct {
        uint32_t pattern_id;
        uint32_t occurrence_count;
        uint16_t best_scheduler;
        uint32_t success_rate;
    } patterns[16];
} dsrtos_adapt_model_t;

/* Adaptation configuration */
typedef struct {
    dsrtos_adapt_mode_t mode;
    uint32_t evaluation_period_ms;
    uint32_t min_stability_time_ms;
    uint32_t trigger_mask;
    
    /* Thresholds */
    struct {
        uint8_t cpu_low;
        uint8_t cpu_high;
        uint32_t ipc_low;
        uint32_t ipc_high;
        uint32_t deadline_miss_limit;
        uint32_t contention_limit;
    } thresholds;
    
    /* Learning parameters */
    struct {
        bool enabled;
        uint32_t learning_rate;
        uint32_t exploration_rate;
        uint32_t confidence_threshold;
    } learning;
} dsrtos_adapt_config_t;

/* Adaptation callback */
typedef void (*dsrtos_adapt_callback_t)(
    const dsrtos_adapt_decision_t* decision, void* user_data);

/*==============================================================================
 *                           FUNCTION PROTOTYPES
 *============================================================================*/
/* Initialization */
dsrtos_error_t dsrtos_adapt_init(void);
dsrtos_error_t dsrtos_adapt_deinit(void);

/* Configuration */
dsrtos_error_t dsrtos_adapt_configure(const dsrtos_adapt_config_t* config);
dsrtos_error_t dsrtos_adapt_get_config(dsrtos_adapt_config_t* config);
dsrtos_error_t dsrtos_adapt_set_mode(dsrtos_adapt_mode_t mode);
dsrtos_adapt_mode_t dsrtos_adapt_get_mode(void);

/* Adaptation control */
dsrtos_error_t dsrtos_adapt_start(void);
dsrtos_error_t dsrtos_adapt_stop(void);
void dsrtos_adapt_evaluate(void);
void dsrtos_adapt_force_evaluation(void);

/* Decision making */
dsrtos_adapt_decision_t dsrtos_adapt_make_decision(void);
uint16_t dsrtos_adapt_recommend_scheduler(
    const dsrtos_state_snapshot_t* state);
uint32_t dsrtos_adapt_calculate_confidence(
    uint16_t scheduler_id, const dsrtos_state_snapshot_t* state);

/* State management */
void dsrtos_adapt_capture_state(dsrtos_state_snapshot_t* snapshot);
void dsrtos_adapt_record_state(const dsrtos_state_snapshot_t* snapshot);
void dsrtos_adapt_get_history(
    dsrtos_state_snapshot_t* buffer, uint32_t* count);

/* Learning */
dsrtos_error_t dsrtos_adapt_enable_learning(bool enable);
void dsrtos_adapt_update_model(
    uint16_t scheduler_id, uint32_t performance_score);
void dsrtos_adapt_train_model(
    const dsrtos_state_snapshot_t* data, uint32_t count);
dsrtos_adapt_model_t* dsrtos_adapt_get_model(void);

/* Pattern recognition */
uint32_t dsrtos_adapt_detect_pattern(
    const dsrtos_state_snapshot_t* state);
void dsrtos_adapt_register_pattern(
    uint32_t pattern_id, uint16_t best_scheduler);

/* Callbacks */
dsrtos_error_t dsrtos_adapt_register_callback(
    dsrtos_adapt_callback_t callback, void* user_data);
dsrtos_error_t dsrtos_adapt_unregister_callback(
    dsrtos_adapt_callback_t callback);

/* Statistics */
void dsrtos_adapt_get_statistics(
    uint32_t* total_adaptations,
    uint32_t* successful_adaptations,
    uint32_t* average_decision_time_us);
void dsrtos_adapt_reset_statistics(void);

/* Validation */
bool dsrtos_adapt_validate_decision(
    const dsrtos_adapt_decision_t* decision);
bool dsrtos_adapt_is_stable(void);

#ifdef __cplusplus
}
#endif

#endif /* DSRTOS_SCHEDULER_ADAPT_H */
