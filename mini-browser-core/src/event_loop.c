#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void el_init(EventLoop *el) {
    memset(el, 0, sizeof(*el));
    el->running = false;
    el->spin_microtasks = true;
    el->detect_long_tasks = true;
    el->starvation_prevention = true;
    el->max_consecutive_micro = 100;
    el->micro_starvation_limit = 128;
    el->idle_period_enabled = true;
    el->idle_deadline_ms = IDLE_DEADLINE_MS;
}

void el_destroy(EventLoop *el) {
    memset(el, 0, sizeof(*el));
}

uint64_t el_schedule_task(EventLoop *el, TaskType type, TaskFn fn,
                          void *user_data, uint64_t delay_ms) {
    if (el->macro_count >= MAX_MACRO_TASKS) return 0;
    uint64_t id = ++el->task_counter;
    MacroTask *t = &el->macro_queue[el->macro_tail];
    t->id = id;
    t->type = type;
    t->state = TASK_PENDING;
    t->fn = fn;
    t->user_data = user_data;
    t->scheduled_time_ms = el->current_time_ms;
    t->started_time_ms = 0;
    t->completed_time_ms = 0;
    t->delay_ms = delay_ms;
    t->priority = 0;
    t->is_recurring = false;
    el->macro_tail = (el->macro_tail + 1) % MAX_MACRO_TASKS;
    el->macro_count++;
    return id;
}

uint64_t el_schedule_recurring(EventLoop *el, TaskType type, TaskFn fn,
                               void *user_data, uint64_t interval_ms) {
    uint64_t id = el_schedule_task(el, type, fn, user_data, interval_ms);
    if (id && el->macro_count > 0) {
        uint32_t idx = (el->macro_tail == 0) ? MAX_MACRO_TASKS - 1 : el->macro_tail - 1;
        el->macro_queue[idx].is_recurring = true;
    }
    return id;
}

void el_cancel_task(EventLoop *el, uint64_t task_id) {
    for (uint32_t i = el->macro_head, c = 0; c < el->macro_count; c++, i = (i + 1) % MAX_MACRO_TASKS) {
        if (el->macro_queue[i].id == task_id) {
            el->macro_queue[i].state = TASK_CANCELLED;
            return;
        }
    }
}

void el_cancel_all_type(EventLoop *el, TaskType type) {
    for (uint32_t i = el->macro_head, c = 0; c < el->macro_count; c++, i = (i + 1) % MAX_MACRO_TASKS) {
        if (el->macro_queue[i].type == type)
            el->macro_queue[i].state = TASK_CANCELLED;
    }
}

uint64_t el_queue_microtask(EventLoop *el, MicroTaskType type,
                            TaskFn fn, void *user_data) {
    if (el->micro_count >= MAX_MICRO_TASKS) return 0;
    uint64_t id = ++el->task_counter;
    MicroTask *mt = &el->micro_queue[el->micro_tail];
    mt->id = id;
    mt->type = type;
    mt->fn = fn;
    mt->user_data = user_data;
    mt->complete = false;
    el->micro_tail = (el->micro_tail + 1) % MAX_MICRO_TASKS;
    el->micro_count++;
    return id;
}

void el_enqueue_microtask(EventLoop *el, TaskFn fn, void *user_data) {
    el_queue_microtask(el, MICRO_QUEUE_MICROTASK, fn, user_data);
}

uint64_t el_request_idle_callback(EventLoop *el, IdleCallbackFn fn, void *user_data) {
    if (el->idle_count >= MAX_IDLE_CALLBACKS) return 0;
    uint64_t id = ++el->task_counter;
    IdleCallbackEntry *icb = &el->idle_callbacks[el->idle_count++];
    icb->id = id;
    icb->fn = fn;
    icb->user_data = user_data;
    icb->registered_time_ms = el->current_time_ms;
    icb->executed = false;
    icb->cancelled = false;
    return id;
}

void el_cancel_idle(EventLoop *el, uint64_t id) {
    for (uint32_t i = 0; i < el->idle_count; i++) {
        if (el->idle_callbacks[i].id == id) {
            el->idle_callbacks[i].cancelled = true;
            return;
        }
    }
}

