#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "redux_store.h"
#include "mobx_observable.h"
#include "zustand_sim.h"
#include "immutable_store.h"

/* ══════════════════════════════════════════════════════════════════════════
   Demo: Full Redux Stack — Store, Middleware, DevTools, Time Travel
   ══════════════════════════════════════════════════════════════════════════ */

/* ── App State ──────────────────────────────────────────────────────────── */

typedef struct {
    int    counter;
    char   message[256];
    int    history[64];
    int    history_len;
    bool   loading;
} AppState;

/* ── Counter Reducer ────────────────────────────────────────────────────── */

static void *counter_reducer(void *state, const ReduxAction *action,
                             size_t *out_size)
{
    AppState *s = (AppState *)state;
    if (!s) return NULL;

    if (strcmp(action->type, "INCREMENT") == 0) {
        s->counter++;
        s->history[s->history_len % 64] = s->counter;
        s->history_len++;
        snprintf(s->message, sizeof(s->message), "Incremented to %d", s->counter);
    } else if (strcmp(action->type, "DECREMENT") == 0) {
        s->counter--;
        s->history[s->history_len % 64] = s->counter;
        s->history_len++;
        snprintf(s->message, sizeof(s->message), "Decremented to %d", s->counter);
    } else if (strcmp(action->type, "SET_COUNTER") == 0) {
        if (action->payload && action->payload_size == sizeof(int)) {
            s->counter = *(int *)action->payload;
            snprintf(s->message, sizeof(s->message), "Set to %d", s->counter);
        }
    } else if (strcmp(action->type, "RESET") == 0) {
        s->counter = 0;
        s->history_len = 0;
        snprintf(s->message, sizeof(s->message), "Reset");
    } else if (strcmp(action->type, "LOADING") == 0) {
        s->loading = action->payload ? *(bool *)action->payload : !s->loading;
        snprintf(s->message, sizeof(s->message), "Loading: %s",
                 s->loading ? "on" : "off");
    }

    *out_size = sizeof(AppState);
    return s;
}

/* ── Listener ───────────────────────────────────────────────────────────── */

static void on_state_change(void *new_state, size_t size, void *userdata)
{
    AppState *s = (AppState *)new_state;
    const char *label = (const char *)userdata;
    printf("  [%s] counter=%d, message='%s', loading=%s\n",
           label, s->counter, s->message, s->loading ? "yes" : "no");
    (void)size;
}

/* ── Saga simulation ────────────────────────────────────────────────────── */

static void saga_sim(ReduxSagaCtx *ctx)
{
    (void)ctx;
    printf("  [saga] Long-running saga simulating background work...\n");
}

/* ── Helper: print history ──────────────────────────────────────────────── */

static void print_history(const char *title, int *history, int len, int max)
{
    printf("%s: [", title);
    int start = len > max ? len - max : 0;
    for (int i = start; i < len; i++) {
        printf("%d%s", history[i % 64], i < len - 1 ? ", " : "");
    }
    printf("]\n");
}

/* ── Test combineReducers ──────────────────────────────────────────────── */

static void *sub_a_reduce(void *state, const ReduxAction *action,
                          size_t *out_size)
{
    (void)action;
    *out_size = sizeof(int);
    return state;
}

static void *sub_b_reduce(void *state, const ReduxAction *action,
                          size_t *out_size)
{
    (void)action;
    *out_size = sizeof(int);
    return state;
}

/* ── Main Demo ──────────────────────────────────────────────────────────── */

