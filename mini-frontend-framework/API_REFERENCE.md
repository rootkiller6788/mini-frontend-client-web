# Mini Frontend Framework — API Reference

## Virtual DOM (`virtual_dom.h`)

### VNode Types
| Type | Description |
|------|-------------|
| `VNODE_ELEMENT` | HTML/SVG element (`<div>`, `<span>`) |
| `VNODE_TEXT` | Text node |
| `VNODE_FRAGMENT` | Fragment wrapper (no DOM element) |
| `VNODE_COMPONENT` | Component node (calls render function) |

### VNode Lifecycle
```
createElement() ──► setProps() ──► appendChild() ──► diff()
    │                                                    │
    └── clone() ──► free()                               └── patch() ──► DOM
```

### Functions

#### Creation
```c
VNode* vdom_create_element(const char* tag);
VNode* vdom_create_text(const char* text);
VNode* vdom_create_fragment(void);
VNode* vdom_create_component(const char* name, ComponentRender render_fn);
```

#### Property & Child Management
```c
void vdom_set_prop(VNode* node, const char* key, const char* value);
void vdom_set_event(VNode* node, const char* event, const char* handler);
void vdom_append_child(VNode* parent, VNode* child);
void vdom_remove_child(VNode* parent, VNode* child);
void vdom_replace_child(VNode* parent, VNode* old_child, VNode* new_child);
void vdom_set_text(VNode* node, const char* text);
void vdom_set_key(VNode* node, const char* key);
```

#### Diff & Patch
```c
void vdom_diff(VNode* old_tree, VNode* new_tree, void* dom_parent);
void vdom_patch(VNode* old_tree, VNode* new_tree);
void vdom_patch_children(VNode* old_parent, VNode* new_parent, void* dom_parent);
void vdom_patch_props(VNode* old_node, VNode* new_node, void* dom_parent);
void vdom_keyed_reconciliation(VNodeList* old_children, VNodeList* new_children,
                                void* dom_parent, VNode* parent_node);
```

#### Batch Updates
```c
void vdom_batch_begin(void);
void vdom_batch_end(void);
void vdom_batch_schedule(VPatch* patch);
void vdom_batch_flush(void);
bool vdom_batch_is_pending(void);
```

#### Memory
```c
VNode* vdom_clone(VNode* node);
void   vdom_free(VNode* node);
void   vdom_free_recursive(VNode* node);
```

---

## Hooks (`react_hooks.h`)

### Hook Rules
1. Only call hooks at the top level of a component render function
2. Hook call order must be consistent across renders
3. Hooks use a Fiber-like linked list for state preservation

### Hook Types

#### useState
```c
int  hooks_use_state(HookState* state, void* initial_value, void** out_value);
void hooks_set_state(HookState* state, int slot, void* new_value);
void* hooks_get_state_value(HookState* state, int slot);
```
Returns a state slot index. `out_value` receives a pointer to the current value.
Calling `hooks_set_state` triggers a component re-render.

#### useEffect
```c
int hooks_use_effect(HookState* state,
                     EffectCallback mount,
                     EffectCallback update,
                     CleanupCallback cleanup,
                     void** deps, int dep_count);
void hooks_run_effects(HookState* state);
void hooks_run_cleanups(HookState* state);
bool hooks_deps_changed(void** old_deps, void** new_deps, int count);
```
- `mount`: runs on first render
- `update`: runs when dependencies change
- `cleanup`: runs before re-run or unmount
- `deps`: NULL means run every render; empty array means run once

#### useRef
```c
int   hooks_use_ref(HookState* state, void* initial_value);
void* hooks_get_ref(HookState* state, int slot);
void  hooks_set_ref(HookState* state, int slot, void* value);
```
Mutable reference that persists across renders without triggering re-render.

#### useMemo
```c
int   hooks_use_memo(HookState* state, void* (*compute)(void*), void* arg,
                     void** deps, int dep_count, size_t value_size);
void* hooks_get_memo(HookState* state, int slot);
```
Cached computation. Recomputes only when dependencies change.

#### useContext
```c
int   hooks_use_context(HookState* state, int context_id);
void* hooks_get_context_value(HookState* state, int slot);
```
Consumes a context value from the nearest Provider ancestor.

### HookState API
```c
HookState* hooks_create_state(void* component_instance, ReRenderFn rerender);
void       hooks_destroy_state(HookState* state);
void       hooks_begin_render(HookState* state);
void       hooks_end_render(HookState* state);
```

### Fiber
```c
Fiber* fiber_create(void* component, void* props);
void   fiber_free(Fiber* fiber);
void   fiber_link_child(Fiber* parent, Fiber* child);
void   fiber_link_sibling(Fiber* prev, Fiber* sibling);
```

---

## Component Model (`component_model.h`)

### Component Lifecycle
```
create() ──► mount() ──► render() ──► update() ──► unmount()
              │                        │  ▲           │
              └── on_mount()           │  └── hooks_set_state()
                                       └── on_update()
```

