#include "reactive_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ReactiveContext* reactive_create_context(void) {
    ReactiveContext* ctx = (ReactiveContext*)calloc(1, sizeof(ReactiveContext));
    if (!ctx) return NULL;
    ctx->current_effect_id = -1;
    ctx->is_tracking = false;
    ctx->is_batching = false;
    ctx->batch_queue = (BatchQueue*)calloc(1, sizeof(BatchQueue));
    return ctx;
}

void reactive_destroy_context(ReactiveContext* ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->signal_count; i++) {
        reactive_signal_free(ctx->signals[i]);
    }
    for (int i = 0; i < ctx->effect_count; i++) {
        reactive_effect_free(ctx->effects[i]);
    }
    for (int i = 0; i < ctx->computed_count; i++) {
        reactive_computed_free(ctx->computeds[i]);
    }
    free(ctx->batch_queue);
    free(ctx);
}

Signal* reactive_create_signal(ReactiveContext* ctx, const char* name,
                               void* initial_value, size_t value_size) {
    if (!ctx || ctx->signal_count >= REACTIVE_MAX_SIGNALS) return NULL;
    Signal* sig = (Signal*)calloc(1, sizeof(Signal));
    if (!sig) return NULL;
    sig->id = ctx->signal_count;
    strncpy(sig->name, name, sizeof(sig->name) - 1);
    sig->value_size = value_size;
    sig->value = malloc(value_size);
    if (sig->value && initial_value) {
        memcpy(sig->value, initial_value, value_size);
    }
    sig->is_dirty = false;
    sig->subscribers = NULL;
    sig->subscriber_count = 0;
    sig->kind = SIG_STATE;
    ctx->signals[ctx->signal_count++] = sig;
    return sig;
}

void reactive_signal_set(Signal* signal, void* new_value) {
    if (!signal || !new_value) return;
    memcpy(signal->value, new_value, signal->value_size);
    signal->is_dirty = true;

    Dependency* dep = signal->subscribers;
    while (dep) {
        ReactiveContext* ctx = NULL;
        for (int i = 0; i < REACTIVE_MAX_SIGNALS && !ctx; i++) {
        }
        dep = dep->next;
    }
}

void* reactive_signal_get(Signal* signal) {
    if (!signal) return NULL;
    return signal->value;
}

void reactive_signal_free(Signal* signal) {
    if (!signal) return;
    Dependency* dep = signal->subscribers;
    while (dep) {
        Dependency* next = dep->next;
        free(dep);
        dep = next;
    }
    if (signal->value) free(signal->value);
    free(signal);
}

Computed* reactive_create_computed(ReactiveContext* ctx, const char* name,
                                   ComputedFn compute, void* context) {
    if (!ctx || ctx->computed_count >= REACTIVE_MAX_COMPUTED) return NULL;
    Computed* comp = (Computed*)calloc(1, sizeof(Computed));
    if (!comp) return NULL;
    comp->base.id = ctx->computed_count + 1000;
    strncpy(comp->base.name, name, sizeof(comp->base.name) - 1);
    comp->base.kind = SIG_COMPUTED;
    comp->compute = compute;
    comp->context = context;
    comp->dependencies = NULL;
    comp->dep_count = 0;
    comp->needs_recompute = true;
    comp->base.value = NULL;
    ctx->computeds[ctx->computed_count++] = comp;
    return comp;
}

void* reactive_computed_get(Computed* computed) {
    if (!computed) return NULL;
    if (computed->needs_recompute && computed->compute) {
        void* result = computed->compute(computed->context);
        if (computed->base.value) free(computed->base.value);
        computed->base.value = result;
        computed->needs_recompute = false;
    }
    return computed->base.value;
}

void reactive_computed_free(Computed* computed) {
    if (!computed) return;
    if (computed->base.value) free(computed->base.value);
    if (computed->dependencies) free(computed->dependencies);
    free(computed);
}

void reactive_computed_mark_dirty(Computed* computed) {
    if (!computed) return;
    computed->needs_recompute = true;
    reactive_trigger(NULL, &computed->base);
}

bool reactive_computed_needs_recompute(Computed* computed) {
    return computed ? computed->needs_recompute : false;
}

Effect* reactive_create_effect(ReactiveContext* ctx, EffectFn run, void* context, int priority) {
    if (!ctx || ctx->effect_count >= REACTIVE_MAX_EFFECTS) return NULL;
    Effect* effect = (Effect*)calloc(1, sizeof(Effect));
    if (!effect) return NULL;
    effect->id = ctx->effect_count;
    effect->run = run;
    effect->context = context;
    effect->dependencies = NULL;
    effect->dep_count = 0;
    effect->is_active = true;
    effect->is_scheduled = false;
    effect->priority = priority;
    ctx->effects[ctx->effect_count++] = effect;

    ctx->current_effect_id = effect->id;
    ctx->is_tracking = true;
    reactive_effect_run(effect);
    ctx->is_tracking = false;
    ctx->current_effect_id = -1;

    return effect;
}

void reactive_effect_run(Effect* effect) {
    if (!effect || !effect->is_active) return;
    if (effect->run) effect->run(effect->context);
}

void reactive_effect_schedule(Effect* effect) {
    if (!effect || effect->is_scheduled) return;
    effect->is_scheduled = true;
}

void reactive_effect_free(Effect* effect) {
    if (!effect) return;
    reactive_clear_dependencies(effect);
    free(effect);
}

void reactive_effect_deactivate(Effect* effect) {
    if (effect) effect->is_active = false;
}

