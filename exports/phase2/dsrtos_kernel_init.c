/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Kernel Core Initialization Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_kernel_init.h"
#include "dsrtos_critical.h"
#include "dsrtos_panic.h"
#include "dsrtos_syscall.h"
#include "dsrtos_hooks.h"
#include "dsrtos_stats.h"
#include "dsrtos_assert.h"
#include "dsrtos_memory.h"
#include "dsrtos_error.h"
#include <string.h>
#include "core_cm4.h" 

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define KERNEL_INIT_TIMEOUT_MS      1000U
#define KERNEL_INIT_MAX_ENTRIES     32U
#define KERNEL_CHECKSUM_SEED        0xDEADBEEFU

/* Memory barriers for safety-critical operations */
#define KERNEL_MEMORY_BARRIER()     __asm volatile("dmb" ::: "memory")
#define KERNEL_DATA_BARRIER()       __asm volatile("dsb" ::: "memory")
#define KERNEL_INSTRUCTION_BARRIER() __asm volatile("isb" ::: "memory")

/*=============================================================================
 * PRIVATE TYPES
 *============================================================================*/
typedef struct {
    dsrtos_init_entry_t entries[KERNEL_INIT_MAX_ENTRIES];
    uint32_t count;
    uint32_t current_index;
} kernel_init_table_t;

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
/* Kernel Control Block - Aligned for cache optimization */
static dsrtos_kernel_t g_kernel_cb __attribute__((aligned(32))) = {
    .state = DSRTOS_KERNEL_STATE_UNINITIALIZED,
    .init_phase = DSRTOS_INIT_PHASE_HARDWARE,
    .config = NULL,
    .services = {NULL},
    .services_active = 0U,
    .uptime_ticks = 0U,
    .idle_ticks = 0U,
    .context_switches = 0U,
    .error_count = 0U,
    .last_error = DSRTOS_SUCCESS,
    .critical_nesting = 0U,
    .interrupt_nesting = 0U,
    .magic_number = DSRTOS_KERNEL_MAGIC,
    .checksum = 0U
};

/* Initialization table */
static kernel_init_table_t g_init_table = {
    .entries = {{0}},
    .count = 0U,
    .current_index = 0U
};

/* Default kernel configuration */
static const dsrtos_kernel_config_t g_default_config = {
    .cpu_frequency_hz = 168000000U,
    .systick_frequency_hz = 1000U,
    .heap_size_bytes = 65536U,
    .stack_size_words = 1024U,
    .max_tasks = 32U,
    .max_priorities = 32U,
    .max_mutexes = 16U,
    .max_semaphores = 16U,
    .max_queues = 8U,
    .max_timers = 16U,
    .features_enabled = 0xFFFFFFFFU,
    .stack_overflow_check = true,
    .memory_protection = true,
    .deadline_monitoring = true,
    .error_logging = true,
    .config_magic = DSRTOS_KERNEL_INIT_MAGIC,
    .config_checksum = 0U
};

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static uint32_t kernel_calculate_checksum(const void* data, size_t size);
static dsrtos_error_t kernel_init_phase_hardware(void);
static dsrtos_error_t kernel_init_phase_memory(void);
static dsrtos_error_t kernel_init_phase_interrupts(void);
static dsrtos_error_t kernel_init_phase_scheduler(void);
static dsrtos_error_t kernel_init_phase_services(void);
static dsrtos_error_t kernel_init_phase_drivers(void);
static dsrtos_error_t kernel_init_phase_application(void);
static dsrtos_error_t kernel_execute_init_phase(dsrtos_init_phase_t phase);
static void kernel_set_state(dsrtos_kernel_state_t new_state);
static bool kernel_verify_integrity(void);
static void kernel_update_checksum(void);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize the DSRTOS kernel
 */
