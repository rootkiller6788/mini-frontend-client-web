# API Reference — mini-state-data

## redux_store.h

### Store

| Function | Description |
|----------|-------------|
| `redux_store_create(initial, size, reducer)` | Create a Redux store with initial state and reducer |
| `redux_store_destroy(store)` | Free store and all resources |
| `redux_store_get_state(store, &size)` | Get current state tree |
| `redux_store_dispatch(store, action)` | Dispatch an action through middleware chain to reducer |
| `redux_store_add_middleware(store, mw)` | Append middleware to the chain |
| `redux_store_subscribe(store, listener, ud)` | Subscribe to state changes, returns handle |
| `redux_store_unsubscribe(store, handle)` | Remove a listener |

### Actions

```c
ReduxAction a = REDUX_ACTION("INCREMENT");
ReduxAction b = REDUX_ACTION_PAYLOAD("SET", &val, sizeof(int));
redux_action_make("CUSTOM", payload, size);
redux_action_free(&action);
```

### combineReducers

```c
ReduxReducerEntry entries[] = {
    {"todos", todo_reducer},
    {"filter", filter_reducer},
};
ReduxReducer root = redux_combine_reducers(entries, 2);
```

### createSlice

```c
ReduxCaseReducer cases[] = {
    {"counter/increment", increment_handler},
    {"counter/decrement", decrement_handler},
};
ReduxSlice slice = redux_create_slice("counter", &init, sizeof(int),
                                       cases, 2, parent_reducer);
```

### Middleware

| Function | Description |
|----------|-------------|
| `redux_middleware_logger()` | Logs each action and next state to stdout |
| `redux_middleware_thunk()` | Supports thunk-style async dispatch |
| `redux_middleware_saga(saga_fn, ud)` | Simulates redux-saga with generator pattern |

### DevTools

| Function | Description |
|----------|-------------|
| `redux_devtools_create(max)` | Create devtools with history capacity |
| `redux_devtools_record(dt, state, sz, action)` | Record a snapshot |
| `redux_devtools_get_snapshot(dt, idx)` | Get snapshot at index |
| `redux_devtools_time_travel(dt, idx, &sz)` | Get state copy at snapshot |
| `redux_devtools_jump_to(store, dt, idx)` | Replace store state with snapshot |
| `redux_devtools_history_count(dt)` | Number of snapshots |
| `redux_devtools_current_index(dt)` | Current position in history |

## mobx_observable.h

### Observable

```c
MobxObservable *o = mobx_observable_create("name", MOBX_INT(42));
MobxValue v = mobx_observable_get(o);
mobx_observable_set(o, MOBX_STRING("hello"));
mobx_observable_destroy(o);
```

Value constructors: `MOBX_INT(v)`, `MOBX_DOUBLE(v)`, `MOBX_STRING(v)`, `MOBX_PTR(p,s)`, `MOBX_BOOL(v)`

### Autorun

```c
MobxAutorun *ar = mobx_autorun_create(my_fn, userdata);
mobx_autorun_track(ar, observable);
mobx_autorun_schedule(ar);
mobx_autorun_destroy(ar);
```

### Computed

```c
MobxValue compute_impl(void *ud) { return MOBX_INT(100); }
MobxComputed *c = mobx_computed_create("sum", compute_impl, NULL);
MobxValue result = mobx_computed_get(c);
mobx_computed_invalidate(c);
```

### Actions

```c
MOBX_ACTION_BLOCK("myAction", {
    mobx_observable_set(x, MOBX_INT(10));
});
mobx_action_is_running();
```

### Reactions

```c
MobxReaction *r = mobx_reaction_create(track_fn, effect_fn, ud);
mobx_reaction_dispose(r);
mobx_when(predicate, effect, ud);
```

### Observable Collections

**Array:** `mobx_array_create`, `push`, `pop`, `get`, `set`, `insert`, `remove`, `clear`, `length`, `data`

**Map:** `mobx_map_create`, `has`, `get`, `set`, `delete`, `clear`, `size`, `foreach`