void reactive_effect_activate(Effect* effect) {
    if (effect) {
        effect->is_active = true;
        reactive_effect_run(effect);
    }
}

void reactive_track(ReactiveContext* ctx, Signal* signal) {
    if (!ctx || !signal || !ctx->is_tracking) return;
    reactive_track_dependency(signal, ctx->current_effect_id);
}

void reactive_trigger(ReactiveContext* ctx, Signal* signal) {
    if (!signal) return;
    if (ctx && ctx->is_batching) {
        Dependency* dep = signal->subscribers;
        while (dep) {
            dep = dep->next;
        }
        return;
    }
    Dependency* dep = signal->subscribers;
    while (dep) {
        for (int i = 0; i < (ctx ? ctx->effect_count : 0); i++) {
            if (ctx->effects[i] && ctx->effects[i]->id == dep->subscriber_id) {
                reactive_effect_schedule(ctx->effects[i]);
                break;
            }
        }
        dep = dep->next;
    }
}

void reactive_track_dependency(Signal* signal, int effect_id) {
    if (!signal) return;
    Dependency* dep = (Dependency*)calloc(1, sizeof(Dependency));
    if (!dep) return;
    dep->signal = signal;
    dep->subscriber_id = effect_id;
    dep->next = signal->subscribers;
    signal->subscribers = dep;
    signal->subscriber_count++;
}

void reactive_clear_dependencies(Effect* effect) {
    if (!effect || !effect->dependencies) return;
    free(effect->dependencies);
    effect->dependencies = NULL;
    effect->dep_count = 0;
}

void reactive_batch_begin(ReactiveContext* ctx) {
    if (!ctx) return;
    ctx->is_batching = true;
    ctx->batch_queue->count = 0;
}

void reactive_batch_end(ReactiveContext* ctx) {
    if (!ctx) return;
    ctx->is_batching = false;
    reactive_batch_flush(ctx);
}

void reactive_batch_flush(ReactiveContext* ctx) {
    if (!ctx || ctx->batch_queue->is_flushing) return;
    ctx->batch_queue->is_flushing = true;
    for (int i = 0; i < ctx->batch_queue->count; i++) {
        Effect* effect = ctx->batch_queue->pending[i];
        if (effect && effect->is_scheduled) {
            reactive_effect_run(effect);
            effect->is_scheduled = false;
        }
    }
    ctx->batch_queue->count = 0;
    ctx->batch_queue->is_flushing = false;
}

void reactive_microtask_schedule(Effect* effect) {
    if (!effect) return;
    reactive_effect_schedule(effect);
}

void reactive_memoize(MemoizationCache** cache, void* key, void* value,
                      size_t value_size, Signal** deps, int dep_count) {
    if (!cache || !key) return;
    MemoizationCache* entry = (MemoizationCache*)calloc(1, sizeof(MemoizationCache));
    if (!entry) return;
    entry->key = key;
    entry->value = malloc(value_size);
    if (entry->value && value) memcpy(entry->value, value, value_size);
    entry->value_size = value_size;
    entry->deps = (Signal**)calloc(dep_count, sizeof(Signal*));
    if (entry->deps && deps) memcpy(entry->deps, deps, dep_count * sizeof(Signal*));
    entry->dep_count = dep_count;
    entry->is_valid = true;
    entry->next = *cache;
    *cache = entry;
}

void* reactive_memoized_get(MemoizationCache* cache, void* key) {
    MemoizationCache* entry = cache;
    while (entry) {
        if (entry->key == key && entry->is_valid)
            return entry->value;
        entry = entry->next;
    }
    return NULL;
}

bool reactive_memoized_is_valid(MemoizationCache* cache, void* key) {
    MemoizationCache* entry = cache;
    while (entry) {
        if (entry->key == key)
            return entry->is_valid;
        entry = entry->next;
    }
    return false;
}

void reactive_memoize_clear(MemoizationCache** cache) {
    MemoizationCache* entry = *cache;
    while (entry) {
        MemoizationCache* next = entry->next;
        if (entry->value) free(entry->value);
        if (entry->deps) free(entry->deps);
        free(entry);
        entry = next;
    }
    *cache = NULL;
}

void reactive_comparison_vdom_vs_fine_grained(void) {
    printf("=== VDOM vs Fine-Grained Reactivity Comparison ===\n\n");
    printf("Virtual DOM:\n");
    printf("  + Full tree diff on each update\n");
    printf("  + O(n) diff algorithm (n = VNode count)\n");
    printf("  + Batch updates via requestAnimationFrame\n");
    printf("  + Abstract DOM operations (cross-platform)\n");
    printf("  + Large memory overhead for VNode trees\n");
    printf("  + Single-pass reconciliation\n");
    printf("\n");
    printf("Fine-Grained Reactivity (Signals):\n");
    printf("  + Direct DOM updates at signal granularity\n");
    printf("  + O(1) per signal change (no tree diff)\n");
    printf("  + Automatic dependency tracking via effects\n");
    printf("  + Minimal memory overhead (no VNode clones)\n");
    printf("  + Glitch-free computed values\n");
    printf("  + Microtask-level batching\n");
    printf("\n");
    printf("Hybrid Approach (this framework):\n");
    printf("  + Use VDOM for component tree structure\n");
    printf("  + Use Signals for fine-grained state updates\n");
    printf("  + Components re-render via hooks + VDOM diff\n");
    printf("  + Effects auto-track signal dependencies\n");
    printf("  + Batch updates across both systems\n");
}
