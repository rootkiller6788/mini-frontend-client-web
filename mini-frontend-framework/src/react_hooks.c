#include "react_hooks.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static Hook* hook_allocate(HookState* state, HookType type) {
    Hook* hook = (Hook*)calloc(1, sizeof(Hook));
    if (!hook) return NULL;
    hook->type = type;
    hook->index = state->count;
    hook->next = NULL;

    if (!state->head) {
        state->head = hook;
        state->current = hook;
    } else {
        Hook* last = state->head;
        while (last->next) last = last->next;
        last->next = hook;
    }
    state->count++;
    return hook;
}

Fiber* fiber_create(void* component, void* props) {
    Fiber* fiber = (Fiber*)calloc(1, sizeof(Fiber));
    if (!fiber) return NULL;
    fiber->component = component;
    fiber->props = props;
    fiber->hooks = NULL;
    return fiber;
}

void fiber_free(Fiber* fiber) {
    if (!fiber) return;
    if (fiber->hooks) hooks_destroy_state(fiber->hooks);
    free(fiber);
}

void fiber_link_child(Fiber* parent, Fiber* child) {
    if (!parent || !child) return;
    child->parent = parent;
    if (!parent->child) {
        parent->child = child;
    } else {
        Fiber* last = parent->child;
        while (last->sibling) last = last->sibling;
        last->sibling = child;
    }
}

void fiber_link_sibling(Fiber* prev, Fiber* sibling) {
    if (!prev || !sibling) return;
    prev->sibling = sibling;
    sibling->parent = prev->parent;
}

HookState* hooks_create_state(void* component_instance, ReRenderFn rerender) {
    HookState* state = (HookState*)calloc(1, sizeof(HookState));
    if (!state) return NULL;
    state->component_instance = component_instance;
    state->rerender = rerender;
    state->is_mounting = true;
    state->cursor = 0;
    state->count = 0;
    return state;
}

void hooks_destroy_state(HookState* state) {
    if (!state) return;
    Hook* hook = state->head;
    while (hook) {
        Hook* next = hook->next;
        if (hook->type == HOOK_MEMO && hook->data.memo.value) {
            free(hook->data.memo.value);
        }
        free(hook);
        hook = next;
    }
    free(state);
}

void hooks_begin_render(HookState* state) {
    if (!state) return;
    state->cursor = 0;
    state->current = state->head;
}

void hooks_end_render(HookState* state) {
    if (!state) return;
    state->is_mounting = false;
}

void hooks_reset_cursor(HookState* state) {
    if (!state) return;
    state->cursor = 0;
    state->current = state->head;
}

Hook* hooks_get_next(HookState* state) {
    if (!state) return NULL;
    if (state->cursor < state->count && state->current) {
        Hook* hook = state->current;
        state->current = state->current->next;
        state->cursor++;
        return hook;
    }
    return NULL;
}

int hooks_use_state(HookState* state, void* initial_value, void** out_value) {
    if (!state) return -1;
    Hook* existing = hooks_get_next(state);
    Hook* hook;
    if (existing && existing->type == HOOK_STATE) {
        hook = existing;
        if (out_value) *out_value = hook->data.state.value;
    } else {
        hook = hook_allocate(state, HOOK_STATE);
        if (!hook) return -1;
        hook->data.state.slot = hook->index;
        hook->data.state.value = initial_value;
        if (out_value) *out_value = initial_value;
    }
    return hook->data.state.slot;
}

void hooks_set_state(HookState* state, int slot, void* new_value) {
    if (!state) return;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_STATE && hook->data.state.slot == slot) {
            hook->data.state.value = new_value;
            if (state->rerender) {
                state->rerender(state->component_instance);
            }
            return;
        }
        hook = hook->next;
    }
}

void* hooks_get_state_value(HookState* state, int slot) {
    if (!state) return NULL;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_STATE && hook->data.state.slot == slot)
            return hook->data.state.value;
        hook = hook->next;
    }
    return NULL;
}

int hooks_use_effect(HookState* state, EffectCallback mount, EffectCallback update,
                     CleanupCallback cleanup, void** deps, int dep_count) {
    if (!state) return -1;
    Hook* existing = hooks_get_next(state);
    Hook* hook;
    if (existing && existing->type == HOOK_EFFECT) {
        hook = existing;
        bool changed = hooks_deps_changed(hook->data.effect.deps, deps, dep_count);
        if (changed) {
            if (hook->data.effect.cleanup && hook->data.effect.has_run) {
                hook->data.effect.cleanup();
            }
            if (dep_count > 0) {
                memcpy(hook->data.effect.deps, deps, dep_count * sizeof(void*));
                hook->data.effect.dep_count = dep_count;
            }
            hook->data.effect.has_run = false;
            hook->data.effect.update = update;
        }
    } else {
        hook = hook_allocate(state, HOOK_EFFECT);
        if (!hook) return -1;
        hook->data.effect.mount = mount;
        hook->data.effect.update = update;
        hook->data.effect.cleanup = cleanup;
        hook->data.effect.has_run = false;
        hook->data.effect.dep_count = 0;
        hook->data.effect.cleanup_list = NULL;
    }
    return hook->index;
}