**Set:** `mobx_set_create`, `has`, `add`, `delete`, `clear`, `size`, `foreach`

### Transaction

```c
MOBX_RAW({ /* batched mutations */ });
mobx_transaction_begin() / mobx_transaction_end()
mobx_batch_begin() / mobx_batch_end()
```

## zustand_sim.h

### Store

```c
ZustandStore *s = zustand_store_create(&state, sizeof(state));
void *current = zustand_store_get_state(s, &size);
zustand_store_set_state(s, new_data, size, replace);
zustand_store_update(s, updater_fn, userdata);
zustand_store_destroy(s);
```

### Subscriptions

```c
int h = zustand_store_subscribe(s, listener, ud);
int h = zustand_store_subscribe_with_selector(s, listener, selector, ud);
zustand_store_unsubscribe(s, h);
```

### Produce (Immer-like)

```c
void recipe(void *draft, size_t *sz, void *ud) {
    ((MyState *)draft)->value = 42;
}
void *new_state = zustand_produce(base, base_sz, recipe, ud, &out_sz);
```

### Draft API

```c
ZustandDraft *d = zustand_draft_create(base, size);
zustand_draft_set(d, offset, value, size);
zustand_draft_get(d, offset, size);
void *result = zustand_draft_finish(d, &out_size);
zustand_draft_abort(d);
```

### Middleware

```c
ZustandMWConfig configs[] = {
    {ZUSTAND_MW_PERSIST, "my-store", &persist_opts},
    {ZUSTAND_MW_DEVTOOLS, "my-store", NULL},
    {ZUSTAND_MW_LOGGER, NULL, NULL},
};
zustand_apply_middleware(store, configs, count);
```

### Slice Pattern

```c
ZustandSlice slices[] = {
    {"users", &users_state, sizeof(users_state), users_actions},
    {"ui", &ui_state, sizeof(ui_state), NULL},
};
ZustandStore *s = zustand_create_with_slices(slices, 2);
```

### Async Actions

```c
ZustandAsync *a = zustand_async_create(store);
zustand_async_dispatch(a, async_fn, userdata);
zustand_async_is_pending(a, "task-id");
zustand_async_abort(a, "task-id");
```

## query_cache.h

### Cache

```c
QueryCache *c = query_cache_create();
query_cache_set_default_free(c, my_free_fn);
query_cache_set_default_options(c, opts);
query_cache_destroy(c);
```

### Queries

```c
QueryOptions opts = {
    .query_key = "users:list",
    .stale_time_ms = 5000,
    .cache_time_ms = 60000,
    .retry_count = 3,
    .retry_delay_ms = 1000,
    .retry_backoff = true,
    .refetch_on_focus = true,
};
QueryEntry *e = query_cache_fetch(c, "key", fetch_fn, &opts);
QueryEntry *e = query_cache_get(c, "key");
QueryEntry *e = query_cache_prefetch(c, "key", fetch_fn, &opts);
```

### Entry Access

```c
query_entry_key(e), query_entry_status(e), query_entry_data(e, &sz),
query_entry_error(e, &sz), query_entry_is_stale(e),
query_entry_is_fetching(e), query_entry_retry_attempt(e)
query_entry_refetch(e), query_entry_invalidate(e)
```

### Auto-Refetch

```c
query_cache_on_window_focus(cache);
query_cache_on_window_blur(cache);
query_cache_on_mount(cache, "key");
query_cache_on_reconnect(cache);
```

### Mutations

```c
MutationOptions mopts = {
    .key = "create-user",
    .invalidate_keys = "users:list",
    .optimistic = true,
};
MutationEntry *m = query_cache_mutate(c, "key", mutate_fn, vars, sz, &mopts);
mutation_entry_status(m), mutation_entry_data(m, &sz),
mutation_entry_reset(m)
```

### Infinite Query

```c
InfiniteQuery *iq = infinite_query_create(cache, &opts, fetch_fn);
infinite_query_fetch_page(iq, page);
infinite_query_all_data(iq, &total_sz, &count);
infinite_query_page_data(iq, page, &sz);
infinite_query_page_count(iq);
infinite_query_has_next_page(iq);
```

