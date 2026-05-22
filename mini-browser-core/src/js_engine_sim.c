#include "js_engine_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static uint64_t js_hash_string(const char *s) {
    uint64_t h = 0x100;
    if (!s) return h;
    while (*s) h = h * 31 + (unsigned char)(*s++);
    return h;
}

void js_isolate_init(Isolate *iso, uint64_t id) {
    memset(iso, 0, sizeof(*iso));
    iso->isolate_id = id;
    iso->is_main_thread = true;
    iso->profiling_enabled = true;
    iso->turbofan_enabled = true;
    iso->jit.ignition_threshold = 100;
    iso->jit.sparkplug_threshold = 1000;
    iso->jit.turbofan_threshold = 10000;
    iso->jit.current_tier = JIT_IGNITION;
    iso->gc.semi_space_size = 1024 * 1024;
    iso->gc.allocation_limit = iso->gc.semi_space_size;
}

void js_isolate_destroy(Isolate *iso) {
    memset(iso, 0, sizeof(*iso));
}

void js_isolate_set_main_thread(Isolate *iso, bool main) {
    iso->is_main_thread = main;
}

JSValue js_make_smi(Isolate *iso, int32_t val) {
    (void)iso;
    JSValue v = { .value = (uint64_t)(int64_t)val, .tag = JS_TAG_SMI };
    return v;
}

JSValue js_make_heap_number(Isolate *iso, double val) {
    (void)iso;
    union { double d; uint64_t u; } u;
    u.d = val;
    JSValue v = { .value = u.u, .tag = JS_TAG_HEAP_NUMBER };
    return v;
}

JSValue js_make_string(Isolate *iso, const char *str) {
    JSValue v = { .value = js_hash_string(str), .tag = JS_TAG_STRING };
    (void)iso;
    return v;
}

JSValue js_make_boolean(Isolate *iso, bool val) {
    (void)iso;
    JSValue v = { .value = 0, .tag = val ? JS_TAG_TRUE : JS_TAG_FALSE };
    return v;
}

JSValue js_make_undefined(void) {
    JSValue v = { .value = 0, .tag = JS_TAG_UNDEFINED };
    return v;
}

JSValue js_make_null(void) {
    JSValue v = { .value = 0, .tag = JS_TAG_NULL };
    return v;
}

HiddenClass *js_create_shape(Isolate *iso, const char *prop_keys[], uint32_t count) {
    if (iso->shape_count >= 256) return &iso->shapes[0];
    uint64_t hash = 0x100;
    for (uint32_t i = 0; i < count; i++) hash ^= js_hash_string(prop_keys[i]);
    HiddenClass *hc = &iso->shapes[iso->shape_count++];
    hc->map_hash = hash;
    hc->property_count = count < 64 ? count : 64;
    hc->element_count = 0;
    for (uint32_t i = 0; i < hc->property_count; i++) {
        hc->property_keys[i] = js_hash_string(prop_keys[i]);
        hc->property_offsets[i] = i;
    }
    hc->is_dictionary_mode = (count > 32);
    hc->prototype_map_hash = 0;
    hc->transition_tree_hash = hash;
    return hc;
}

HiddenClass *js_get_shape(Isolate *iso, uint64_t hash) {
    for (uint32_t i = 0; i < iso->shape_count; i++) {
        if (iso->shapes[i].map_hash == hash) return &iso->shapes[i];
    }
    return NULL;
}

JSObject *js_create_object(Isolate *iso, HiddenClass *shape) {
    if (iso->object_count >= 512) return &iso->objects[0];
    JSObject *obj = &iso->objects[iso->object_count++];
    obj->id = iso->object_count;
    obj->shape = shape;
    obj->element_count = 0;
    memset(obj->properties, 0, sizeof(obj->properties));
    memset(obj->elements, 0, sizeof(obj->elements));
    return obj;
}

void js_set_property(Isolate *iso, JSObject *obj, uint32_t offset, JSValue val) {
    (void)iso;
    if (offset < 64) obj->properties[offset] = val.value;
}

