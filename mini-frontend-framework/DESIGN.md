# Mini Frontend Framework — Design Document

## 1. Overview

`mini-frontend-framework` is a C99 implementation of modern frontend architecture patterns:
Virtual DOM, Hooks, Component Model, SPA Router, and Reactive Signals.

### Goals
- Demonstrate frontend concepts in a systems language
- Zero external dependencies (pure C99 + standard library)
- Modular design: each module is independently usable
- Educational: shows how frameworks like React, Vue, SolidJS work internally

### Non-Goals
- Real DOM manipulation (C has no browser DOM)
- CSS/HTML parsing engine
- JS/WASM compilation target (pure native C)

---

## 2. Architecture

```
┌──────────────────────────────────────────────────────┐
│                   Application Code                   │
├──────────────────────────────────────────────────────┤
│  Component Model  │  SPA Router  │  Reactive System  │
├───────────────────┴──────────────┴───────────────────┤
│                  React Hooks (State)                 │
├──────────────────────────────────────────────────────┤
│              Virtual DOM (Rendering)                 │
├──────────────────────────────────────────────────────┤
│                  C99 Standard Library                │
└──────────────────────────────────────────────────────┘
```

### Module Responsibilities

| Module | Role |
|--------|------|
| `virtual_dom` | Tree creation, diffing, patching, batching |
| `react_hooks` | Stateful logic, effects, refs, memos, context |
| `component_model` | Component instances, lifecycle, tree management |
| `spa_router` | URL routing, history, guards, nested routes |
| `reactive_system` | Signals, computed values, effects, memoization |

---

## 3. Virtual DOM Design

### Data Structures
```
VNode (variable size via arrays)
├── type: element|text|fragment|component
├── tag[32]: element/component name
├── text[1024]: text content
├── key[64]: reconciliation key
├── props[64]: fixed-size prop array
├── children[256]: fixed-size child pointer array
└── parent, component, render_fn, dom_ref, flags
```

### Diff Algorithm

The diff follows React's reconciliation strategy:

1. **Type Check**: If node types or tags differ, replace entirely
2. **Text Update**: If both are text nodes, compare and update text
3. **Props Diff**: 
   - Remove props not in new node
   - Update changed prop values
   - Add new props
4. **Children Reconciliation** (keyed):
   - First pass: match by position for same-key nodes
   - Second pass: find moved nodes by key
   - Third pass: remove extra old children
   - Fourth pass: insert new children

### Patching

Patches are generated during diff and can be:
- Applied immediately (`vdom_patch_apply`)
- Batched for later (`vdom_batch_schedule`)
- Reordered for optimization (reorder patches first)

---

## 4. Hooks Design

### Fiber-like Architecture
```
Fiber (linked tree)
├── component pointer
├── props
├── child / sibling / parent (linked list tree)
├── alternate (work-in-progress)
├── hooks (HookState linked list)
├── effect_tag (side effect flags)
└── state_node / dom_node
```

### Hook State Machine
```
Mount Phase:    allocate new hooks → set initial values → run effects
Update Phase:   advance cursor through existing hooks → check deps → re-run
Unmount Phase:  run cleanups → free hook memory
```

### Hook Rules Enforcement
- Hook count is tracked; re-renders with different hook count trigger assertion
- `hooks_assert_order()` validates hook type at each cursor position
- Hooks are stored in insertion-order linked list, traversed sequentially

---

## 5. Component Model Design

### Component Tree
```
Component
├── name, id (unique)
├── render function pointer
├── props (key-value store)
├── vnode cache (last render result)
├── hook_state (per-instance)
├── lifecycle callbacks (mount/update/unmount/error)
├── parent/children tree links
└── flags (mounted, needs_update, ...)
```

### Lifecycle Sequence
```
comp_create()          — allocate, assign ID
comp_set_prop()        — configure props
comp_mount()           — set is_mounted, call on_mount, trigger render
comp_render()          — call render fn, store VNode
comp_update()          — re-render, diff against cached VNode
comp_force_update()    — skip needs_update check
comp_unmount()         — call on_unmount, clean hooks, free VNode
comp_free()            — final cleanup
```

