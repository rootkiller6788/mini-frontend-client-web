#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "js_engine_sim.h"

static void demo_object_creation(Isolate *iso) {
    printf("--- Object creation & hidden classes ---\n");

    const char *shapeA_keys[] = { "x", "y" };
    const char *shapeB_keys[] = { "x", "y", "z" };

    HiddenClass *shapeA = js_create_shape(iso, shapeA_keys, 2);
    HiddenClass *shapeB = js_create_shape(iso, shapeB_keys, 3);

    JSObject *obj1 = js_create_object(iso, shapeA);
    JSObject *obj2 = js_create_object(iso, shapeA);
    JSObject *obj3 = js_create_object(iso, shapeB);

    js_set_property(iso, obj1, 0, js_make_smi(iso, 42));
    js_set_property(iso, obj1, 1, js_make_smi(iso, 99));
    js_set_property(iso, obj3, 2, js_make_heap_number(iso, 3.14));

    printf("  obj1 shape hash: %llu (props=%u)\n",
           (unsigned long long)obj1->shape->map_hash, obj1->shape->property_count);
    printf("  obj2 shape hash: %llu (same shape as obj1: %s)\n",
           (unsigned long long)obj2->shape->map_hash,
           obj1->shape == obj2->shape ? "yes" : "no");
    printf("  obj3 shape hash: %llu (props=%u)\n",
           (unsigned long long)obj3->shape->map_hash, obj3->shape->property_count);
}

static void demo_jit_compilation(Isolate *iso) {
    printf("\n--- JIT compilation tiers ---\n");

    JITState *js = &iso->jit;
    printf("  Initial tier: %s\n", js_jit_tier_name(js->current_tier));

    for (int i = 0; i < 105; i++) {
        js_jit_on_call(iso, 0xDEADBEEF);
        if (i == 99) printf("  After %d calls: tier=%s\n", i, js_jit_tier_name(js->current_tier));
    }
    printf("  After 105 calls: tier=%s (hot_for_sparkplug=%d hot_for_turbofan=%d)\n",
           js_jit_tier_name(js->current_tier),
           js->hot_enough_for_sparkplug, js->hot_enough_for_turbofan);

    for (int i = 0; i < 900; i++) js_jit_on_call(iso, 0xDEADBEEF);
    printf("  After more calls: tier=%s (hot_for_turbofan=%d)\n",
           js_jit_tier_name(js->current_tier), js->hot_enough_for_turbofan);

    js_jit_deoptimize(iso, JIT_TURBOFAN, 0xF00B);
    printf("  After deopt: tier=%s deopt_reason=0x%llX\n",
           js_jit_tier_name(js->current_tier), (unsigned long long)js->deopt_reason);
}

static void demo_inline_caching(Isolate *iso) {
    printf("\n--- Inline Caching ---\n");

    InlineCache *ic = js_ic_get_or_create(iso, "getElementById");
    printf("  IC '%s' initial state: %d\n", ic->name, ic->state);

    const char *sk[] = { "id", "class" };
    HiddenClass *shape = js_create_shape(iso, sk, 2);

    for (int i = 0; i < 5; i++)  js_ic_update(ic, shape, true);
    printf("  After 5 monomorphic hits: state=%d hits=%llu\n", ic->state,
           (unsigned long long)ic->monomorphic_hits);

    for (int i = 0; i < 15; i++) js_ic_update(ic, shape, false);
    printf("  After 15 misses: state=%d misses=%llu\n", ic->state,
           (unsigned long long)ic->miss_count);
}

static void demo_gc(Isolate *iso) {
    printf("\n--- Garbage Collection ---\n");

    iso->gc.allocation_pointer = 500 * 1024;
    printf("  Before scavenge: allocated=%llu\n",
           (unsigned long long)iso->gc.allocation_pointer);

    js_gc_trigger_scavenge(iso);
    printf("  After scavenge: allocated=%llu survived=%llu freed=%llu\n",
           (unsigned long long)iso->gc.allocation_pointer,
           (unsigned long long)iso->gc.survived_objects,
           (unsigned long long)iso->gc.freed_objects);

    js_gc_trigger_mark_compact(iso);
    printf("  After mark-compact: allocated=%llu promoted=%llu\n",
           (unsigned long long)iso->gc.allocation_pointer,
           (unsigned long long)iso->gc.promoted_objects);

    const MemoryStats *ms = js_get_memory_stats(iso);
    printf("  Memory stats: gc_count=%llu pause_total=%lluus last=%lluus\n",
           (unsigned long long)ms->gc_count,
           (unsigned long long)ms->gc_pause_total_us,
           (unsigned long long)ms->last_gc_pause_us);
}

int main(void) {
    printf("=== mini-browser-core Demo 2: V8 Engine Simulation ===\n\n");

    Isolate iso;
    js_isolate_init(&iso, 1);
    js_isolate_set_main_thread(&iso, true);

    demo_object_creation(&iso);
    demo_jit_compilation(&iso);
    demo_inline_caching(&iso);
    demo_gc(&iso);

    printf("\n--- Dump shapes ---\n");
    js_dump_shapes(&iso);

    printf("\n--- Dump inline caches ---\n");
    js_dump_ic_state(&iso);

    js_isolate_destroy(&iso);
    printf("\nDone.\n");
    return 0;
}