void el_flush_microtasks(EventLoop *el) {
    uint32_t processed = 0;
    while (el->micro_count > 0) {
        if (el->starvation_prevention && processed >= el->micro_starvation_limit) {
            el->last_task_metrics.is_long_task = true;
            el->long_task_count++;
            break;
        }
        MicroTask *mt = &el->micro_queue[el->micro_head];
        if (mt->fn) mt->fn(mt->user_data);
        mt->complete = true;
        el->micro_head = (el->micro_head + 1) % MAX_MICRO_TASKS;
        el->micro_count--;
        processed++;
        el->total_tasks_processed++;
    }
}

void el_process_one_macro(EventLoop *el, uint64_t now_ms) {
    if (el->macro_count == 0) return;
    MacroTask *t = &el->macro_queue[el->macro_head];
    uint64_t ready_time = t->scheduled_time_ms + t->delay_ms;
    if (now_ms < ready_time) return;

    el->macro_head = (el->macro_head + 1) % MAX_MACRO_TASKS;
    el->macro_count--;

    if (t->state == TASK_CANCELLED) return;
    t->state = TASK_RUNNING;
    t->started_time_ms = now_ms;
    el->last_task_metrics.elapsed_ms = 0;
    el->last_task_metrics.task_type = t->type;
    el->last_task_metrics.task_id = t->id;
    el->last_task_metrics.timestamp_ms = now_ms;

    if (t->fn) t->fn(t->user_data);

    t->completed_time_ms = el->current_time_ms;
    t->state = TASK_COMPLETED;
    el->total_tasks_processed++;

    uint64_t elapsed = t->completed_time_ms - t->started_time_ms;
    el->last_task_metrics.elapsed_ms = elapsed;
    el->last_task_metrics.is_long_task = (elapsed >= LONG_TASK_THRESHOLD_MS);
    if (el->last_task_metrics.is_long_task) {
        el->last_task_metrics.exceeded_by_ms = elapsed - LONG_TASK_THRESHOLD_MS;
        el->long_task_count++;
    }

    if (el->spin_microtasks) el_flush_microtasks(el);

    if (t->is_recurring && t->state == TASK_COMPLETED) {
        uint64_t new_id = el_schedule_task(el, t->type, t->fn, t->user_data, t->delay_ms);
        if (new_id && el->macro_count > 0) {
            uint32_t idx = (el->macro_tail == 0) ? MAX_MACRO_TASKS - 1 : el->macro_tail - 1;
            el->macro_queue[idx].is_recurring = true;
        }
    }
}

void el_tick(EventLoop *el, uint64_t now_ms) {
    el->current_time_ms = now_ms;
    el_process_one_macro(el, now_ms);
    el_flush_microtasks(el);
}

void el_run(EventLoop *el) {
    el->running = true;
    while (el->running) {
        if (el_is_empty(el)) { el->running = false; break; }
        el_tick(el, el->current_time_ms + 1);
    }
}

void el_set_time(EventLoop *el, uint64_t now_ms) {
    el->current_time_ms = now_ms;
}

uint64_t el_now(const EventLoop *el) {
    return el->current_time_ms;
}

void el_enable_long_task_detection(EventLoop *el, bool enable) {
    el->detect_long_tasks = enable;
}

void el_enable_starvation_prevention(EventLoop *el, bool enable) {
    el->starvation_prevention = enable;
}

void el_set_micro_starvation_limit(EventLoop *el, uint32_t limit) {
    el->micro_starvation_limit = limit;
}

TaskMetrics el_get_last_metrics(const EventLoop *el) {
    return el->last_task_metrics;
}

uint64_t el_get_long_task_count(const EventLoop *el) {
    return el->long_task_count;
}

uint64_t el_get_total_processed(const EventLoop *el) {
    return el->total_tasks_processed;
}

bool el_is_empty(const EventLoop *el) {
    if (el->macro_count > 0) return false;
    if (el->micro_count > 0) return false;
    return true;
}

void el_dump_state(const EventLoop *el) {
    printf("[EventLoop] time=%llu macro=%u micro=%u idle=%u long_tasks=%llu total=%llu\n",
           (unsigned long long)el->current_time_ms,
           el->macro_count, el->micro_count, el->idle_count,
           (unsigned long long)el->long_task_count,
           (unsigned long long)el->total_tasks_processed);
    for (uint32_t i = el->macro_head, c = 0; c < el->macro_count && c < 8; c++, i = (i + 1) % MAX_MACRO_TASKS) {
        printf("  macro[%u] id=%llu type=%d state=%d delay=%llu\n",
               c, (unsigned long long)el->macro_queue[i].id,
               el->macro_queue[i].type, el->macro_queue[i].state,
               (unsigned long long)el->macro_queue[i].delay_ms);
    }
}