JSValue js_get_property(const JSObject *obj, uint32_t offset) {
    JSValue v = { .value = 0, .tag = JS_TAG_UNDEFINED };
    if (offset < 64) {
        v.value = obj->properties[offset];
        v.tag = JS_TAG_OBJECT;
    }
    return v;
}

void js_jit_on_call(Isolate *iso, uint64_t func_hash) {
    JITState *js = &iso->jit;
    js->call_count++;
    if (js->call_count >= js->turbofan_threshold && iso->turbofan_enabled) {
        js->hot_enough_for_turbofan = true;
        if (js->current_tier < JIT_TURBOFAN) js_jit_compile(iso, func_hash, JIT_TURBOFAN);
    } else if (js->call_count >= js->sparkplug_threshold) {
        js->hot_enough_for_sparkplug = true;
        if (js->current_tier < JIT_SPARKPLUG) js_jit_compile(iso, func_hash, JIT_SPARKPLUG);
    }
}

JITTier js_jit_determine_tier(const JITState *js) {
    if (js->hot_enough_for_turbofan) return JIT_TURBOFAN;
    if (js->hot_enough_for_sparkplug) return JIT_SPARKPLUG;
    return JIT_IGNITION;
}

void js_jit_compile(Isolate *iso, uint64_t func_hash, JITTier tier) {
    JITState *js = &iso->jit;
    js->current_tier = tier;
    for (int i = 0; i < 4; i++) {
        if (js->codes[i].code_hash == func_hash && js->codes[i].tier == tier) return;
    }
    for (int i = 0; i < 4; i++) {
        if (js->codes[i].code_hash == 0) {
            js->codes[i].code_hash = func_hash;
            js->codes[i].tier = tier;
            js->codes[i].compiled_at_ms = iso->gc.stats.total_allocated;
            js->codes[i].execution_count = 0;
            js->codes[i].deopt_count = 0;
            js->codes[i].is_optimized = (tier >= JIT_TURBOFAN);
            js->codes[i].on_stack = true;
            js->codes[i].bytecode_size = 512;
            js->codes[i].machine_code_size = (tier == JIT_IGNITION) ? 0 : ((tier == JIT_SPARKPLUG) ? 1024 : 4096);
            return;
        }
    }
}

void js_jit_deoptimize(Isolate *iso, JITTier from, uint64_t reason) {
    JITState *js = &iso->jit;
    js->deopt_reason = reason;
    js->current_tier = JIT_IGNITION;
    for (int i = 0; i < 4; i++) {
        if (js->codes[i].tier == from && js->codes[i].on_stack) {
            js->codes[i].deopt_count++;
            js->codes[i].on_stack = false;
            js->codes[i].is_optimized = false;
        }
    }
    js->hot_enough_for_turbofan = false;
    js->hot_enough_for_sparkplug = false;
    js->call_count = 0;
}

const char *js_jit_tier_name(JITTier tier) {
    static const char *names[] = { "None", "Ignition", "Sparkplug", "TurboFan", "Maglev" };
    return (tier <= 4) ? names[tier] : "?";
}

InlineCache *js_ic_get_or_create(Isolate *iso, const char *name) {
    (void)name;
    if (iso->jit.ic_count < 32) {
        InlineCache *ic = &iso->jit.ic_cache[iso->jit.ic_count++];
        ic->name = name;
        ic->state = IC_UNINITIALIZED;
        ic->call_count = 0;
        return ic;
    }
    return &iso->jit.ic_cache[0];
}

ICState js_ic_update(InlineCache *ic, HiddenClass *observed_shape, bool hit) {
    (void)observed_shape;
    ic->call_count++;
    if (hit) {
        ic->monomorphic_hits++;
        if (ic->state == IC_UNINITIALIZED) ic->state = IC_MONOMORPHIC;
    } else {
        ic->miss_count++;
        if (ic->state == IC_MONOMORPHIC) ic->state = IC_POLYMORPHIC;
        else if (ic->state == IC_POLYMORPHIC && ic->miss_count > 10) ic->state = IC_MEGAMORPHIC;
    }
    return ic->state;
}

