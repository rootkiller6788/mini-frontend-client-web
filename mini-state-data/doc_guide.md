# Usage Guide & Design Patterns — mini-state-data

## Overview

`mini-state-data` is a C99 library implementing five major frontend state management paradigms. Each module is independent and compiles separately. Link the `.o` files your application needs.

## Getting Started

```c
#include "redux_store.h"
#include "mobx_observable.h"
#include "zustand_sim.h"
#include "query_cache.h"
#include "immutable_store.h"
```

Compile with:
```sh
gcc -std=c99 -Wall -O2 myapp.c redux_store.o mobx_observable.o \
    zustand_sim.o query_cache.o immutable_store.o -o myapp
```

## Pattern 1: Redux — Single Source of Truth

Best for: Complex state logic, undo/redo, middleware pipelines, debugging.

### Minimum Example

```c
typedef struct { int count; } State;

void *my_reducer(void *s, const ReduxAction *a, size_t *sz) {
    State *st = (State *)s;
    if (strcmp(a->type, "INC") == 0) st->count++;
    *sz = sizeof(State);
    return st;
}

int main() {
    State init = {0};
    ReduxReducer r = {my_reducer, NULL, NULL};
    ReduxStore *store = redux_store_create(&init, sizeof(State), r);

    redux_store_dispatch(store, &REDUX_ACTION("INC"));
    redux_store_dispatch(store, &REDUX_ACTION("INC"));

    size_t sz;
    State *cur = redux_store_get_state(store, &sz);
    printf("count=%d\n", cur->count); // 2

    redux_store_destroy(store);
}
```

### Adding Middleware

```c
// Logger: prints every action and state transition
redux_store_add_middleware(store, redux_middleware_logger());

// Thunk: async dispatch
redux_store_add_middleware(store, redux_middleware_thunk());

// Saga: background effects
redux_store_add_middleware(store, redux_middleware_saga(my_saga, NULL));
```

### Time-Travel Debugging

```c
ReduxDevTools *dt = redux_devtools_create(50);

// Record periodically
AppState *s = redux_store_get_state(store, &sz);
redux_devtools_record(dt, s, sz, "after-increment");

// Jump back
redux_devtools_jump_to(store, dt, 2);  // restore snapshot #2

// Browse history
for (int i = 0; i < redux_devtools_history_count(dt); i++) {
    ReduxSnapshot *snap = redux_devtools_get_snapshot(dt, i);
    printf("[%d] %s\n", i, snap->action_type);
}
```

## Pattern 2: MobX — Transparent Reactivity

Best for: Derived values, automatic recalculation, fine-grained change propagation.

### Observables

```c
MobxObservable *name = mobx_observable_create("name", MOBX_STRING("Alice"));
MobxObservable *age  = mobx_observable_create("age", MOBX_INT(30));

mobx_observable_set(name, MOBX_STRING("Bob"));
printf("%s\n", mobx_observable_get(name).data.s);
```

### Computed Values

```c
MobxValue full_info(void *ud) {
    (void)ud;
    // In real code, access observables here to auto-track
    return MOBX_STRING("Bob, 30");
}
MobxComputed *info = mobx_computed_create("info", full_info, NULL);
MobxValue v = mobx_computed_get(info);
```

### Autorun

```c
void render(MobxAutorun *ar, void *ud) {
    (void)ar; (void)ud;
    // This re-runs whenever tracked observables change
    printf("Name changed to: %s\n", mobx_observable_get(name).data.s);
}
MobxAutorun *runner = mobx_autorun_create(render, NULL);
```

### Transactions

Batch multiple mutations so side effects fire once:

```c
MOBX_RAW({
    mobx_observable_set(name, MOBX_STRING("Charlie"));
    mobx_observable_set(age, MOBX_INT(35));
    // Reactions/autoruns fire once after the block
});
```

Or use actions for named boundaries:

