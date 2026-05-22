#ifndef REACTIVE_SYSTEM_H
#define REACTIVE_SYSTEM_H

#include <stddef.h>
#include <stdbool.h>

#define REACTIVE_MAX_SIGNALS    256
#define REACTIVE_MAX_EFFECTS    128
#define REACTIVE_MAX_COMPUTED   64
#define REACTIVE_MAX_DEPS       32

typedef enum {
    SIG_STATE,
    SIG_COMPUTED,
    SIG_EFFECT
} ReactiveType;

typedef struct Signal Signal;
typedef struct Effect Effect;
typedef struct Computed Computed;
typedef struct Dependency Dependency;
typedef struct ReactiveContext ReactiveContext;
typedef struct BatchQueue BatchQueue;
typedef struct MemoizationCache MemoizationCache;

typedef void* (*ComputedFn)(void* ctx);
typedef void  (*EffectFn)(void* ctx);

struct Dependency {
    Signal* signal;
    int subscriber_id;
    Dependency* next;
};

struct Signal {
    int id;
    char name[64];
    void* value;
    size_t value_size;
    bool is_dirty;
    Dependency* subscribers;
    int subscriber_count;
    ReactiveType kind;
};

struct Computed {
    Signal base;
    ComputedFn compute;
    void* context;
    Signal** dependencies;
    int dep_count;
    bool needs_recompute;
};

struct Effect {
    int id;
    EffectFn run;
    void* context;
    Signal** dependencies;
    int dep_count;
    bool is_active;
    bool is_scheduled;
    int priority;
};

struct ReactiveContext {
    Signal* signals[REACTIVE_MAX_SIGNALS];
    int signal_count;
    Effect* effects[REACTIVE_MAX_EFFECTS];
    int effect_count;
    Computed* computeds[REACTIVE_MAX_COMPUTED];
    int computed_count;
    int current_effect_id;
    bool is_tracking;
    bool is_batching;
    BatchQueue* batch_queue;
};

struct BatchQueue {
    Effect* pending[REACTIVE_MAX_EFFECTS];
    int count;
    bool is_flushing;
};

struct MemoizationCache {
    void* key;
    void* value;
    size_t value_size;
    Signal** deps;
    int dep_count;
    bool is_valid;
    MemoizationCache* next;
};

ReactiveContext* reactive_create_context(void);
void             reactive_destroy_context(ReactiveContext* ctx);

Signal*  reactive_create_signal(ReactiveContext* ctx, const char* name, void* initial_value, size_t value_size);
void     reactive_signal_set(Signal* signal, void* new_value);
void*    reactive_signal_get(Signal* signal);
void     reactive_signal_free(Signal* signal);

Computed* reactive_create_computed(ReactiveContext* ctx, const char* name, ComputedFn compute, void* context);
void*      reactive_computed_get(Computed* computed);
void       reactive_computed_free(Computed* computed);
void       reactive_computed_mark_dirty(Computed* computed);
bool       reactive_computed_needs_recompute(Computed* computed);

Effect* reactive_create_effect(ReactiveContext* ctx, EffectFn run, void* context, int priority);
void     reactive_effect_run(Effect* effect);
void     reactive_effect_schedule(Effect* effect);
void     reactive_effect_free(Effect* effect);
void     reactive_effect_deactivate(Effect* effect);
void     reactive_effect_activate(Effect* effect);

void     reactive_track(ReactiveContext* ctx, Signal* signal);
void     reactive_trigger(ReactiveContext* ctx, Signal* signal);
void     reactive_track_dependency(Signal* signal, int effect_id);
void     reactive_clear_dependencies(Effect* effect);

void     reactive_batch_begin(ReactiveContext* ctx);
void     reactive_batch_end(ReactiveContext* ctx);
void     reactive_batch_flush(ReactiveContext* ctx);
void     reactive_microtask_schedule(Effect* effect);

void     reactive_memoize(MemoizationCache** cache, void* key, void* value, size_t value_size,
                          Signal** deps, int dep_count);
void*    reactive_memoized_get(MemoizationCache* cache, void* key);
bool     reactive_memoized_is_valid(MemoizationCache* cache, void* key);
void     reactive_memoize_clear(MemoizationCache** cache);

void     reactive_comparison_vdom_vs_fine_grained(void);

#endif
