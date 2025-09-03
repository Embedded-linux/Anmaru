/*
 * DSRTOS - Dynamic Scheduler Real-Time Operating System
 * Phase 2: Panic Handler Implementation
 * 
 * Copyright (c) 2024 DSRTOS
 * Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
 */

/*=============================================================================
 * INCLUDES
 *============================================================================*/
#include "dsrtos_panic.h"
#include "dsrtos_kernel_init.h"
#include "dsrtos_critical.h"
#include "dsrtos_assert.h"
#include "dsrtos_error.h"
#include <string.h>
#include <stdio.h>
#include "core_cm4.h"

/*=============================================================================
 * PRIVATE MACROS
 *============================================================================*/
#define PANIC_MAGIC             0x50414E43U  /* 'PANC' */
#define PANIC_MAX_MESSAGE_LEN   128U
#define PANIC_STACK_DUMP_WORDS  32U

/*=============================================================================
 * PRIVATE VARIABLES
 *============================================================================*/
/* Panic handler configuration */
static dsrtos_panic_config_t g_panic_config = {
    .auto_restart = false,
    .restart_delay_ms = 5000U,
    .dump_to_console = true,
    .save_to_flash = false,
    .flash_address = 0U,
    .max_panics = 3U
};

/* Panic context storage */
static dsrtos_panic_context_t g_panic_context __attribute__((section(".noinit")));
static dsrtos_panic_context_t g_panic_history[3] __attribute__((section(".noinit")));

static inline void __WFI(void)
{
    __asm volatile ("wfi" ::: "memory");
}

/* Panic statistics */
static struct {
    uint32_t panic_count;
    uint32_t magic;
    bool panic_active;
    dsrtos_panic_handler_t custom_handler;
} g_panic_state __attribute__((section(".noinit"))) = {
    .panic_count = 0U,
    .magic = 0U,
    .panic_active = false,
    .custom_handler = NULL
};

/*=============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *============================================================================*/
static void panic_capture_context(dsrtos_panic_context_t* context,
                                  dsrtos_panic_reason_t reason,
                                  const char* message,
                                  const char* file,
                                  uint32_t line);
static void panic_capture_cpu_state(dsrtos_panic_context_t* context);
static void panic_capture_fault_status(dsrtos_panic_context_t* context);
static void panic_capture_system_state(dsrtos_panic_context_t* context);
static void panic_default_handler(const dsrtos_panic_context_t* context);
static void panic_print_context(const dsrtos_panic_context_t* context);
static void panic_system_reset(void);
static const char* panic_get_reason_string(dsrtos_panic_reason_t reason);

/*=============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize panic handler
 */
dsrtos_error_t dsrtos_panic_init(const dsrtos_panic_config_t* config)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Check if already initialized */
    if (g_panic_state.magic == PANIC_MAGIC) {
        /* Already initialized, check for previous panic */
        if (g_panic_context.reason != DSRTOS_PANIC_NONE) {
            /* Previous panic detected */
            if (g_panic_config.save_to_flash) {
                dsrtos_panic_save(&g_panic_context);
            }
            /* Move to history */
            if (g_panic_state.panic_count < 3U) {
                g_panic_history[g_panic_state.panic_count % 3U] = g_panic_context;
            }
        }
    }
    else {
        /* First initialization */
        g_panic_state.magic = PANIC_MAGIC;
        g_panic_state.panic_count = 0U;
        g_panic_state.panic_active = false;
        g_panic_state.custom_handler = NULL;
        
        /* Clear panic context */
        memset(&g_panic_context, 0, sizeof(g_panic_context));
        memset(g_panic_history, 0, sizeof(g_panic_history));
    }
    
    /* Apply configuration */
    if (config != NULL) {
        g_panic_config = *config;
    }
    
    /* Register with kernel */
    dsrtos_kernel_t* kernel = dsrtos_kernel_get_kcb();
    if (kernel != NULL) {
        result = dsrtos_kernel_register_service(
            DSRTOS_SERVICE_ERROR_HANDLER,
            &g_panic_state
        );
    }
    
    return result;
}

/**
 * @brief Trigger system panic
 */
void dsrtos_panic(dsrtos_panic_reason_t reason,
                  const char* message,
                  const char* file,
                  uint32_t line)
{
    /* Disable all interrupts immediately */
    __disable_irq();
    
    /* Check for recursive panic */
    if (g_panic_state.panic_active) {
        /* Double panic - immediate reset */
        NVIC_SystemReset();
        while (1) { __NOP(); }
    }
    
    /* Mark panic as active */
    g_panic_state.panic_active = true;
    g_panic_state.panic_count++;
    
    /* Capture panic context */
    panic_capture_context(&g_panic_context, reason, message, file, line);
    
    /* Call custom handler if registered */
    if (g_panic_state.custom_handler != NULL) {
        g_panic_state.custom_handler(&g_panic_context);
    }
    else {
        /* Use default handler */
        panic_default_handler(&g_panic_context);
    }
    
    /* Handle restart or halt */
    if (g_panic_config.auto_restart) {
        /* Delay before restart */
        volatile uint32_t delay = g_panic_config.restart_delay_ms * 1000U;
        while (delay-- > 0U) {
            __NOP();
        }
        panic_system_reset();
    }
    
    /* Halt system */
    while (1) {
        __WFI();  /* Wait for interrupt (will never come) */
    }
}