/* CERTIFIED_DUPLICATE_REMOVED: dsrtos_error_t dsrtos_kernel_init(const dsrtos_kernel_config_t* config)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* MISRA-C:2012 Rule 15.5 - Single return point */
    do {
        /* Verify kernel is not already initialized */
        if (g_kernel_cb.state != DSRTOS_KERNEL_STATE_UNINITIALIZED) {
            result = DSRTOS_ERROR_INVALID_STATE;
            break;
        }
        
        /* Set initializing state */
        kernel_set_state(DSRTOS_KERNEL_STATE_INITIALIZING);
        
        /* Use default config if none provided */
        if (config == NULL) {
            config = &g_default_config;
        }
        
        /* Validate configuration */
        result = dsrtos_kernel_validate_config(config);
        if (result != DSRTOS_SUCCESS) {
            break;
        }
        
        /* Store configuration */
        g_kernel_cb.config = config;
        
        /* Execute initialization phases */
        for (dsrtos_init_phase_t phase = DSRTOS_INIT_PHASE_HARDWARE; 
             phase < DSRTOS_INIT_PHASE_MAX; 
             phase++) {
            
            g_kernel_cb.init_phase = phase;
            result = kernel_execute_init_phase(phase);
            
            if (result != DSRTOS_SUCCESS) {
                /* Log error and abort initialization */
                g_kernel_cb.last_error = result;
                g_kernel_cb.error_count++;
                break;
            }
        }
        
        if (result != DSRTOS_SUCCESS) {
            break;
        }
        
        /* Perform self-test */
        result = dsrtos_kernel_self_test();
        if (result != DSRTOS_SUCCESS) {
            break;
        }
        
        /* Update kernel checksum */
        kernel_update_checksum();
        
        /* Set ready state */
        kernel_set_state(DSRTOS_KERNEL_STATE_READY);
        
    } while (false);
    
    /* Handle initialization failure */
    if (result != DSRTOS_SUCCESS) {
        kernel_set_state(DSRTOS_KERNEL_STATE_ERROR);
    }
    
    return result;
}

/**
 * @brief Start the DSRTOS kernel
 */
/* CERTIFIED_DUPLICATE_REMOVED: dsrtos_error_t dsrtos_kernel_start(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Enter critical section */
    dsrtos_critical_enter();
    
    do {
        /* Verify kernel is ready to start */
        if (g_kernel_cb.state != DSRTOS_KERNEL_STATE_READY) {
            result = DSRTOS_ERROR_INVALID_STATE;
            break;
        }
        
        /* Verify kernel integrity */
        if (!kernel_verify_integrity()) {
            result = DSRTOS_ERROR_INTEGRITY;
            break;
        }
        
        /* Call pre-start hook if registered */
        dsrtos_hook_call(DSRTOS_HOOK_KERNEL_PRE_START, NULL);
        
        /* Set running state */
        kernel_set_state(DSRTOS_KERNEL_STATE_RUNNING);
        
        /* Enable interrupts and start scheduler */
        /* Note: This function typically doesn't return */
        dsrtos_critical_exit();
        
        /* Start the scheduler (implemented in Phase 5) */
        /* dsrtos_scheduler_start(); */
        
        /* Should not reach here */
        result = DSRTOS_ERROR_FATAL;
        
    } while (false);
    
    /* Exit critical section if error occurred */
    if (result != DSRTOS_SUCCESS) {
        dsrtos_critical_exit();
    }
    
    return result;
}

/**
 * @brief Shutdown the DSRTOS kernel
 */
dsrtos_error_t dsrtos_kernel_shutdown(uint32_t reason)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Enter critical section */
    dsrtos_critical_enter();
    
    do {
        /* Verify kernel is running */
        if (g_kernel_cb.state != DSRTOS_KERNEL_STATE_RUNNING) {
            result = DSRTOS_ERROR_INVALID_STATE;
            break;
        }
        
        /* Call pre-shutdown hook */
        dsrtos_hook_call(DSRTOS_HOOK_KERNEL_PRE_SHUTDOWN, &reason);
        
        /* Set shutdown state */
        kernel_set_state(DSRTOS_KERNEL_STATE_SHUTDOWN);
        
        /* Stop all services in reverse order */
        for (int32_t i = (int32_t)DSRTOS_SERVICE_MAX - 1; i >= 0; i--) {
            if ((g_kernel_cb.services_active & (1U << i)) != 0U) {
                /* Service shutdown would be implemented here */
                g_kernel_cb.services_active &= ~(1U << i);
            }
        }
        
        /* Call post-shutdown hook */
        dsrtos_hook_call(DSRTOS_HOOK_KERNEL_POST_SHUTDOWN, &reason);
        
        /* Disable interrupts */
        __disable_irq();
        
        /* System reset or halt */
        NVIC_SystemReset();
        
    } while (false);
    
    /* Should not reach here */
    dsrtos_critical_exit();
    
    return result;
}

/**
 * @brief Get current kernel state
 */
