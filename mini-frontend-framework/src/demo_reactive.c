#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "reactive_system.h"
#include "virtual_dom.h"
#include "react_hooks.h"
#include "component_model.h"

typedef struct {
    char name[64];
    int value;
} CounterState;

typedef struct {
    char title[128];
    int width;
    int height;
} WindowState;

static void* computed_double(void* ctx) {
    Signal* sig = (Signal*)ctx;
    int val = sig ? *(int*)reactive_signal_get(sig) : 0;
    int* result = malloc(sizeof(int));
    if (result) *result = val * 2;
    return result;
}

static void* computed_sum(void* ctx) {
    Signal** sigs = (Signal**)ctx;
    int a = sigs[0] ? *(int*)reactive_signal_get(sigs[0]) : 0;
    int b = sigs[1] ? *(int*)reactive_signal_get(sigs[1]) : 0;
    int* result = malloc(sizeof(int));
    if (result) *result = a + b;
    return result;
}

static void effect_log_fn(void* ctx) {
    Signal* sig = (Signal*)ctx;
    if (sig) {
        printf("  [Effect] Signal '%s' changed to: %d\n",
               sig->name, *(int*)reactive_signal_get(sig));
    }
}

static void demo_signal_basics(void) {
    printf("\n--- Signal Basics ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_a = 10, init_b = 20;
    Signal* a = reactive_create_signal(ctx, "countA", &init_a, sizeof(int));
    Signal* b = reactive_create_signal(ctx, "countB", &init_b, sizeof(int));

    printf("  Created signal '%s' = %d\n", a->name, *(int*)reactive_signal_get(a));
    printf("  Created signal '%s' = %d\n", b->name, *(int*)reactive_signal_get(b));

    int new_a = 100;
    reactive_signal_set(a, &new_a);
    printf("  After set: '%s' = %d\n", a->name, *(int*)reactive_signal_get(a));

    int new_b = 200;
    reactive_signal_set(b, &new_b);
    printf("  After set: '%s' = %d\n", b->name, *(int*)reactive_signal_get(b));

    reactive_comparison_vdom_vs_fine_grained();

    reactive_destroy_context(ctx);
}

static void demo_computed_signals(void) {
    printf("\n--- Computed Signals ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_x = 5;
    Signal* x = reactive_create_signal(ctx, "x", &init_x, sizeof(int));

    Computed* doubled = reactive_create_computed(ctx, "doubled_x", computed_double, x);
    printf("  Created computed 'doubled_x' from signal 'x'\n");

    printf("  x=%d, doubled_x=%d\n",
           *(int*)reactive_signal_get(x),
           *(int*)reactive_computed_get(doubled));

    int new_x = 10;
    reactive_signal_set(x, &new_x);
    reactive_computed_mark_dirty(doubled);
    printf("  After set x=10: doubled_x=%d (recomputed)\n",
           *(int*)reactive_computed_get(doubled));

    int init_y = 3;
    Signal* y = reactive_create_signal(ctx, "y", &init_y, sizeof(int));

    Signal* sum_deps[2] = {x, y};
    Computed* sum = reactive_create_computed(ctx, "sum_xy", computed_sum, sum_deps);
    printf("  x=%d + y=%d = sum=%d\n", *(int*)reactive_signal_get(x),
           *(int*)reactive_signal_get(y), *(int*)reactive_computed_get(sum));

    int new_y = 7;
    reactive_signal_set(y, &new_y);
    reactive_computed_mark_dirty(sum);
    printf("  After set y=7: sum=%d\n", *(int*)reactive_computed_get(sum));

    reactive_destroy_context(ctx);
}

static void demo_effects_tracking(void) {
    printf("\n--- Effects & Dependency Tracking ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_val = 1;
    Signal* counter = reactive_create_signal(ctx, "counter", &init_val, sizeof(int));

    Effect* eff = reactive_create_effect(ctx, effect_log_fn, counter, 0);
    printf("  Effect created and auto-run (priority=%d)\n", eff->priority);

    printf("  Updating counter signal (triggers effect)...\n");
    for (int i = 2; i <= 5; i++) {
        reactive_signal_set(counter, &i);
        reactive_trigger(ctx, counter);
        printf("    Set counter = %d\n", i);
    }

    printf("  Deactivating effect...\n");
    reactive_effect_deactivate(eff);

    int no_trigger = 99;
    reactive_signal_set(counter, &no_trigger);
    printf("  Set counter = 99 (effect deactivated, no log)\n");

    printf("  Reactivating effect...\n");
    reactive_effect_activate(eff);

    reactive_destroy_context(ctx);
}

static void demo_batch_updates(void) {
    printf("\n--- Batch Updates ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_s1 = 0, init_s2 = 0, init_s3 = 0;
    Signal* s1 = reactive_create_signal(ctx, "batch.s1", &init_s1, sizeof(int));
    Signal* s2 = reactive_create_signal(ctx, "batch.s2", &init_s2, sizeof(int));
    Signal* s3 = reactive_create_signal(ctx, "batch.s3", &init_s3, sizeof(int));

    printf("  Batching 3 signal updates...\n");
    reactive_batch_begin(ctx);

    int v1 = 10, v2 = 20, v3 = 30;
    reactive_signal_set(s1, &v1);
    reactive_signal_set(s2, &v2);
    reactive_signal_set(s3, &v3);
    printf("  Inside batch: s1=%d s2=%d s3=%d (flushed at end)\n",
           *(int*)reactive_signal_get(s1),
           *(int*)reactive_signal_get(s2),
           *(int*)reactive_signal_get(s3));

    reactive_batch_end(ctx);
    printf("  After batch flush: all effects scheduled\n");

    reactive_destroy_context(ctx);
}

static void demo_memoization(void) {
    printf("\n--- Memoization ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_a = 5, init_b = 10;
    Signal* a = reactive_create_signal(ctx, "memo.a", &init_a, sizeof(int));
    Signal* b = reactive_create_signal(ctx, "memo.b", &init_b, sizeof(int));

    MemoizationCache* cache = NULL;

    int result_val = 50;
    Signal* deps[2] = {a, b};
    reactive_memoize(&cache, (void*)"key-1", &result_val, sizeof(int), deps, 2);
    printf("  Memoized key='key-1' with value=%d\n", result_val);

    void* cached = reactive_memoized_get(cache, (void*)"key-1");
    printf("  Cache hit: %s (value=%d)\n",
           cached ? "yes" : "no",
           cached ? *(int*)cached : -1);

    bool valid = reactive_memoized_is_valid(cache, (void*)"key-1");
    printf("  Cache valid: %s\n", valid ? "yes" : "no");

    void* miss = reactive_memoized_get(cache, (void*)"key-2");
    printf("  Cache miss for 'key-2': %s\n", miss ? "found" : "not found");

    reactive_memoize_clear(&cache);
    printf("  Cache cleared\n");

    reactive_destroy_context(ctx);
}

static void demo_reactivity_patterns(void) {
    printf("\n--- Reactivity Patterns ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_count = 0;
    Signal* count = reactive_create_signal(ctx, "count", &init_count, sizeof(int));

    Computed* is_even = reactive_create_computed(ctx, "isEven", (ComputedFn)(void*)NULL, NULL);
    (void)is_even;

    Effect* auto_log = reactive_create_effect(ctx, effect_log_fn, count, 1);
    (void)auto_log;

    printf("  Pattern: Signal -> Computed -> Effect pipeline\n");
    printf("  signal count = %d\n", *(int*)reactive_signal_get(count));

    for (int i = 1; i <= 5; i++) {
        reactive_signal_set(count, &i);
        reactive_trigger(ctx, count);
        printf("  count = %d -> effect triggered\n", i);
    }

    reactive_destroy_context(ctx);
}

static void demo_real_world_scenario(void) {
    printf("\n--- Real-World Reactive Scenario ---\n");

    ReactiveContext* ctx = reactive_create_context();

    WindowState ws = { .title = "My App", .width = 800, .height = 600 };
    Signal* window_state = reactive_create_signal(ctx, "window", &ws, sizeof(WindowState));

    CounterState cs = { .name = "clicks", .value = 0 };
    Signal* counter_state = reactive_create_signal(ctx, "clicks", &cs, sizeof(CounterState));

    printf("  Window: %s (%dx%d)\n", ws.title, ws.width, ws.height);
    printf("  Counter: %s = %d\n", cs.name, cs.value);

    ws.width = 1024;
    ws.height = 768;
    reactive_signal_set(window_state, &ws);
    WindowState* updated_ws = (WindowState*)reactive_signal_get(window_state);
    printf("  Window resized: %dx%d\n", updated_ws->width, updated_ws->height);

    cs.value = 42;
    reactive_signal_set(counter_state, &cs);
    CounterState* updated_cs = (CounterState*)reactive_signal_get(counter_state);
    printf("  Counter updated: %d\n", updated_cs->value);

    reactive_destroy_context(ctx);
}

static void demo_integration_vdom_signals(void) {
    printf("\n--- VDOM + Signals Integration ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_count = 0;
    Signal* count_signal = reactive_create_signal(ctx, "ui.count", &init_count, sizeof(int));

    VNode* ui = vdom_create_element("div");
    vdom_set_prop(ui, "class", "reactive-ui");

    VNode* h2 = vdom_create_element("h2");
    vdom_set_text(h2, "Reactive UI (Signals + VDOM)");
    vdom_append_child(ui, h2);

    VNode* p = vdom_create_element("p");
    char count_text[64];
    snprintf(count_text, sizeof(count_text), "Count from signal: %d",
             *(int*)reactive_signal_get(count_signal));
    vdom_set_text(p, count_text);
    vdom_append_child(ui, p);
    vdom_set_key(p, "count-display");

    printf("  Initial VDOM with signal count=%d\n", *(int*)reactive_signal_get(count_signal));
    printf("  Rendered tree:\n");
    vdom_debug_print(ui, 1);

    int new_count = 5;
    reactive_signal_set(count_signal, &new_count);

    VNode* updated_ui = vdom_create_element("div");
    vdom_set_prop(updated_ui, "class", "reactive-ui");
    h2 = vdom_create_element("h2");
    vdom_set_text(h2, "Reactive UI (Signals + VDOM)");
    vdom_append_child(updated_ui, h2);
    p = vdom_create_element("p");
    snprintf(count_text, sizeof(count_text), "Count from signal: %d",
             *(int*)reactive_signal_get(count_signal));
    vdom_set_text(p, count_text);
    vdom_append_child(updated_ui, p);
    vdom_set_key(p, "count-display");

    printf("  Signal updated to %d, running VDOM diff...\n", *(int*)reactive_signal_get(count_signal));
    vdom_diff(ui, updated_ui, NULL);
    printf("  Diff complete (text node patched)\n");

    vdom_free_recursive(ui);
    vdom_free_recursive(updated_ui);
    reactive_destroy_context(ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  Mini Frontend Framework - Reactive Demo\n");
    printf("========================================\n");

    demo_signal_basics();
    demo_computed_signals();
    demo_effects_tracking();
    demo_batch_updates();
    demo_memoization();
    demo_reactivity_patterns();
    demo_real_world_scenario();
    demo_integration_vdom_signals();

    printf("\n========================================\n");
    printf("  Reactive Demo Completed\n");
    printf("========================================\n");
    return 0;
}
