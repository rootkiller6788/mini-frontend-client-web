#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_MACRO_TASKS      256
#define MAX_MICRO_TASKS      512
#define MAX_IDLE_CALLBACKS   64
#define LONG_TASK_THRESHOLD_MS  50
#define IDLE_DEADLINE_MS        15

typedef enum {
    TASK_SETTIMEOUT    = 0,
    TASK_SETINTERVAL   = 1,
    TASK_EVENT_CLICK   = 2,
    TASK_EVENT_KEY     = 3,
    TASK_EVENT_SCROLL  = 4,
    TASK_EVENT_RESIZE  = 5,
    TASK_MESSAGE       = 6,
    TASK_FETCH         = 7,
    TASK_IMMEDIATE     = 8,
    TASK_CUSTOM        = 9
} TaskType;

typedef enum {
    TASK_PENDING       = 0,
    TASK_RUNNING       = 1,
    TASK_COMPLETED     = 2,
    TASK_CANCELLED     = 3,
    TASK_FAILED        = 4
} TaskState;

typedef enum {
    MICRO_PROMISE_THEN    = 0,
    MICRO_PROMISE_CATCH   = 1,
    MICRO_PROMISE_FINALLY = 2,
    MICRO_MUTATION_OBSERVER = 3,
    MICRO_QUEUE_MICROTASK = 4
} MicroTaskType;

typedef void (*TaskFn)(void *user_data);
typedef void (*IdleCallbackFn)(uint64_t deadline_ms, bool did_timeout, void *user_data);

typedef struct {
    uint64_t    id;
    TaskType    type;
    TaskState   state;
    TaskFn      fn;
    void       *user_data;
    uint64_t    scheduled_time_ms;
    uint64_t    started_time_ms;
    uint64_t    completed_time_ms;
    uint64_t    delay_ms;
    int32_t     priority;
    bool        is_recurring;
} MacroTask;

typedef struct {
    uint64_t       id;
    MicroTaskType  type;
    TaskFn         fn;
    void          *user_data;
    bool           complete;
} MicroTask;

typedef struct {
    uint64_t         id;
    IdleCallbackFn   fn;
    void            *user_data;
    uint64_t         registered_time_ms;
    bool             executed;
    bool             cancelled;
} IdleCallbackEntry;

typedef struct {
    uint64_t        elapsed_ms;
    TaskType        task_type;
    uint64_t        task_id;
    bool            is_long_task;
    uint64_t        exceeded_by_ms;
    uint64_t        timestamp_ms;
} TaskMetrics;

typedef struct {
    MacroTask           macro_queue[MAX_MACRO_TASKS];
    uint32_t            macro_head;
    uint32_t            macro_tail;
    uint32_t            macro_count;

    MicroTask           micro_queue[MAX_MICRO_TASKS];
    uint32_t            micro_head;
    uint32_t            micro_tail;
    uint32_t            micro_count;

    IdleCallbackEntry   idle_callbacks[MAX_IDLE_CALLBACKS];
    uint32_t            idle_count;

    uint64_t            current_time_ms;
    uint64_t            last_vsync_ms;
    uint64_t            task_counter;
    uint64_t            long_task_count;
    uint64_t            total_tasks_processed;

    bool                running;
    bool                spin_microtasks;
    bool                idle_period_enabled;
    bool                detect_long_tasks;
    bool                starvation_prevention;
    uint32_t            max_consecutive_micro;
    uint32_t            micro_starvation_limit;

    TaskMetrics         last_task_metrics;
    uint64_t            idle_deadline_ms;
} EventLoop;

void el_init(EventLoop *el);
void el_destroy(EventLoop *el);

uint64_t el_schedule_task(EventLoop *el, TaskType type, TaskFn fn,
                          void *user_data, uint64_t delay_ms);
uint64_t el_schedule_recurring(EventLoop *el, TaskType type, TaskFn fn,
                               void *user_data, uint64_t interval_ms);
void el_cancel_task(EventLoop *el, uint64_t task_id);
void el_cancel_all_type(EventLoop *el, TaskType type);

uint64_t el_queue_microtask(EventLoop *el, MicroTaskType type,
                            TaskFn fn, void *user_data);
void el_enqueue_microtask(EventLoop *el, TaskFn fn, void *user_data);

uint64_t el_request_idle_callback(EventLoop *el, IdleCallbackFn fn,
                                  void *user_data);
void el_cancel_idle(EventLoop *el, uint64_t id);

void el_run(EventLoop *el);
void el_tick(EventLoop *el, uint64_t now_ms);
void el_flush_microtasks(EventLoop *el);
void el_process_one_macro(EventLoop *el, uint64_t now_ms);

void el_set_time(EventLoop *el, uint64_t now_ms);
uint64_t el_now(const EventLoop *el);

void el_enable_long_task_detection(EventLoop *el, bool enable);
void el_enable_starvation_prevention(EventLoop *el, bool enable);
void el_set_micro_starvation_limit(EventLoop *el, uint32_t limit);

TaskMetrics el_get_last_metrics(const EventLoop *el);
uint64_t el_get_long_task_count(const EventLoop *el);
uint64_t el_get_total_processed(const EventLoop *el);
bool el_is_empty(const EventLoop *el);
void el_dump_state(const EventLoop *el);

#endif