dsrtos_kernel_state_t dsrtos_kernel_get_state(void)
{
    dsrtos_kernel_state_t state;
    
    /* Atomic read of state */
    KERNEL_DATA_BARRIER();
    state = g_kernel_cb.state;
    KERNEL_DATA_BARRIER();
    
    return state;
}

/**
 * @brief Register a kernel service
 */
dsrtos_error_t dsrtos_kernel_register_service(
    dsrtos_service_id_t service_id,
    void* service_ptr)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Validate parameters */
    if ((service_id >= DSRTOS_SERVICE_MAX) || (service_ptr == NULL)) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (g_kernel_cb.state != DSRTOS_KERNEL_STATE_INITIALIZING) {
        result = DSRTOS_ERROR_INVALID_STATE;
    }
    else if (g_kernel_cb.services[service_id] != NULL) {
        result = DSRTOS_ERROR_ALREADY_EXISTS;
    }
    else {
        /* Register service */
        g_kernel_cb.services[service_id] = service_ptr;
        g_kernel_cb.services_active |= (1U << service_id);
    }
    
    return result;
}

/**
 * @brief Get a registered kernel service
 */
void* dsrtos_kernel_get_service(dsrtos_service_id_t service_id)
{
    void* service = NULL;
    
    if ((service_id < DSRTOS_SERVICE_MAX) &&
        ((g_kernel_cb.services_active & (1U << service_id)) != 0U)) {
        service = g_kernel_cb.services[service_id];
    }
    
    return service;
}

/**
 * @brief Register initialization callback
 */
dsrtos_error_t dsrtos_kernel_register_init(const dsrtos_init_entry_t* entry)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Validate parameters */
    if (entry == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (entry->callback == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (g_init_table.count >= KERNEL_INIT_MAX_ENTRIES) {
        result = DSRTOS_ERROR_NO_MEMORY;
    }
    else {
        /* Add entry to initialization table */
        g_init_table.entries[g_init_table.count] = *entry;
        g_init_table.count++;
    }
    
    return result;
}

/**
 * @brief Get kernel version information
 */
void dsrtos_kernel_get_version(
    uint32_t* major,
    uint32_t* minor,
    uint32_t* patch,
    uint32_t* build)
{
    if (major != NULL) {
        *major = DSRTOS_VERSION_MAJOR;
    }
    if (minor != NULL) {
        *minor = DSRTOS_VERSION_MINOR;
    }
    if (patch != NULL) {
        *patch = DSRTOS_VERSION_PATCH;
    }
    if (build != NULL) {
        *build = DSRTOS_VERSION_BUILD;
    }
}

/**
 * @brief Get kernel statistics
 */
void dsrtos_kernel_get_stats(
    uint64_t* uptime,
    uint64_t* idle_time,
    uint32_t* context_switches)
{
    /* Enter critical section for atomic read */
    dsrtos_critical_enter();
    
    if (uptime != NULL) {
        *uptime = g_kernel_cb.uptime_ticks;
    }
    if (idle_time != NULL) {
        *idle_time = g_kernel_cb.idle_ticks;
    }
    if (context_switches != NULL) {
        *context_switches = g_kernel_cb.context_switches;
    }
    
    dsrtos_critical_exit();
}

/**
 * @brief Validate kernel configuration
 */
dsrtos_error_t dsrtos_kernel_validate_config(const dsrtos_kernel_config_t* config)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (config == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (config->config_magic != DSRTOS_KERNEL_INIT_MAGIC) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->cpu_frequency_hz == 0U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->systick_frequency_hz == 0U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->heap_size_bytes < 1024U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->stack_size_words < 128U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->max_tasks == 0U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else if (config->max_priorities == 0U) {
        result = DSRTOS_ERROR_INVALID_CONFIG;
    }
    else {
        /* Configuration is valid */
    }
    
    return result;
}

/**
 * @brief Perform kernel self-test
 */
/* CERTIFIED_DUPLICATE_REMOVED: dsrtos_error_t dsrtos_kernel_self_test(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    uint32_t test_value = 0x12345678U;
    uint32_t test_result = 0U;
    
    /* Test 1: Memory write/read */
    volatile uint32_t* test_addr = (volatile uint32_t*)&test_result;
    *test_addr = test_value;
    KERNEL_DATA_BARRIER();
    
    if (*test_addr != test_value) {
        result = DSRTOS_ERROR_SELF_TEST;
    }
    
    /* Test 2: Critical section */
    if (result == DSRTOS_SUCCESS) {
        dsrtos_critical_enter();
        test_result = g_kernel_cb.critical_nesting;
        dsrtos_critical_exit();
        
        if (test_result == 0U) {
            result = DSRTOS_ERROR_SELF_TEST;
        }
    }
    
    /* Test 3: Service registration */
    if (result == DSRTOS_SUCCESS) {
        void* dummy_service = (void*)&test_value;
        dsrtos_service_id_t test_id = DSRTOS_SERVICE_DEBUG;
        
        if (g_kernel_cb.services[test_id] == NULL) {
            dsrtos_kernel_register_service(test_id, dummy_service);
            if (dsrtos_kernel_get_service(test_id) != dummy_service) {
                result = DSRTOS_ERROR_SELF_TEST;
            }
            /* Clean up */
            g_kernel_cb.services[test_id] = NULL;
            g_kernel_cb.services_active &= ~(1U << test_id);
        }
    }
    
    /* Test 4: Checksum calculation */
    if (result == DSRTOS_SUCCESS) {
        uint32_t checksum = kernel_calculate_checksum(&test_value, sizeof(test_value));
        if (checksum == 0U) {
            result = DSRTOS_ERROR_SELF_TEST;
        }
    }
    
    return result;
}