int main(void)
{
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║     mini-state-data: Redux Full Stack Demo                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* ── Step 1: Create store ──────────────────────────────────────────── */
    printf("── 1. Creating Redux Store ──────────────────────────────────\n");
    AppState initial = {0, "Ready", {0}, 0, false};

    ReduxReducer reducer = {counter_reducer, NULL, NULL};
    ReduxStore *store = redux_store_create(&initial, sizeof(AppState),
                                           reducer);
    printf("   Store created. Initial state: counter=%d\n", initial.counter);

    /* ── Step 2: Subscribe ────────────────────────────────────────────── */
    printf("\n── 2. Subscriptions ───────────────────────────────────────\n");
    int h1 = redux_store_subscribe(store, on_state_change, "Listener-1");
    int h2 = redux_store_subscribe(store, on_state_change, "Listener-2");
    printf("   Subscribed: handle-1=%d, handle-2=%d\n", h1, h2);

    /* ── Step 3: Dispatch basic actions ────────────────────────────────── */
    printf("\n── 3. Dispatching Actions ──────────────────────────────────\n");

    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});

    int set_val = 42;
    ReduxAction set_action = REDUX_ACTION_PAYLOAD("SET_COUNTER",
                                                   &set_val, sizeof(int));
    redux_store_dispatch(store, &set_action);

    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("DECREMENT")});
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("DECREMENT")});

    size_t sz;
    AppState *current = (AppState *)redux_store_get_state(store, &sz);
    print_history("   Counter history", current->history,
                  current->history_len, 10);

    /* ── Step 4: Unsubscribe ──────────────────────────────────────────── */
    printf("\n── 4. Unsubscription Test ──────────────────────────────────\n");
    redux_store_unsubscribe(store, h2);
    printf("   Unsubscribed handle-2. Only Listener-1 should fire:\n");
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});

    /* Re-subscribe */
    redux_store_subscribe(store, on_state_change, "Listener-2");

    /* ── Step 5: Logger middleware ─────────────────────────────────────── */
    printf("\n── 5. Logger Middleware ─────────────────────────────────────\n");
    ReduxMiddleware *logger = redux_middleware_logger();
    redux_store_add_middleware(store, logger);
    printf("   Logger middleware added. Next dispatch will be logged:\n");

    bool loading_true = true;
    ReduxAction load_action = REDUX_ACTION_PAYLOAD("LOADING",
                                                    &loading_true, sizeof(bool));
    redux_store_dispatch(store, &load_action);

    /* ── Step 6: Saga middleware ───────────────────────────────────────── */
    printf("\n── 6. Saga Middleware ───────────────────────────────────────\n");
    int saga_data = 0xDEAD;
    ReduxMiddleware *saga = redux_middleware_saga(saga_sim, &saga_data);
    redux_store_add_middleware(store, saga);
    printf("   Saga middleware added.\n");
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});

    /* ── Step 7: Thunk middleware ──────────────────────────────────────── */
    printf("\n── 7. Thunk Middleware ───────────────────────────────────────\n");
    ReduxMiddleware *thunk = redux_middleware_thunk();
    redux_store_add_middleware(store, thunk);
    printf("   Thunk middleware added (async dispatch simulation).\n");
    redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("RESET")});

    /* ── Step 8: DevTools Time-Travel ──────────────────────────────────── */
    printf("\n── 8. DevTools: Record & Time-Travel ────────────────────────\n");
    ReduxDevTools *devtools = redux_devtools_create(50);

    AppState *snap = (AppState *)redux_store_get_state(store, &sz);
    redux_devtools_record(devtools, snap, sz, "SNAPSHOT_START");

    /* Perform a series of actions, recording each */
    for (int i = 0; i < 5; i++) {
        redux_store_dispatch(store, &(ReduxAction){REDUX_ACTION("INCREMENT")});
        snap = (AppState *)redux_store_get_state(store, &sz);
        redux_devtools_record(devtools, snap, sz, "INCREMENT");
    }

    printf("   Recorded %d snapshots.\n", redux_devtools_history_count(devtools));

    /* List all snapshots */
    for (int i = 0; i < redux_devtools_history_count(devtools); i++) {
        ReduxSnapshot *s = redux_devtools_get_snapshot(devtools, i);
        if (s) {
            AppState *ast = (AppState *)s->state;
            printf("   [%d] action='%s', counter=%d, ts=%lld\n",
                   i, s->action_type, ast->counter, (long long)s->timestamp);
        }
    }

    /* Time-travel to snapshot 2 */
    printf("\n   Time-travel to snapshot [2]:\n");
    size_t restored_sz;
    void *restored = redux_devtools_time_travel(devtools, 2, &restored_sz);
    if (restored) {
        AppState *rst = (AppState *)restored;
        printf("   Restored state: counter=%d\n", rst->counter);
        free(restored);
    }

    /* Jump to (in-place restore) */
    printf("   Jump to snapshot [4]:\n");
    if (redux_devtools_jump_to(store, devtools, 4)) {
        AppState *after = (AppState *)redux_store_get_state(store, &sz);
        printf("   After jump: counter=%d (current devtools index=%d)\n",
               after->counter, redux_devtools_current_index(devtools));
    }

    /* ── Step 9: combineReducers ──────────────────────────────────────── */
    printf("\n── 9. combineReducers ───────────────────────────────────────\n");
    ReduxReducerEntry entries[] = {
        {"a", {sub_a_reduce, NULL, NULL}},
        {"b", {sub_b_reduce, NULL, NULL}},
    };
    ReduxReducer combined = redux_combine_reducers(entries, 2);
    printf("   Combined reducer created (2 sub-reducers).\n");
    (void)combined;

    /* ── Step 10: createSlice ─────────────────────────────────────────── */
    printf("\n── 10. createSlice ─────────────────────────────────────────\n");
    int init_val = 0;
    ReduxCaseReducer cases[] = {
        {"counter/increment", NULL},
        {"counter/decrement", NULL},
    };
    ReduxSlice slice = redux_create_slice("counter", &init_val,
                                          sizeof(int), cases, 2, reducer);
    printf("    Slice '%s' created with %d actions.\n",
           slice.name, slice.action_count);

    /* ── Step 11: MobX reaction tracking ──────────────────────────────── */
    printf("\n── 11. MobX Reactions ──────────────────────────────────────\n");

    MobxObservable *obs_x = mobx_observable_create("x", MOBX_INT(100));
    MobxObservable *obs_y = mobx_observable_create("y", MOBX_INT(200));

    MobxComputed *sum_c = mobx_computed_create("sum",
        (MobxComputedFn)(intptr_t)NULL, NULL);

    printf("    Observable x=%lld, y=%lld\n",
           (long long)mobx_observable_get(obs_x).data.i,
           (long long)mobx_observable_get(obs_y).data.i);

    MOBX_ACTION_BLOCK("updateCoordinates", {
        mobx_observable_set(obs_x, MOBX_INT(150));
        mobx_observable_set(obs_y, MOBX_INT(250));
        printf("    Updated within action: (%lld, %lld)\n",
               (long long)mobx_observable_get(obs_x).data.i,
               (long long)mobx_observable_get(obs_y).data.i);
    });

    bool is_running = mobx_action_is_running();
    printf("    Action running after block: %s\n", is_running ? "yes" : "no");

    /* ── Step 12: Transaction batching ────────────────────────────────── */
    printf("\n── 12. MobX Transaction ─────────────────────────────────────\n");
    MobxArray *arr = mobx_array_create();
    mobx_array_push(arr, MOBX_INT(1));
    mobx_array_push(arr, MOBX_INT(2));
    mobx_array_push(arr, MOBX_INT(3));

    printf("    Array before transaction: [");
    for (int i = 0; i < mobx_array_length(arr); i++)
        printf("%lld%s", (long long)mobx_array_get(arr, i).data.i,
               i < mobx_array_length(arr) - 1 ? ", " : "");
    printf("]\n");

    MOBX_RAW({
        mobx_array_push(arr, MOBX_INT(4));
        mobx_array_push(arr, MOBX_INT(5));
        printf("    Inside transaction: array length=%d\n",
               mobx_array_length(arr));
    });

    printf("    Array after transaction: [");
    for (int i = 0; i < mobx_array_length(arr); i++)
        printf("%lld%s", (long long)mobx_array_get(arr, i).data.i,
               i < mobx_array_length(arr) - 1 ? ", " : "");
    printf("]\n");

    /* ── Step 13: Zustand slices ──────────────────────────────────────── */
    printf("\n── 13. Zustand Slice Composition ────────────────────────────\n");

    typedef struct { int volume; bool muted; } AudioSlice;
    typedef struct { int brightness; bool dark_mode; } DisplaySlice;

    AudioSlice audio = {50, false};
    DisplaySlice display = {80, true};

    ZustandSlice slices[] = {
        {"audio", &audio, sizeof(AudioSlice), NULL},
        {"display", &display, sizeof(DisplaySlice), NULL},
    };

    ZustandStore *zstore = zustand_create_with_slices(slices, 2);
    printf("    Zustand store created with %d slices.\n", 2);

    /* Apply devtools middleware to zustand */
    ZustandMWConfig mw_configs[] = {
        {ZUSTAND_MW_DEVTOOLS, "zustand-demo", NULL},
    };
    zustand_apply_middleware(zstore, mw_configs, 1);
    printf("    DevTools middleware applied.\n");

    /* Immutable produce on zustand state */
    printf("    Using produce for immutable update:\n");
    typedef struct { int value; } SimpleState;
    SimpleState base = {42};

    void *produced = zustand_produce(&base, sizeof(SimpleState),
        NULL, NULL, NULL);
    if (produced) {
        printf("    Produced new state (no change → NULL): %s\n",
               produced ? "yes" : "NULL");
        free(produced);
    }

    /* ── Step 14: Normalized store ────────────────────────────────────── */
    printf("\n── 14. Normalized Store (Immutable) ─────────────────────────\n");

    NormalizedSchema schemas[] = {
        {"product", "id", NULL, 0},
        {"category", "id", NULL, 0},
        {"review", "id", NULL, 0},
    };
    ImmutNormalized *norm = immut_normalized_create(schemas, 3);

    typedef struct { int id; char name[64]; double price; } Product;
    typedef struct { int id; char name[64]; } Category;

    Product p1 = {1, "Laptop", 999.99};
    Product p2 = {2, "Mouse", 29.99};
    Product p3 = {3, "Keyboard", 89.99};
    Category c1 = {1, "Electronics"};

    immut_normalized_upsert(norm, "product", "1", &p1, sizeof(Product));
    immut_normalized_upsert(norm, "product", "2", &p2, sizeof(Product));
    immut_normalized_upsert(norm, "product", "3", &p3, sizeof(Product));
    immut_normalized_upsert(norm, "category", "1", &c1, sizeof(Category));

    printf("    Upserted 4 entities across 2 types.\n");

    Product *found_p = immut_normalized_get(norm, "product", "2", &sz);
    if (found_p) printf("    Found: %s ($%.2f)\n", found_p->name, found_p->price);

    immut_normalized_remove(norm, "product", "3");
    printf("    Removed product:3\n");

    /* ── Step 15: Change detection ────────────────────────────────────── */
    printf("\n── 15. Change Detection ────────────────────────────────────\n");

    int val_a = 100, val_b = 200, val_c = 100;

    printf("    ref_equal(&a, &a): %s\n", immut_ref_equal(&val_a, &val_a) ? "true" : "false");
    printf("    ref_equal(&a, &b): %s\n", immut_ref_equal(&val_a, &val_b) ? "true" : "false");
    printf("    quick_equal(&a, &c): %s\n",
           immut_quick_equal(&val_a, sizeof(int), &val_c, sizeof(int)) ? "true" : "false");
    printf("    quick_equal(&a, &b): %s\n",
           immut_quick_equal(&val_a, sizeof(int), &val_b, sizeof(int)) ? "true" : "false");
    printf("    shallow_eq: %s\n",
           immut_shallow_eq(&val_a, sizeof(int), &val_c, sizeof(int)) ? "true" : "false");
    printf("    hash(42): 0x%08x\n", immut_hash_jenkins(&val_a, sizeof(int)));

    /* ── Step 16: Memoization ─────────────────────────────────────────── */
    printf("\n── 16. Memoization ──────────────────────────────────────────\n");

    ImmutMemo *memo = immut_memo_create(16);
    int memo_key = 0xCAFE, memo_val = 0xBEEF;
    immut_memo_put(memo, &memo_key, sizeof(int), &memo_val, sizeof(int));

    void *cached_val = NULL;
    size_t cached_sz;
    if (immut_memo_get(memo, &memo_key, sizeof(int), &cached_val, &cached_sz)) {
        printf("    Memo hit: value=0x%08x\n", *(int *)cached_val);
    }

    int wrong_key = 0xDEAD;
    if (!immut_memo_get(memo, &wrong_key, sizeof(int), NULL, NULL)) {
        printf("    Memo miss for unknown key.\n");
    }

    /* ── Cleanup ──────────────────────────────────────────────────────── */
    printf("\n── 17. Cleanup ──────────────────────────────────────────────\n");

    immut_memo_destroy(memo);
    immut_normalized_destroy(norm);
    zustand_store_destroy(zstore);
    mobx_array_destroy(arr);
    mobx_computed_destroy(sum_c);
    mobx_observable_destroy(obs_x);
    mobx_observable_destroy(obs_y);
    redux_devtools_destroy(devtools);
    redux_store_destroy(store);

    printf("   All resources freed.\n");
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              Demo Complete                                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    return 0;
}