### Utilities

```c
query_cache_now_ms();
query_cache_entry_count(cache);
query_cache_stale_count(cache);
query_cache_gc(cache);
query_cache_invalidate(cache, "key");
query_cache_invalidate_prefix(cache, "prefix");
query_cache_remove(cache, "key");
query_cache_clear(cache);
query_cache_batch_fetch(cache, keys, count);
query_cache_wait_all(cache, keys, count, timeout_ms);
```

## immutable_store.h

### Trie

```c
ImmutTrie *t = immut_trie_create();
immut_trie_set(t, "key", value, size);
void *v = immut_trie_get(t, "key", &size);
immut_trie_has(t, "key"), immut_trie_remove(t, "key");
immut_trie_size(t), immut_trie_foreach(t, fn, ud);
```

### Produce

```c
void *new = immut_produce(base_state, size, recipe_fn, ud, &out_sz);
void *new = immut_produce_nested(base, size, "path", recipe_fn, ud, &out_sz);
```

### Draft

```c
ImmutDraft *d = immut_draft_begin(base, size);
immut_draft_set(d, "path", value, size);
immut_draft_get(d, "path", &size), immut_draft_remove(d, "path");
void *result = immut_draft_commit(d, &size);
immut_draft_abort(d), immut_draft_is_modified(d);
```

### Immutable Map

```c
ImmutMap *m = immut_map_create();
immut_map_set(m, "key", value, size), immut_map_get(m, "key", &size);
immut_map_has(m, "key"), immut_map_remove(m, "key");
immut_map_size(m), immut_map_merge(a, b);
immut_map_set_in(m, path_keys, depth, value, size);
immut_map_get_in(m, path_keys, depth, &size);
immut_map_foreach(m, fn, ud);
```

### Immutable List

```c
ImmutList *l = immut_list_create();
immut_list_push(l, value, size), immut_list_pop(l, &val, &size);
immut_list_get(l, idx, &size), immut_list_set(l, idx, val, size);
immut_list_insert(l, idx, val, size), immut_list_remove(l, idx);
immut_list_concat(a, b), immut_list_slice(l, start, end);
immut_list_size(l), immut_list_foreach(l, fn, ud);
immut_list_map(l, map_fn, ud), immut_list_filter(l, pred, ud);
```

### Patches

```c
ImmutPatch *p = immut_diff(state_a, sz_a, state_b, sz_b);
immut_patch_entry_count(p), immut_patch_get(p, idx);
immut_patch_apply(state, sz, patch, &out_sz);
immut_patch_invert(patch);
immut_diff_free(patch);
```

### Memoization

```c
ImmutMemo *m = immut_memo_create(capacity);
immut_memo_get(m, key, key_sz, &val, &val_sz);
immut_memo_put(m, key, key_sz, val, val_sz);
immut_memo_clear(m);
```

### Equality

```c
immut_ref_equal(a, b)          // pointer equality
immut_quick_equal(a, sa, b, sb) // fast byte compare
immut_shallow_eq(a, sa, b, sb)  // byte-level equality
immut_deep_eq(a, sa, b, sb)     // deep structural equality
immut_hash_jenkins(data, size)  // Jenkins hash
```

### Normalized Store

```c
NormalizedSchema schemas[] = {
    {"todos", "id", fields_array, field_count},
};
ImmutNormalized *ns = immut_normalized_create(schemas, count);
immut_normalized_upsert(ns, "todos", "1", data, size);
immut_normalized_get(ns, "todos", "1", &size);
immut_normalized_remove(ns, "todos", "1");
immut_normalized_all_ids(ns, "todos");
immut_normalized_foreach(ns, "todos", iter_fn, ud);
```

### Store Root

```c
ImmutStore *s = immut_store_create();
immut_store_set_state(s, state, size), immut_store_get_state(s, &size);
immut_store_subscribe(s, fn, ud), immut_store_unsubscribe(s, handle);
immut_store_diff(s, from_ver, to_ver), immut_store_version(s);
```