void js_ic_invalidate(InlineCache *ic) {
    ic->state = IC_UNINITIALIZED;
    ic->monomorphic_shape = NULL;
    ic->polymorphic_count = 0;
}

void js_gc_trigger_scavenge(Isolate *iso) {
    iso->gc.current_phase = GC_SCAVENGE;
    iso->gc.gc_in_progress = true;
    iso->gc.stats.gc_count++;
    uint64_t survived = iso->gc.allocation_pointer / 4;
    iso->gc.survived_objects = survived;
    iso->gc.freed_objects = iso->gc.allocation_pointer - survived;
    iso->gc.allocation_pointer = survived;
    iso->gc.stats.total_freed += iso->gc.freed_objects;
    iso->gc.gc_in_progress = false;
}

void js_gc_trigger_mark_compact(Isolate *iso) {
    iso->gc.current_phase = GC_MARK_COMPACT;
    iso->gc.gc_in_progress = true;
    iso->gc.compaction_in_progress = true;
    iso->gc.stats.gc_count++;
    uint64_t freed = iso->gc.allocation_pointer / 2;
    iso->gc.freed_objects = freed;
    iso->gc.survived_objects = iso->gc.allocation_pointer - freed;
    iso->gc.promoted_objects = iso->gc.survived_objects / 3;
    iso->gc.allocation_pointer = iso->gc.survived_objects;
    iso->gc.stats.total_freed += freed;
    iso->gc.compaction_in_progress = false;
    iso->gc.gc_in_progress = false;
}

void js_gc_trigger_full(Isolate *iso) {
    iso->gc.current_phase = GC_FULL;
    iso->gc.gc_in_progress = true;
    iso->gc.incremental_marking = true;
    js_gc_trigger_scavenge(iso);
    js_gc_step_incremental(iso);
    iso->gc.incremental_marking = false;
    iso->gc.gc_in_progress = false;
}

void js_gc_step_incremental(Isolate *iso) {
    iso->gc.current_phase = GC_INCREMENTAL_MARK;
    iso->gc.stats.last_gc_pause_us = 160;
    iso->gc.stats.gc_pause_total_us += 160;
}

bool js_gc_is_in_progress(const GCState *gc) {
    return gc->gc_in_progress;
}

const char *js_gc_phase_name(GCPhase phase) {
    static const char *names[] = { "None", "Scavenge", "IncrementalMark", "MarkCompact", "Full" };
    return (phase <= 4) ? names[phase] : "?";
}

const MemoryStats *js_get_memory_stats(const Isolate *iso) {
    return &iso->gc.stats;
}

void js_dump_shapes(const Isolate *iso) {
    printf("[JSEngine] %u shapes registered:\n", iso->shape_count);
    for (uint32_t i = 0; i < iso->shape_count && i < 10; i++) {
        printf("  shape[%u] hash=%llu props=%u dict=%d\n",
               i, (unsigned long long)iso->shapes[i].map_hash,
               iso->shapes[i].property_count,
               iso->shapes[i].is_dictionary_mode);
    }
}

void js_dump_ic_state(const Isolate *iso) {
    printf("[JSEngine] %u inline caches:\n", iso->jit.ic_count);
    for (uint32_t i = 0; i < iso->jit.ic_count; i++) {
        const InlineCache *ic = &iso->jit.ic_cache[i];
        printf("  ic[%u] state=%d calls=%llu mono_hits=%llu poly_hits=%llu mega_hits=%llu misses=%llu\n",
               i, ic->state,
               (unsigned long long)ic->call_count,
               (unsigned long long)ic->monomorphic_hits,
               (unsigned long long)ic->polymorphic_hits,
               (unsigned long long)ic->megamorphic_hits,
               (unsigned long long)ic->miss_count);
    }
}
