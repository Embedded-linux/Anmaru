/*
 * @file dsrtos_task_state.c
 * @brief DSRTOS Task State Management and Transitions
 * @date 2024-12-30
 * 
 * Implements safe task state transitions with validation and
 * tracking for safety-critical systems.
 * 
 * COMPLIANCE:
 * - MISRA-C:2012 compliant
 * - DO-178C DAL-B certifiable
 * - IEC 62304 Class B compliant
 * - ISO 26262 ASIL D compliant
 */

#include "dsrtos_task_state.h"
#include "dsrtos_task_manager.h"
#include "dsrtos_task_queue.h"
#include "dsrtos_kernel.h"
#include "dsrtos_critical.h"
#include <string.h>

/*==============================================================================
 * CONSTANTS
 *============================================================================*/

#define STATE_TRANSITION_MAX_TIME_US   (100U)
#define STATE_HISTORY_SIZE             (8U)

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/* State transition rule */
typedef struct {
    dsrtos_task_state_t from_state;
    dsrtos_task_state_t to_state;
    bool allowed;
    bool (*validator)(const dsrtos_tcb_t *tcb);
} state_transition_rule_t;

/* State transition history entry */
typedef struct {
    uint32_t timestamp;
    dsrtos_task_state_t from_state;
    dsrtos_task_state_t to_state;
    uint32_t task_id;
} state_history_entry_t;

/* State management context */
typedef struct {
    uint32_t total_transitions;
    uint32_t invalid_transitions;
    uint32_t transition_time_max;
    state_history_entry_t history[STATE_HISTORY_SIZE];
    uint32_t history_index;
    dsrtos_state_change_hook_t state_change_hook;
} state_manager_t;

/*==============================================================================
 * STATIC VARIABLES
 *============================================================================*/

/* State manager instance */
static state_manager_t g_state_manager = {
    .total_transitions = 0U,
    .invalid_transitions = 0U,
    .transition_time_max = 0U,
    .history_index = 0U,
    .state_change_hook = NULL
};

/* State transition rules table */
static const state_transition_rule_t g_transition_rules[] = {
    /* From INVALID state */
    {DSRTOS_TASK_STATE_INVALID, DSRTOS_TASK_STATE_READY, true, NULL},
    {DSRTOS_TASK_STATE_INVALID, DSRTOS_TASK_STATE_DORMANT, true, NULL},
    
    /* From READY state */
    {DSRTOS_TASK_STATE_READY, DSRTOS_TASK_STATE_RUNNING, true, NULL},
    {DSRTOS_TASK_STATE_READY, DSRTOS_TASK_STATE_BLOCKED, true, NULL},
    {DSRTOS_TASK_STATE_READY, DSRTOS_TASK_STATE_SUSPENDED, true, NULL},
    {DSRTOS_TASK_STATE_READY, DSRTOS_TASK_STATE_TERMINATED, true, NULL},
    
    /* From RUNNING state */
    {DSRTOS_TASK_STATE_RUNNING, DSRTOS_TASK_STATE_READY, true, NULL},
    {DSRTOS_TASK_STATE_RUNNING, DSRTOS_TASK_STATE_BLOCKED, true, NULL},
    {DSRTOS_TASK_STATE_RUNNING, DSRTOS_TASK_STATE_SUSPENDED, true, NULL},
    {DSRTOS_TASK_STATE_RUNNING, DSRTOS_TASK_STATE_TERMINATED, true, NULL},
    
    /* From BLOCKED state */
    {DSRTOS_TASK_STATE_BLOCKED, DSRTOS_TASK_STATE_READY, true, NULL},
    {DSRTOS_TASK_STATE_BLOCKED, DSRTOS_TASK_STATE_SUSPENDED, true, NULL},
    {DSRTOS_TASK_STATE_BLOCKED, DSRTOS_TASK_STATE_TERMINATED, true, NULL},
    
    /* From SUSPENDED state */
    {DSRTOS_TASK_STATE_SUSPENDED, DSRTOS_TASK_STATE_READY, true, NULL},
    {DSRTOS_TASK_STATE_SUSPENDED, DSRTOS_TASK_STATE_TERMINATED, true, NULL},
    
    /* From DORMANT state */
    {DSRTOS_TASK_STATE_DORMANT, DSRTOS_TASK_STATE_READY, true, NULL},
    {DSRTOS_TASK_STATE_DORMANT, DSRTOS_TASK_STATE_TERMINATED, true, NULL},
    
    /* TERMINATED is final state - no transitions out */
};

/* State names for debugging */
static const char* const g_state_names[] = {
    "INVALID",
    "READY",
    "RUNNING",
    "BLOCKED",
    "SUSPENDED",
    "TERMINATED",
    "DORMANT"
};

/*==============================================================================
 * STATIC FUNCTION PROTOTYPES
 *============================================================================*/