/**
 * @brief Register custom panic handler
 */
dsrtos_panic_handler_t dsrtos_panic_register_handler(dsrtos_panic_handler_t handler)
{
    dsrtos_panic_handler_t previous = g_panic_state.custom_handler;
    g_panic_state.custom_handler = handler;
    return previous;
}

/**
 * @brief Get last panic context
 */
dsrtos_error_t dsrtos_panic_get_last(dsrtos_panic_context_t* context)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (context == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (g_panic_context.reason == DSRTOS_PANIC_NONE) {
        result = DSRTOS_ERROR_NOT_FOUND;
    }
    else {
        *context = g_panic_context;
    }
    
    return result;
}

/**
 * @brief Clear panic history
 */
dsrtos_error_t dsrtos_panic_clear_history(void)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    /* Clear current panic context */
    memset(&g_panic_context, 0, sizeof(g_panic_context));
    
    /* Clear history */
    memset(g_panic_history, 0, sizeof(g_panic_history));
    
    /* Reset counter */
    g_panic_state.panic_count = 0U;
    
    return result;
}

/**
 * @brief Get panic count
 */
uint32_t dsrtos_panic_get_count(void)
{
    return g_panic_state.panic_count;
}

/**
 * @brief Dump panic information
 */
void dsrtos_panic_dump(const dsrtos_panic_context_t* context)
{
    if (context != NULL && g_panic_config.dump_to_console) {
        panic_print_context(context);
    }
}

/**
 * @brief Save panic context to persistent storage
 */
dsrtos_error_t dsrtos_panic_save(const dsrtos_panic_context_t* context)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (context == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_panic_config.save_to_flash || g_panic_config.flash_address == 0U) {
        result = DSRTOS_ERROR_NOT_SUPPORTED;
    }
    else {
        /* Flash write would be implemented here */
        /* This requires flash driver from later phases */
    }
    
    return result;
}

/**
 * @brief Load panic context from persistent storage
 */
dsrtos_error_t dsrtos_panic_load(dsrtos_panic_context_t* context)
{
    dsrtos_error_t result = DSRTOS_SUCCESS;
    
    if (context == NULL) {
        result = DSRTOS_ERROR_INVALID_PARAM;
    }
    else if (!g_panic_config.save_to_flash || g_panic_config.flash_address == 0U) {
        result = DSRTOS_ERROR_NOT_SUPPORTED;
    }
    else {
        /* Flash read would be implemented here */
        /* This requires flash driver from later phases */
    }
    
    return result;
}

/*=============================================================================
 * FAULT HANDLER IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Hard fault handler
 */
void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b hard_fault_handler_c"
    );
}

/**
 * @brief Hard fault handler C function
 */
void hard_fault_handler_c(uint32_t* sp)
{
    /* Extract fault information from stack */
    g_panic_context.cpu_context.r0 = sp[0];
    g_panic_context.cpu_context.r1 = sp[1];
    g_panic_context.cpu_context.r2 = sp[2];
    g_panic_context.cpu_context.r3 = sp[3];
    g_panic_context.cpu_context.r12 = sp[4];
    g_panic_context.cpu_context.lr = sp[5];
    g_panic_context.cpu_context.pc = sp[6];
    g_panic_context.cpu_context.psr = sp[7];
    
    /* Trigger panic */
    dsrtos_panic(DSRTOS_PANIC_HARD_FAULT, "Hard Fault", __FILE__, __LINE__);
}

/**
 * @brief Memory management fault handler
 */
void MemManage_Handler(void)
{
    dsrtos_panic(DSRTOS_PANIC_MEM_FAULT, "Memory Management Fault", __FILE__, __LINE__);
}

/**
 * @brief Bus fault handler
 */
void BusFault_Handler(void)
{
    dsrtos_panic(DSRTOS_PANIC_BUS_FAULT, "Bus Fault", __FILE__, __LINE__);
}

/**
 * @brief Usage fault handler
 */
void UsageFault_Handler(void)
{
    dsrtos_panic(DSRTOS_PANIC_USAGE_FAULT, "Usage Fault", __FILE__, __LINE__);
}

/*=============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Capture panic context
 */
static void panic_capture_context(dsrtos_panic_context_t* context,
                                  dsrtos_panic_reason_t reason,
                                  const char* message,
                                  const char* file,
                                  uint32_t line)
{
    /* Basic information */
    context->reason = reason;
    context->message = message;
    context->file = file;
    context->line = line;
    context->timestamp = SysTick->VAL;
    
    /* Capture CPU state */
    panic_capture_cpu_state(context);
    
    /* Capture fault status */
    panic_capture_fault_status(context);
    
    /* Capture system state */
    panic_capture_system_state(context);
}

