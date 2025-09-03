#ifndef DSRTOS_COMMON_H
#define DSRTOS_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* DSRTOS Configuration - Optimized for low latency and small footprint */
#define DSRTOS_VERSION "2.0"
#define MAX_TASKS 32
#define MAX_PRIORITIES 5
#define DEFAULT_STACK_SIZE 1024
#define TICK_RATE_HZ 1000
#define TIME_SLICE_MS 10

/* Task States */
typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED,
    TASK_DELETED
} task_state_t;

/* Priority Levels */
typedef enum {
    PRIORITY_IDLE = 0,
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
} priority_t;

/* Task Control Block - Optimized for cache efficiency */
typedef struct tcb {
    uint32_t *stack_ptr;        /* Stack pointer - MUST be first */
    uint32_t stack_size;
    uint8_t task_id;
    uint8_t priority;
    uint8_t state;
    uint8_t flags;
    struct tcb *next;
    struct tcb *prev;
    void (*task_func)(void*);
    void *params;
    uint32_t wake_time;
} tcb_t;

/* Scheduler Control Block */
typedef struct {
    tcb_t *current_task;
    tcb_t *ready_list[MAX_PRIORITIES];
    uint32_t task_count;
    uint32_t system_ticks;
    uint8_t scheduler_running;
} scheduler_t;

/* Global RTOS Functions */
void dsrtos_init(void);
int dsrtos_start(void);
tcb_t* dsrtos_task_create(void (*func)(void*), void *params, uint8_t priority);
void dsrtos_task_delete(tcb_t *task);
void dsrtos_delay(uint32_t ticks);
void dsrtos_yield(void);

/* Scheduler Functions */
void scheduler_init(void);
void scheduler_start(void);
void scheduler_tick(void);
void schedule(void);

/* Context Switch Functions */
void context_switch(uint32_t **old_sp, uint32_t *new_sp);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* Memory Management */
void* dsrtos_malloc(size_t size);
void dsrtos_free(void *ptr);

#endif /* DSRTOS_COMMON_H */