static bool is_transition_valid(dsrtos_task_state_t from, dsrtos_task_state_t to);
static void record_transition(const dsrtos_tcb_t *tcb,
                             dsrtos_task_state_t from,
                             dsrtos_task_state_t to);
static dsrtos_error_t perform_state_actions(dsrtos_tcb_t *tcb,
                                           dsrtos_task_state_t new_state);
static void update_task_timing(dsrtos_tcb_t *tcb,
                              dsrtos_task_state_t old_state,
                              dsrtos_task_state_t new_state);

/*==============================================================================
 * PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize task state management
 * @return Error code
 */
dsrtos_error_t dsrtos_state_init(void)
{
    /* Clear state manager */
    (void)memset(&g_state_manager, 0, sizeof(g_state_manager));
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Transition task to new state
 * @param tcb Task control block
 * @param new_state New task state
 * @return Error code
 */
dsrtos_error_t dsrtos_state_transition(dsrtos_tcb_t *tcb,
                                       dsrtos_task_state_t new_state)
{
    dsrtos_task_state_t old_state;
    dsrtos_error_t err;
    uint32_t start_time;
    uint32_t transition_time;
    
    /* Validate parameters */
    if (tcb == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    if (new_state > DSRTOS_TASK_STATE_DORMANT) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    /* Verify TCB integrity */
    if (dsrtos_task_validate_tcb(tcb) != DSRTOS_SUCCESS) {
        return DSRTOS_ERROR_CORRUPTED;
    }
    
    old_state = tcb->state;
    
    /* Check if transition is valid */
    if (!is_transition_valid(old_state, new_state)) {
        g_state_manager.invalid_transitions++;
        return DSRTOS_ERROR_INVALID_STATE;
    }
    
    /* Same state - no transition needed */
    if (old_state == new_state) {
        return DSRTOS_SUCCESS;
    }
    
    start_time = dsrtos_get_tick_count();
    
    dsrtos_critical_enter();
    
    /* Store previous state */
    tcb->prev_state = old_state;
    
    /* Perform state-specific actions */
    err = perform_state_actions(tcb, new_state);
    if (err != DSRTOS_SUCCESS) {
        dsrtos_critical_exit();
        return err;
    }
    
    /* Update state */
    tcb->state = new_state;
    
    /* Update timing information */
    update_task_timing(tcb, old_state, new_state);
    
    /* Record transition */
    record_transition(tcb, old_state, new_state);
    
    /* Update statistics */
    g_state_manager.total_transitions++;
    
    dsrtos_critical_exit();
    
    /* Measure transition time */
    transition_time = dsrtos_get_tick_count() - start_time;
    if (transition_time > g_state_manager.transition_time_max) {
        g_state_manager.transition_time_max = transition_time;
    }
    
    /* Call state change hook if registered */
    if (g_state_manager.state_change_hook != NULL) {
        g_state_manager.state_change_hook(tcb, old_state, new_state);
    }
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get current task state
 * @param tcb Task control block
 * @param state Pointer to store state
 * @return Error code
 */
dsrtos_error_t dsrtos_state_get(const dsrtos_tcb_t *tcb,
                                dsrtos_task_state_t *state)
{
    /* Validate parameters */
    if ((tcb == NULL) || (state == NULL)) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    /* Verify TCB integrity */
    if (dsrtos_task_validate_tcb(tcb) != DSRTOS_SUCCESS) {
        return DSRTOS_ERROR_CORRUPTED;
    }
    
    *state = tcb->state;
    
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get task state name string
 * @param state Task state
 * @return State name string
 */
const char* dsrtos_state_get_name(dsrtos_task_state_t state)
{
    if (state <= DSRTOS_TASK_STATE_DORMANT) {
        return g_state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief Check if task is in runnable state
 * @param tcb Task control block
 * @return True if runnable
 */
bool dsrtos_state_is_runnable(const dsrtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        return false;
    }
    
    return (tcb->state == DSRTOS_TASK_STATE_READY) ||
           (tcb->state == DSRTOS_TASK_STATE_RUNNING);
}

/**
 * @brief Check if task is blocked
 * @param tcb Task control block
 * @return True if blocked
 */
bool dsrtos_state_is_blocked(const dsrtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        return false;
    }
    
    return (tcb->state == DSRTOS_TASK_STATE_BLOCKED) ||
           (tcb->state == DSRTOS_TASK_STATE_SUSPENDED);
}

/**
 * @brief Register state change hook
 * @param hook Hook function
 * @return Error code
 */
dsrtos_error_t dsrtos_state_register_hook(dsrtos_state_change_hook_t hook)
{
    g_state_manager.state_change_hook = hook;
    return DSRTOS_SUCCESS;
}

/**
 * @brief Get state transition statistics
 * @param stats Pointer to store statistics
 * @return Error code
 */
dsrtos_error_t dsrtos_state_get_stats(dsrtos_state_stats_t *stats)
{
    if (stats == NULL) {
        return DSRTOS_ERROR_INVALID_PARAM;
    }
    
    dsrtos_critical_enter();
    
    stats->total_transitions = g_state_manager.total_transitions;
    stats->invalid_transitions = g_state_manager.invalid_transitions;
    stats->transition_time_max = g_state_manager.transition_time_max;
    
    dsrtos_critical_exit();
    
    return DSRTOS_SUCCESS;
}

/*==============================================================================
 * STATIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Check if state transition is valid
 * @param from Source state
 * @param to Destination state
 * @return True if valid
 */
static bool is_transition_valid(dsrtos_task_state_t from, dsrtos_task_state_t to)
{
    uint32_t rule_count = sizeof(g_transition_rules) / sizeof(g_transition_rules[0]);
    
    for (uint32_t i = 0U; i < rule_count; i++) {
        if ((g_transition_rules[i].from_state == from) &&
            (g_transition_rules[i].to_state == to)) {
            return g_transition_rules[i].allowed;
        }
    }
    
    /* Transition not found in rules - not allowed */
    return false;
}

/**
 * @brief Record state transition in history
 * @param tcb Task control block
 * @param from Previous state
 * @param to New state
 */
static void record_transition(const dsrtos_tcb_t *tcb,
                             dsrtos_task_state_t from,
                             dsrtos_task_state_t to)
{
    state_history_entry_t *entry;
    
    entry = &g_state_manager.history[g_state_manager.history_index];
    
    entry->timestamp = dsrtos_get_system_time();
    entry->from_state = from;
    entry->to_state = to;
    entry->task_id = tcb->task_id;
    
    /* Circular buffer */
    g_state_manager.history_index = 
        (g_state_manager.history_index + 1U) % STATE_HISTORY_SIZE;
}

/**
 * @brief Perform state-specific actions
 * @param tcb Task control block
 * @param new_state New state
 * @return Error code
 */
static dsrtos_error_t perform_state_actions(dsrtos_tcb_t *tcb,
                                           dsrtos_task_state_t new_state)
{
    dsrtos_error_t err = DSRTOS_SUCCESS;
    
    /* Remove from current queue if needed */
    switch (tcb->state) {
        case DSRTOS_TASK_STATE_READY:
            err = dsrtos_queue_ready_remove(tcb);
            break;
            
        case DSRTOS_TASK_STATE_BLOCKED:
            err = dsrtos_queue_blocked_remove(tcb);
            break;
            
        case DSRTOS_TASK_STATE_SUSPENDED:
            err = dsrtos_queue_suspended_remove(tcb);
            break;
            
        default:
            /* No queue removal needed */
            break;
    }
    
    if (err != DSRTOS_SUCCESS) {
        return err;
    }
    
    /* Add to new queue if needed */
    switch (new_state) {
        case DSRTOS_TASK_STATE_READY:
            err = dsrtos_queue_ready_insert(tcb);
            break;
            
        case DSRTOS_TASK_STATE_BLOCKED:
            err = dsrtos_queue_blocked_insert(tcb, 0U);
            break;
            
        case DSRTOS_TASK_STATE_SUSPENDED:
            err = dsrtos_queue_suspended_insert(tcb);
            break;
            
        case DSRTOS_TASK_STATE_RUNNING:
            /* Update current task pointer - done by scheduler */
            break;
            
        case DSRTOS_TASK_STATE_TERMINATED:
            /* Clean up resources - done by task manager */
            break;
            
        default:
            break;
    }
    
    return err;
}

/**
 * @brief Update task timing based on state transition
 * @param tcb Task control block
 * @param old_state Previous state
 * @param new_state New state
 */
static void update_task_timing(dsrtos_tcb_t *tcb,
                              dsrtos_task_state_t old_state,
                              dsrtos_task_state_t new_state)
{
    uint64_t current_time = dsrtos_get_system_time();
    
    /* Leaving RUNNING state - update runtime */
    if (old_state == DSRTOS_TASK_STATE_RUNNING) {
        uint64_t runtime = current_time - tcb->timing.activation_time;
        tcb->timing.last_runtime = runtime;
        tcb->timing.total_runtime += runtime;
    }
    
    /* Entering RUNNING state - record activation */
    if (new_state == DSRTOS_TASK_STATE_RUNNING) {
        tcb->timing.activation_time = current_time;
        tcb->stats.context_switches++;
    }
    
    /* Track response time for newly ready tasks */
    if ((old_state == DSRTOS_TASK_STATE_BLOCKED) &&
        (new_state == DSRTOS_TASK_STATE_READY)) {
        /* Response time tracking - simplified */
        tcb->timing.response_time = 
            (uint32_t)(current_time - tcb->timing.activation_time);
    }
}