```c
MOBX_ACTION_BLOCK("updateProfile", {
    mobx_observable_set(name, MOBX_STRING("Diana"));
});
```

## Pattern 3: Zustand — Lightweight Store

Best for: Simple global state, component subscriptions, immer-like immutable updates.

### Create and Use

```c
typedef struct { int x; int y; } Point;
Point p = {10, 20};

ZustandStore *store = zustand_store_create(&p, sizeof(Point));

// Subscribe
void on_change(void *s, size_t sz, void *ud) {
    Point *pt = (Point *)s;
    printf("Point: (%d,%d)\n", pt->x, pt->y);
}
int h = zustand_store_subscribe(store, on_change, NULL);

// Set state
Point new_p = {30, 40};
zustand_store_set_state(store, &new_p, sizeof(Point), true);
```

### Selective Subscriptions (with Selector)

Avoid unnecessary re-renders:

```c
bool select_x(const void *s, void *out, size_t *sz, void *ud) {
    (void)ud;
    if (out) *(int *)out = ((Point *)s)->x;
    *sz = sizeof(int);
    return true;
}
zustand_store_subscribe_with_selector(store, x_changed, select_x, NULL);
```

### Immer-like Immutable Update

```c
void move_right(void *draft, size_t *sz, void *ud) {
    ((Point *)draft)->x += *(int *)ud;
}
int dx = 5;
size_t out_sz;
void *new_pt = zustand_produce(&p, sizeof(Point), move_right, &dx, &out_sz);
zustand_store_set_state(store, new_pt, out_sz, true);
zustand_produce_free(new_pt);
```

### Slice Pattern

Compose stores from multiple slices:

```c
ZustandSlice slices[] = {
    {"auth", &auth_state, sizeof(auth_state), NULL},
    {"cart", &cart_state, sizeof(cart_state), NULL},
};
ZustandStore *s = zustand_create_with_slices(slices, 2);
```

## Pattern 4: Query Cache — Server State

Best for: API data fetching, caching, auto-refetch, optimistic updates.

### Basic Query

```c
QueryCache *cache = query_cache_create();

QueryOptions opts = {
    .query_key = "users",
    .stale_time_ms = 5000,
    .cache_time_ms = 60000,
    .retry_count = 3,
    .refetch_on_focus = true,
};

QueryEntry *entry = query_cache_fetch(cache, "users", fetch_users, &opts);

size_t sz;
User *users = (User *)query_entry_data(entry, &sz);
int count = (int)(sz / sizeof(User));
```

### Cache Access

```c
// Get cached (no network if valid)
QueryEntry *cached = query_cache_get(cache, "users");

// Check staleness
if (query_entry_is_stale(cached)) {
    query_entry_refetch(cached);
}

// Invalidate
query_cache_invalidate(cache, "users");
query_cache_invalidate_prefix(cache, "user:");  // all user:* keys
```

### Mutations with Optimistic Update

```c
MutationOptions mopts = {
    .key = "add-user",
    .invalidate_keys = "users",
    .optimistic = true,
};

MutationEntry *result = query_cache_mutate(cache, "add", add_user_fn,
                                            &new_user, sizeof(User), &mopts);
if (mutation_entry_status(result) == MUTATION_SUCCESS) {
    User *created = mutation_entry_data(result, &sz);
}
```

### Infinite Query (Pagination)

```c
InfiniteQueryOptions iopts = {
    .query_key = "feed",
    .page_size = 20,
};

InfiniteQuery *iq = infinite_query_create(cache, &iopts, fetch_page_fn);

// Fetch pages on demand
for (int page = 0; infinite_query_has_next_page(iq); page++) {
    infinite_query_fetch_page(iq, page);
}

// Combine all pages
size_t total;
void *all = infinite_query_all_data(iq, &total, &item_count);
```

### Auto-Refetch Triggers

```c
query_cache_on_window_focus(cache);    // user returns to tab
query_cache_on_mount(cache, "key");    // component mounts
query_cache_on_reconnect(cache);       // network restored
```

