#ifndef REACT_HOOKS_H
#define REACT_HOOKS_H

#include <stddef.h>
#include <stdbool.h>

#define HOOKS_MAX_SLOTS     32
#define HOOKS_MAX_DEPS      8
#define HOOKS_MAX_CONTEXTS  16

typedef enum {
    HOOK_STATE,
    HOOK_EFFECT,
    HOOK_REF,
    HOOK_MEMO,
    HOOK_CONTEXT
} HookType;

typedef struct Hook Hook;
typedef struct HookState HookState;
typedef struct HookEffect HookEffect;
typedef struct HookRef HookRef;
typedef struct HookMemo HookMemo;
typedef struct HookContextSlot HookContextSlot;
typedef struct EffectCleanup EffectCleanup;
typedef struct Fiber Fiber;

typedef void (*EffectCallback)(void);
typedef void (*CleanupCallback)(void);
typedef void (*StateUpdater)(void* component, void* new_value);
typedef void (*ReRenderFn)(void* component);

struct EffectCleanup {
    CleanupCallback cleanup;
    EffectCleanup* next;
};

struct HookEffect {
    EffectCallback mount;
    EffectCallback update;
    CleanupCallback cleanup;
    void* deps[HOOKS_MAX_DEPS];
    int dep_count;
    bool has_run;
    EffectCleanup* cleanup_list;
};

struct HookRef {
    void* current;
};

struct HookMemo {
    void* value;
    void* deps[HOOKS_MAX_DEPS];
    int dep_count;
    size_t value_size;
};

struct HookContextSlot {
    int context_id;
    void* value;
};

struct Hook {
    HookType type;
    int index;
    union {
        struct { int slot; void* value; StateUpdater setter; } state;
        HookEffect effect;
        HookRef ref;
        HookMemo memo;
        HookContextSlot context_slot;
    } data;
    Hook* next;
};

struct HookState {
    Hook* head;
    Hook* current;
    int count;
    int cursor;
    bool is_mounting;
    void* component_instance;
    ReRenderFn rerender;
};

struct Fiber {
    HookState* hooks;
    void* component;
    void* props;
    Fiber* child;
    Fiber* sibling;
    Fiber* parent;
    Fiber* alternate;
    unsigned int effect_tag;
    void* state_node;
    void* dom_node;
};

Fiber*     fiber_create(void* component, void* props);
void       fiber_free(Fiber* fiber);
void       fiber_link_child(Fiber* parent, Fiber* child);
void       fiber_link_sibling(Fiber* prev, Fiber* sibling);

HookState* hooks_create_state(void* component_instance, ReRenderFn rerender);
void       hooks_destroy_state(HookState* state);
void       hooks_begin_render(HookState* state);
void       hooks_end_render(HookState* state);
void       hooks_reset_cursor(HookState* state);
Hook*      hooks_get_next(HookState* state);

int        hooks_use_state(HookState* state, void* initial_value, void** out_value);
void       hooks_set_state(HookState* state, int slot, void* new_value);
void*      hooks_get_state_value(HookState* state, int slot);

int        hooks_use_effect(HookState* state, EffectCallback mount, EffectCallback update,
                            CleanupCallback cleanup, void** deps, int dep_count);
void       hooks_run_effects(HookState* state);
void       hooks_run_cleanups(HookState* state);
bool       hooks_deps_changed(void** old_deps, void** new_deps, int count);

int        hooks_use_ref(HookState* state, void* initial_value);
void*      hooks_get_ref(HookState* state, int slot);
void       hooks_set_ref(HookState* state, int slot, void* value);

int        hooks_use_memo(HookState* state, void* (*compute)(void*), void* arg,
                          void** deps, int dep_count, size_t value_size);
void*      hooks_get_memo(HookState* state, int slot);

int        hooks_use_context(HookState* state, int context_id);
void*      hooks_get_context_value(HookState* state, int slot);

void       hooks_assert_order(HookState* state, HookType expected_type, int index);

#endif