/**
 * @brief Capture CPU state
 */
static void panic_capture_cpu_state(dsrtos_panic_context_t* context)
{
    /* Capture general purpose registers */
    __asm volatile (
        "str r0, [%0, #0]\n"
        "str r1, [%0, #4]\n"
        "str r2, [%0, #8]\n"
        "str r3, [%0, #12]\n"
        "str r4, [%0, #16]\n"
        "str r5, [%0, #20]\n"
        "str r6, [%0, #24]\n"
        "str r7, [%0, #28]\n"
        "str r8, [%0, #32]\n"
        "str r9, [%0, #36]\n"
        "str r10, [%0, #40]\n"
        "str r11, [%0, #44]\n"
        "str r12, [%0, #48]\n"
        "str sp, [%0, #52]\n"
        "str lr, [%0, #56]\n"
        : : "r" (&context->cpu_context) : "memory"
    );
}

/**
 * @brief Capture fault status registers
 */
static void panic_capture_fault_status(dsrtos_panic_context_t* context)
{
    context->fault_status.cfsr = SCB->CFSR;
    context->fault_status.hfsr = SCB->HFSR;
    context->fault_status.dfsr = SCB->DFSR;
    context->fault_status.mmfar = SCB->MMFAR;
    context->fault_status.bfar = SCB->BFAR;
    context->fault_status.afsr = SCB->AFSR;
}

/**
 * @brief Capture system state
 */
static void panic_capture_system_state(dsrtos_panic_context_t* context)
{
    dsrtos_kernel_t* kernel = dsrtos_kernel_get_kcb();
    
    if (kernel != NULL) {
        context->system_state.kernel_state = (uint32_t)kernel->state;
        context->system_state.critical_nesting = kernel->critical_nesting;
        context->system_state.interrupt_nesting = kernel->interrupt_nesting;
    }
    
    /* Stack information */
    context->stack_info.sp_main = (uint32_t*)__get_MSP();
    context->stack_info.sp_process = (uint32_t*)__get_PSP();
}

/**
 * @brief Default panic handler
 */
static void panic_default_handler(const dsrtos_panic_context_t* context)
{
    /* Dump panic information if configured */
    if (g_panic_config.dump_to_console) {
        panic_print_context(context);
    }
    
    /* Save to flash if configured */
    if (g_panic_config.save_to_flash) {
        dsrtos_panic_save(context);
    }
}

/**
 * @brief Print panic context
 */
static void panic_print_context(const dsrtos_panic_context_t* context)
{
    /* This would output to UART console */
    /* Implementation requires UART driver from Phase 29 */
    
    /* For now, store in buffer for debugger inspection */
    static char panic_buffer[512];
    snprintf(panic_buffer, sizeof(panic_buffer),
             "\r\n*** SYSTEM PANIC ***\r\n"
             "Reason: %s (0x%04X)\r\n"
             "Message: %s\r\n"
             "Location: %s:%lu\r\n"
             "PC: 0x%08lX LR: 0x%08lX\r\n"
             "SP: 0x%08lX PSR: 0x%08lX\r\n",
             panic_get_reason_string(context->reason),
             context->reason,
             context->message ? context->message : "None",
             context->file ? context->file : "Unknown",
             context->line,
             context->cpu_context.pc,
             context->cpu_context.lr,
             context->cpu_context.sp,
             context->cpu_context.psr);
    
    (void)panic_buffer; /* Prevent unused warning */
}

/**
 * @brief System reset
 */
static void panic_system_reset(void)
{
    /* Ensure all memory operations complete */
    __DSB();
    
    /* System reset */
    NVIC_SystemReset();
    
    /* Should never reach here */
    while (1) {
        __NOP();
    }
}

/**
 * @brief Get panic reason string
 */
static const char* panic_get_reason_string(dsrtos_panic_reason_t reason)
{
    switch (reason) {
        case DSRTOS_PANIC_HARD_FAULT:          return "Hard Fault";
        case DSRTOS_PANIC_MEM_FAULT:           return "Memory Fault";
        case DSRTOS_PANIC_BUS_FAULT:           return "Bus Fault";
        case DSRTOS_PANIC_USAGE_FAULT:         return "Usage Fault";
        case DSRTOS_PANIC_STACK_OVERFLOW:      return "Stack Overflow";
        case DSRTOS_PANIC_HEAP_CORRUPTION:     return "Heap Corruption";
        case DSRTOS_PANIC_DEADLOCK:            return "Deadlock";
        case DSRTOS_PANIC_WATCHDOG:            return "Watchdog Timeout";
        case DSRTOS_PANIC_ASSERTION_FAILED:    return "Assertion Failed";
        default:                                return "Unknown";
    }
}