### Garbage Collection

```c
query_cache_gc(cache);  // remove entries past cache_time
```

## Pattern 5: Immutable Store — Structural Sharing

Best for: Large state trees, efficient change detection, patches, normalization.

### Structural Sharing with Tries

```c
ImmutTrie *t = immut_trie_create();
immut_trie_set(t, "user.name", "Alice", 6);
immut_trie_set(t, "user.age", &(int){30}, sizeof(int));

size_t sz;
char *name = immut_trie_get(t, "user.name", &sz);
```

### Immer Produce (Copy-on-Write)

```c
void update_name(void *draft, size_t *sz, void *ud) {
    // draft is a mutable copy of the base state
    strcpy(((MyState *)draft)->name, (char *)ud);
}
void *new_state = immut_produce(base, sizeof(MyState),
                                 update_name, "NewName", &out_sz);
```

### Immutable Lists

```c
ImmutList *list = immut_list_create();
immut_list_push(list, "first", 6);
immut_list_push(list, "second", 7);
immut_list_push(list, "third", 6);

ImmutList *filtered = immut_list_filter(list, starts_with_f, NULL);
ImmutList *mapped = immut_list_map(list, to_upper_fn, NULL);
```

### Change Detection

```c
if (immut_ref_equal(old_state, new_state)) {
    // no re-render needed
}

if (!immut_shallow_eq(&sl_a, sizeof(a), &sl_b, sizeof(b))) {
    // data changed, re-render
}
```

### Patch Generation (Diff)

```c
char v1[] = "hello world";
char v2[] = "hello big world";

ImmutPatch *patch = immut_diff(v1, strlen(v1), v2, strlen(v2));

// Apply patch
size_t out_sz;
char *result = immut_patch_apply(v1, strlen(v1), patch, &out_sz);

// Invert for undo
ImmutPatch *reverse = immut_patch_invert(patch);
```

### Normalized Store (Entity-Relationship)

```c
NormalizedSchema schemas[] = {
    {"todos", "id", fields, 3},
    {"users", "id", fields, 2},
};
ImmutNormalized *ns = immut_normalized_create(schemas, 2);

// Upsert entities
immut_normalized_upsert(ns, "todos", "1", &todo, sizeof(Todo));

// Get by ID
Todo *t = immut_normalized_get(ns, "todos", "1", &sz);

// Get all IDs
NormalizedIds ids = immut_normalized_all_ids(ns, "todos");
for (int i = 0; i < ids.count; i++) {
    Todo *item = immut_normalized_get(ns, "todos", ids.ids[i], NULL);
}
immut_normalized_free_ids(ids);
```

## Combining Patterns

These modules compose well. Common combinations:

| Scenario | Modules |
|----------|---------|
| To-Do app with undo | Redux + Immutable |
| Real-time dashboard | MobX + Query Cache |
| Form with validation | Zustand + Immutable |
| Social feed | Query Cache + Immutable (normalized) |
| Collaborative editor | Redux + MobX + Immutable (patches) |
| E-commerce cart | Zustand + Query Cache |
| Analytics dashboard | Query Cache + MobX (computed metrics) |

## Memory Management

All modules follow consistent patterns:
- `*_create()` allocates, returns opaque pointer
- `*_destroy()` frees all resources
- States are owned by the store; copies are made on set
- Listeners are stack-allocated or heap-referenced by handle

## Thread Safety

The current implementation is single-threaded by design (C99, no threading model). For multi-threaded use, wrap store access with platform-specific primitives.

## Performance Notes

- Use `zustand_store_subscribe_with_selector` to skip unnecessary renders
- Use `immut_ref_equal` for quick no-change detection
- Set `stale_time_ms` and `cache_time_ms` appropriately for query cache
- Use MobX transactions to batch mutations and avoid cascading re-calculations
- Immutable data structures use structural sharing; old versions share unchanged branches
