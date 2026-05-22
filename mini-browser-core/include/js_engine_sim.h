#ifndef JS_ENGINE_SIM_H
#define JS_ENGINE_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    JIT_NONE         = 0,
    JIT_IGNITION     = 1,
    JIT_SPARKPLUG    = 2,
    JIT_TURBOFAN     = 3,
    JIT_MAGLEV       = 4
} JITTier;

typedef enum {
    JS_TAG_SMI        = 0,
    JS_TAG_HEAP_NUMBER = 1,
    JS_TAG_STRING     = 2,
    JS_TAG_OBJECT     = 3,
    JS_TAG_SYMBOL     = 4,
    JS_TAG_UNDEFINED  = 5,
    JS_TAG_NULL       = 6,
    JS_TAG_TRUE       = 7,
    JS_TAG_FALSE      = 8,
    JS_TAG_BIGINT     = 9
} JSTag;

typedef enum {
    GC_NONE             = 0,
    GC_SCAVENGE         = 1,
    GC_INCREMENTAL_MARK = 2,
    GC_MARK_COMPACT     = 3,
    GC_FULL             = 4
} GCPhase;

typedef enum {
    IC_UNINITIALIZED  = 0,
    IC_MONOMORPHIC    = 1,
    IC_POLYMORPHIC    = 2,
    IC_MEGAMORPHIC    = 3
} ICState;

typedef struct {
    uint32_t capacity;
    uint32_t count;
    uint32_t transition_count;
} HiddenClassStats;

typedef struct {
    uint64_t   map_hash;
    uint32_t   property_count;
    uint32_t   element_count;
    uint64_t   property_keys[64];
    uint64_t   property_offsets[64];
    bool       is_dictionary_mode;
    uint64_t   prototype_map_hash;
    uint64_t   transition_tree_hash;
} HiddenClass;

typedef struct {
    uint64_t   id;
    HiddenClass *shape;
    uint64_t   properties[64];
    uint64_t   elements[128];
    uint32_t   element_count;
} JSObject;

typedef struct {
    uint64_t  value;
    JSTag     tag;
} JSValue;

typedef struct {
    const char *name;
    uint64_t    call_count;
    ICState     state;
    HiddenClass *monomorphic_shape;
    HiddenClass *polymorphic_shapes[4];
    uint8_t     polymorphic_count;
    uint64_t    monomorphic_hits;
    uint64_t    polymorphic_hits;
    uint64_t    megamorphic_hits;
    uint64_t    miss_count;
} InlineCache;

typedef struct {
    uint64_t    code_hash;
    JITTier     tier;
    uint64_t    compiled_at_ms;
    uint64_t    execution_count;
    uint64_t    deopt_count;
    bool        is_optimized;
    bool        on_stack;
    uint64_t    bytecode_size;
    uint64_t    machine_code_size;
} CodeObject;

typedef struct {
    uint64_t from_space_size;
    uint64_t to_space_size;
    uint64_t old_space_size;
    uint64_t large_object_space_size;
    uint64_t code_space_size;
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t gc_count;
    uint64_t gc_pause_total_us;
    uint64_t last_gc_pause_us;
} MemoryStats;

typedef struct {
    JITTier      current_tier;
    uint64_t     call_count;
    bool         hot_enough_for_sparkplug;
    bool         hot_enough_for_turbofan;
    uint64_t     ignition_threshold;
    uint64_t     sparkplug_threshold;
    uint64_t     turbofan_threshold;
    CodeObject   codes[4];
    InlineCache  ic_cache[32];
    uint32_t     ic_count;
    uint64_t     deopt_reason;
} JITState;

typedef struct {
    GCPhase         current_phase;
    bool            gc_in_progress;
    bool            incremental_marking;
    bool            compaction_in_progress;
    uint8_t        *from_space;
    uint8_t        *to_space;
    uint64_t        from_offset;
    uint64_t        to_offset;
    uint64_t        semi_space_size;
    uint64_t        allocation_pointer;
    uint64_t        allocation_limit;
    uint64_t        survived_objects;
    uint64_t        promoted_objects;
    uint64_t        freed_objects;
    MemoryStats     stats;
} GCState;

typedef struct {
    uint64_t        isolate_id;
    bool            is_main_thread;
    JITState        jit;
    GCState         gc;
    HiddenClass     *root_shape;
    HiddenClass     shapes[256];
    uint32_t        shape_count;
    JSObject        objects[512];
    uint32_t        object_count;
    JSValue         global_scope[1024];
    uint32_t        global_count;
    uint64_t        script_count;
    uint64_t        total_bytecode_size;
    bool            profiling_enabled;
    bool            turbofan_enabled;
    char            last_error[256];
} Isolate;

void js_isolate_init(Isolate *iso, uint64_t id);
void js_isolate_destroy(Isolate *iso);
void js_isolate_set_main_thread(Isolate *iso, bool main);

JSValue js_make_smi(Isolate *iso, int32_t val);
JSValue js_make_heap_number(Isolate *iso, double val);
JSValue js_make_string(Isolate *iso, const char *str);
JSValue js_make_boolean(Isolate *iso, bool val);
JSValue js_make_undefined(void);
JSValue js_make_null(void);

HiddenClass *js_create_shape(Isolate *iso, const char *prop_keys[], uint32_t count);
HiddenClass *js_get_shape(Isolate *iso, uint64_t hash);
JSObject *js_create_object(Isolate *iso, HiddenClass *shape);
void js_set_property(Isolate *iso, JSObject *obj, uint32_t offset, JSValue val);
JSValue js_get_property(const JSObject *obj, uint32_t offset);

void js_jit_on_call(Isolate *iso, uint64_t func_hash);
JITTier js_jit_determine_tier(const JITState *js);
void js_jit_compile(Isolate *iso, uint64_t func_hash, JITTier tier);
void js_jit_deoptimize(Isolate *iso, JITTier from, uint64_t reason);
const char *js_jit_tier_name(JITTier tier);

InlineCache *js_ic_get_or_create(Isolate *iso, const char *name);
ICState js_ic_update(InlineCache *ic, HiddenClass *observed_shape, bool hit);
void js_ic_invalidate(InlineCache *ic);

void js_gc_trigger_scavenge(Isolate *iso);
void js_gc_trigger_mark_compact(Isolate *iso);
void js_gc_trigger_full(Isolate *iso);
void js_gc_step_incremental(Isolate *iso);
bool js_gc_is_in_progress(const GCState *gc);
const char *js_gc_phase_name(GCPhase phase);

const MemoryStats *js_get_memory_stats(const Isolate *iso);
void js_dump_shapes(const Isolate *iso);
void js_dump_ic_state(const Isolate *iso);

#endif