/**
 * @brief Get kernel control block
 */
dsrtos_kernel_t* dsrtos_kernel_get_kcb(void)
{
    return &g_kernel_cb;
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Calculate checksum for data
 */
static uint32_t kernel_calculate_checksum(const void* data, size_t size)
{
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = KERNEL_CHECKSUM_SEED;
    
    for (size_t i = 0U; i < size; i++) {
        checksum ^= (uint32_t)bytes[i];
        checksum = (checksum << 1) | (checksum >> 31);
    }
    
    return checksum;
}

/**
 * @brief Initialize hardware phase
 */
static dsrtos_error_t kernel_init_phase_hardware(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Initialize FPU if available */
    #ifdef __FPU_PRESENT
    if (__FPU_PRESENT == 1U) {
        SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
    }
    #endif
    
    /* Set interrupt priority grouping */
    NVIC_SetPriorityGrouping(3U);
    
    /* Configure system handlers priority */
    NVIC_SetPriority(SVCall_IRQn, 0xFFU);
    NVIC_SetPriority(PendSV_IRQn, 0xFFU);
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
    
    return result;
}

/**
 * @brief Initialize memory phase
 */
static dsrtos_error_t kernel_init_phase_memory(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Initialize memory management (Phase 13-14) */
    /* This would initialize heap, memory pools, etc. */
    
    /* Clear BSS section */
    extern uint32_t _sbss, _ebss;
    uint32_t* bss_start = &_sbss;
    uint32_t* bss_end = &_ebss;
    
    while (bss_start < bss_end) {
        *bss_start++ = 0U;
    }
    
    return result;
}

/**
 * @brief Initialize interrupts phase
 */
static dsrtos_error_t kernel_init_phase_interrupts(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Clear all pending interrupts */
    for (uint32_t i = 0U; i < 8U; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
    
    /* Enable fault handlers */
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk |
                  SCB_SHCSR_BUSFAULTENA_Msk |
                  SCB_SHCSR_MEMFAULTENA_Msk;
    
    return result;
}

/**
 * @brief Initialize scheduler phase
 */
static dsrtos_error_t kernel_init_phase_scheduler(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Initialize scheduler (Phase 5) */
    /* This would initialize the scheduler framework */
    
    return result;
}

/**
 * @brief Initialize services phase
 */
static dsrtos_error_t kernel_init_phase_services(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Initialize kernel services */
    /* Critical section, syscall, error handler, statistics, hooks */
    
    return result;
}

/**
 * @brief Initialize drivers phase
 */
static dsrtos_error_t kernel_init_phase_drivers(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Initialize device drivers */
    /* This would initialize UART, GPIO, etc. (Phases 29-31) */
    
    return result;
}

/**
 * @brief Initialize application phase
 */
static dsrtos_error_t kernel_init_phase_application(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Call application initialization hook */
    dsrtos_hook_call(DSRTOS_HOOK_APP_INIT, NULL);
    
    return result;
}

/**
 * @brief Execute initialization phase
 */
static dsrtos_error_t kernel_execute_init_phase(dsrtos_init_phase_t phase)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Execute built-in phase initialization */
    switch (phase) {
        case DSRTOS_INIT_PHASE_HARDWARE:
            result = kernel_init_phase_hardware();
            break;
            
        case DSRTOS_INIT_PHASE_MEMORY:
            result = kernel_init_phase_memory();
            break;
            
        case DSRTOS_INIT_PHASE_INTERRUPTS:
            result = kernel_init_phase_interrupts();
            break;
            
        case DSRTOS_INIT_PHASE_SCHEDULER:
            result = kernel_init_phase_scheduler();
            break;
            
        case DSRTOS_INIT_PHASE_SERVICES:
            result = kernel_init_phase_services();
            break;
            
        case DSRTOS_INIT_PHASE_DRIVERS:
            result = kernel_init_phase_drivers();
            break;
            
        case DSRTOS_INIT_PHASE_APPLICATION:
            result = kernel_init_phase_application();
            break;
            
        default:
            result = DSRTOS_ERROR_INVALID_PARAM;
            break;
    }
    
    /* Execute registered callbacks for this phase */
    if (result == DSRTOS_SUCCESS) {
        for (uint32_t i = 0U; i < g_init_table.count; i++) {
            if (g_init_table.entries[i].phase == phase) {
                result = g_init_table.entries[i].callback();
                if (result != DSRTOS_SUCCESS) {
                    /* Critical component failed */
                    if (g_init_table.entries[i].critical) {
                        break;
                    }
                    /* Non-critical, continue */
                    result = DSRTOS_SUCCESS;
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief Set kernel state with safety checks
 */
static void kernel_set_state(dsrtos_kernel_state_t new_state)
{
    /* Validate state transition */
    dsrtos_kernel_state_t current = g_kernel_cb.state;
    bool valid_transition = false;
    
    switch (new_state) {
        case DSRTOS_KERNEL_STATE_INITIALIZING:
            valid_transition = (current == DSRTOS_KERNEL_STATE_UNINITIALIZED);
            break;
            
        case DSRTOS_KERNEL_STATE_READY:
            valid_transition = (current == DSRTOS_KERNEL_STATE_INITIALIZING);
            break;
            
        case DSRTOS_KERNEL_STATE_RUNNING:
            valid_transition = (current == DSRTOS_KERNEL_STATE_READY);
            break;
            
        case DSRTOS_KERNEL_STATE_SUSPENDED:
            valid_transition = (current == DSRTOS_KERNEL_STATE_RUNNING);
            break;
            
        case DSRTOS_KERNEL_STATE_ERROR:
            valid_transition = true; /* Can enter error from any state */
            break;
            
        case DSRTOS_KERNEL_STATE_SHUTDOWN:
            valid_transition = (current == DSRTOS_KERNEL_STATE_RUNNING) ||
                              (current == DSRTOS_KERNEL_STATE_SUSPENDED);
            break;
            
        default:
            valid_transition = false;
            break;
    }
    
    if (valid_transition) {
        KERNEL_DATA_BARRIER();
        g_kernel_cb.state = new_state;
        KERNEL_DATA_BARRIER();
    }
    else {
        /* Invalid state transition - enter error state */
        g_kernel_cb.state = DSRTOS_KERNEL_STATE_ERROR;
        g_kernel_cb.last_error = DSRTOS_ERROR_INVALID_STATE;
    }
}

/**
 * @brief Verify kernel integrity
 */
static bool kernel_verify_integrity(void)
{
    bool valid = true;
    
    /* Check magic number */
    if (g_kernel_cb.magic_number != DSRTOS_KERNEL_MAGIC) {
        valid = false;
    }
    
    /* Verify checksum */
    if (valid) {
        uint32_t calculated = kernel_calculate_checksum(
            &g_kernel_cb,
            offsetof(dsrtos_kernel_t, checksum)
        );
        
        if (calculated != g_kernel_cb.checksum) {
            valid = false;
        }
    }
    
    /* Verify configuration */
    if (valid && (g_kernel_cb.config != NULL)) {
        if (dsrtos_kernel_validate_config(g_kernel_cb.config) != DSRTOS_SUCCESS) {
            valid = false;
        }
    }
    
    return valid;
}

/**
 * @brief Update kernel checksum
 */
static void kernel_update_checksum(void)
{
    g_kernel_cb.checksum = kernel_calculate_checksum(
        &g_kernel_cb,
        offsetof(dsrtos_kernel_t, checksum)
    );
}