### Context Propagation
Context values flow down the component tree via `ContextProvider` chain:
```
Root Provider (ctx_id=1, value="dark")
  └── Intermediate Component
        └── Nested Provider (ctx_id=1, value="light")  ← overrides
              └── Consumer → gets "light"
```

---

## 6. SPA Router Design

### Route Matching
```
/                            ROUTE_EXACT      → home_page
/about                       ROUTE_EXACT      → about_page
/users                       ROUTE_EXACT      → users_list_page
/users/:id                   ROUTE_PARAM      → user_detail_page  (params: id)
/admin                       ROUTE_PREFIX     → admin_layout
/admin/dashboard             ROUTE_EXACT      → admin_dashboard  (nested)
/posts/*                     ROUTE_WILDCARD   → posts_catchall
*                            (not_found)      → not_found_page
```

### Navigation Flow
```
router_navigate(path)
  ├── Save current path
  ├── Match route (try exact → param → prefix → wildcard → 404)
  ├── Run guards (beforeEnter)
  │   ├── Pass → continue
  │   └── Fail → cancel navigation
  ├── Render component with params
  └── Update history (pushState or hash)
```

### Guard System
```c
bool auth_guard(void* params, void* context) {
    return is_authenticated(context);  // return false to block
}
```

---

## 7. Reactive System Design

### Signal
```
Signal
├── id, name
├── value (void*, value_size)
├── is_dirty flag
├── subscribers (linked list of Dependency)
└── kind: state|computed
```

### Tracking & Triggering
```
Effect runs → sets current_effect_id → signal.get() → reactive_track()
                                                         │
                                              ┌──────────┘
                                              ▼
                                         Dependency (signal, effect_id)
                                              │
signal.set() ──► reactive_trigger() ─────────┘
                         │
                         ▼
              Iterate subscribers → schedule effects
```

### Computed Values
Computed signals are lazily evaluated:
1. Mark dirty when dependencies change
2. Recompute on next `get()`
3. Cache result until next invalidation

### Batch Processing
```
reactive_batch_begin()
  signal.set(a, 10)    ← no trigger yet
  signal.set(b, 20)    ← no trigger yet
  signal.set(c, 30)    ← no trigger yet
reactive_batch_end()
  → flush all pending effects at once
```

---

## 8. VDOM vs Fine-Grained Reactivity

| Aspect | Virtual DOM | Fine-Grained (Signals) |
|--------|-------------|----------------------|
| Update granularity | Full tree diff | Per-signal |
| Memory | O(n) VNode copies | O(1) per signal |
| Performance | O(n) diff + O(m) patches | O(1) per change |
| Predictability | Deterministic diff | Dependency graph |
| Glitch freedom | Inherent (single pass) | Requires scheduling |
| Debugging | Easier (declarative) | Harder (push-based) |

This framework provides **both** systems. Choose based on use case:
- **VDOM**: Complex component trees, server-side rendering
- **Signals**: High-frequency updates, animations, real-time data

---

## 9. Memory Management

All allocations use `calloc`/`malloc` with explicit `free`:
- `vdom_free()` — single node
- `vdom_free_recursive()` — node + all children
- `comp_free()` — component + hooks + vnode
- `comp_free_tree()` — component + all descendants
- `context_provider_free()` — provider chain
- `reactive_destroy_context()` — all signals, effects, computeds

No garbage collection. No reference counting. Manual ownership.

---

## 10. Portability

- **C99 standard**: Uses `stdbool.h`, `stddef.h`, `stdint.h`
- **No platform-specific APIs**: All DOM operations are stubs
- **No dynamic linking**: Pure static compilation
- **Thread safety**: Not guaranteed (single-threaded design)
- **Endianness**: Agnostic (all data is byte-copyable)

---

## 11. Future Extensions

- [ ] JSX-like C macro DSL for VNode creation
- [ ] Server-side rendering (HTML string output)
- [ ] Concurrent mode (time-sliced rendering)
- [ ] CSS-in-C (style object to string)
- [ ] Form state management (useForm hook)
- [ ] DevTools protocol (component inspector)
- [ ] Hot module replacement