void hooks_run_effects(HookState* state) {
    if (!state) return;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_EFFECT && !hook->data.effect.has_run) {
            if (hook->data.effect.mount)
                hook->data.effect.mount();
            if (hook->data.effect.update)
                hook->data.effect.update();
            hook->data.effect.has_run = true;
        }
        hook = hook->next;
    }
}

void hooks_run_cleanups(HookState* state) {
    if (!state) return;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_EFFECT && hook->data.effect.has_run) {
            if (hook->data.effect.cleanup) {
                hook->data.effect.cleanup();
            }
            hook->data.effect.has_run = false;
        }
        hook = hook->next;
    }
}

bool hooks_deps_changed(void** old_deps, void** new_deps, int count) {
    if (!old_deps || !new_deps) return true;
    for (int i = 0; i < count; i++) {
        if (old_deps[i] != new_deps[i]) return true;
    }
    return false;
}

int hooks_use_ref(HookState* state, void* initial_value) {
    if (!state) return -1;
    Hook* existing = hooks_get_next(state);
    Hook* hook;
    if (existing && existing->type == HOOK_REF) {
        hook = existing;
    } else {
        hook = hook_allocate(state, HOOK_REF);
        if (!hook) return -1;
        hook->data.ref.current = initial_value;
    }
    return hook->index;
}

void* hooks_get_ref(HookState* state, int slot) {
    if (!state) return NULL;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_REF && hook->index == slot)
            return hook->data.ref.current;
        hook = hook->next;
    }
    return NULL;
}

void hooks_set_ref(HookState* state, int slot, void* value) {
    if (!state) return;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_REF && hook->index == slot) {
            hook->data.ref.current = value;
            return;
        }
        hook = hook->next;
    }
}

int hooks_use_memo(HookState* state, void* (*compute)(void*), void* arg,
                   void** deps, int dep_count, size_t value_size) {
    if (!state) return -1;
    Hook* existing = hooks_get_next(state);
    Hook* hook;
    if (existing && existing->type == HOOK_MEMO) {
        hook = existing;
        if (hooks_deps_changed(hook->data.memo.deps, deps, dep_count)) {
            if (hook->data.memo.value) free(hook->data.memo.value);
            hook->data.memo.value = compute(arg);
            hook->data.memo.value_size = value_size;
            memcpy(hook->data.memo.deps, deps, dep_count * sizeof(void*));
            hook->data.memo.dep_count = dep_count;
        }
    } else {
        hook = hook_allocate(state, HOOK_MEMO);
        if (!hook) return -1;
        hook->data.memo.value = compute(arg);
        hook->data.memo.value_size = value_size;
        if (deps && dep_count > 0) {
            memcpy(hook->data.memo.deps, deps, dep_count * sizeof(void*));
            hook->data.memo.dep_count = dep_count;
        }
    }
    return hook->index;
}

void* hooks_get_memo(HookState* state, int slot) {
    if (!state) return NULL;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_MEMO && hook->index == slot)
            return hook->data.memo.value;
        hook = hook->next;
    }
    return NULL;
}

int hooks_use_context(HookState* state, int context_id) {
    if (!state) return -1;
    Hook* existing = hooks_get_next(state);
    Hook* hook;
    if (existing && existing->type == HOOK_CONTEXT) {
        hook = existing;
    } else {
        hook = hook_allocate(state, HOOK_CONTEXT);
        if (!hook) return -1;
        hook->data.context_slot.context_id = context_id;
        hook->data.context_slot.value = NULL;
    }
    return hook->index;
}

void* hooks_get_context_value(HookState* state, int slot) {
    if (!state) return NULL;
    Hook* hook = state->head;
    while (hook) {
        if (hook->type == HOOK_CONTEXT && hook->index == slot)
            return hook->data.context_slot.value;
        hook = hook->next;
    }
    return NULL;
}

void hooks_assert_order(HookState* state, HookType expected_type, int index) {
    if (!state) return;
    Hook* hook = state->head;
    int current = 0;
    while (hook && current < index) {
        hook = hook->next;
        current++;
    }
    if (hook && hook->type != expected_type) {
        fprintf(stderr, "Hook order violation: expected %d, got %d at index %d\n",
                expected_type, hook->type, index);
    }
}