### Functions
```c
Component* comp_create(const char* name, ComponentFn render);
void       comp_free(Component* comp);
void       comp_free_tree(Component* comp);

void  comp_set_prop(Component* comp, const char* key, void* value);
void* comp_get_prop(Component* comp, const char* key);

void  comp_mount(Component* comp, void* container);
void  comp_update(Component* comp);
void  comp_unmount(Component* comp);
void  comp_force_update(Component* comp);

void  comp_add_child(Component* parent, Component* child);
void  comp_remove_child(Component* parent, Component* child);

void  comp_set_on_mount(Component* comp, LifecycleCallback cb);
void  comp_set_on_update(Component* comp, LifecycleCallback cb);
void  comp_set_on_unmount(Component* comp, LifecycleCallback cb);
void  comp_set_on_error(Component* comp, ErrorHandler handler);

VNode* comp_render(Component* comp);
```

### Context API
```c
int               context_create(void);
ContextProvider*  context_provider_create(int context_id, void* value, Component* owner);
void*             context_get_value(ContextProvider* root, int context_id);
void              context_provide(ContextProvider* parent, int context_id, void* value);
```

### Error Boundary
```c
ErrorBoundary* error_boundary_create(Component* comp, ErrorHandler handler, VNode* fallback_ui);
bool           error_boundary_handle(ErrorBoundary* eb, void* error);
VNode*         error_boundary_get_fallback(ErrorBoundary* eb);
```

### Suspense
```c
SuspenseFallback* suspense_create(Component* comp, VNode* fallback);
void              suspense_resolve(SuspenseFallback* sf, VNode* resolved);
bool              suspense_is_pending(SuspenseFallback* sf);
VNode*            suspense_get_display(SuspenseFallback* sf);
```

---

## SPA Router (`spa_router.h`)

### Route Configuration
```c
int  router_add_route(Router* router, const char* path, const char* name,
                      RouteMatchType match_type, RouteComponentFn component);
void router_set_not_found(Router* router, const char* path, RouteComponentFn component);
void router_add_guard(Route* route, const char* name, GuardFn check, const char* redirect);
void router_add_nested_route(Route* parent, Route* child);
```

### Route Match Types
| Type | Example | Matches |
|------|---------|---------|
| `ROUTE_EXACT` | `/about` | Only `/about` |
| `ROUTE_PREFIX` | `/admin` | `/admin`, `/admin/users`, ... |
| `ROUTE_PARAM` | `/users/:id` | `/users/42`, `/users/abc` |
| `ROUTE_WILDCARD` | `/posts/*` | `/posts/anything/here` |

### Navigation
```c
void        router_navigate(Router* router, const char* path);
void        router_navigate_replace(Router* router, const char* path);
void        router_back(Router* router);
void        router_forward(Router* router);
RouteMatch  router_match(Router* router, const char* path);
```

### Route Guards
```c
bool    router_run_guards(Route* route, RouteParams* params, void* context);
GuardFn router_guard_before_enter(Router* router, const char* path, GuardFn guard);
```
Guards are checked before navigation. If any guard returns `false`, navigation is cancelled.

### History API
```c
void router_push_state(const char* path);
void router_replace_state(const char* path);
void router_history_back(void);
void router_history_forward(void);
int  router_history_length(void);
```

---

## Reactive System (`reactive_system.h`)

### Architecture
```
Signal ──► track() ──► Effect (auto-collects deps)
  │                        │
  └── trigger() ◄──────────┘
```

### Signals
```c
Signal* reactive_create_signal(ReactiveContext* ctx, const char* name,
                                void* initial_value, size_t value_size);
void    reactive_signal_set(Signal* signal, void* new_value);
void*   reactive_signal_get(Signal* signal);
```

### Computed Signals
```c
Computed* reactive_create_computed(ReactiveContext* ctx, const char* name,
                                    ComputedFn compute, void* context);
void*     reactive_computed_get(Computed* computed);
void      reactive_computed_mark_dirty(Computed* computed);
```

### Effects
```c
Effect* reactive_create_effect(ReactiveContext* ctx, EffectFn run, void* context, int priority);
void    reactive_effect_run(Effect* effect);
void    reactive_effect_schedule(Effect* effect);
void    reactive_effect_deactivate(Effect* effect);
void    reactive_effect_activate(Effect* effect);
```

### Batch Updates
```c
void reactive_batch_begin(ReactiveContext* ctx);
void reactive_batch_end(ReactiveContext* ctx);
void reactive_batch_flush(ReactiveContext* ctx);
```

### Memoization
```c
void  reactive_memoize(MemoizationCache** cache, void* key, void* value,
                       size_t value_size, Signal** deps, int dep_count);
void* reactive_memoized_get(MemoizationCache* cache, void* key);
bool  reactive_memoized_is_valid(MemoizationCache* cache, void* key);
void  reactive_memoize_clear(MemoizationCache** cache);
```

---

## Putting It All Together

```c
#include "virtual_dom.h"
#include "react_hooks.h"
#include "component_model.h"
#include "spa_router.h"
#include "reactive_system.h"

VNode* my_component_render(Component* comp, VNode* props) {
    HookState* hs = comp->hook_state;
    hooks_begin_render(hs);

    void* count_ptr = NULL;
    int slot = hooks_use_state(hs, NULL, &count_ptr);

    hooks_end_render(hs);
    hooks_run_effects(hs);

    VNode* div = vdom_create_element("div");
    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Hello from C99!");
    vdom_append_child(div, h1);
    return div;
}
```
