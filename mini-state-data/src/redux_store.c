#include "redux_store.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ── Internal Store ─────────────────────────────────────────────────────── */

struct ReduxStore {
    void            *state;
    size_t           state_size;
    ReduxReducer     reducer;
    ReduxMiddleware *middleware;
    ReduxDevTools   *devtools;

    struct {
        ReduxListener fn;
        void         *userdata;
        int           active;
    } listeners[64];
    int listener_count;
};

/* ── Action ─────────────────────────────────────────────────────────────── */

ReduxAction redux_action_make(const char *type, void *payload, size_t size)
{
    ReduxAction a;
    a.type = type;
    a.payload_size = size;
    a.payload = NULL;
    if (size > 0 && payload) {
        a.payload = malloc(size);
        if (a.payload) memcpy(a.payload, payload, size);
    }
    return a;
}

void redux_action_free(ReduxAction *action)
{
    if (!action) return;
    free(action->payload);
    action->payload = NULL;
    action->payload_size = 0;
}

/* ── Default state helpers ──────────────────────────────────────────────── */

static size_t default_get_size(void *state)
{
    (void)state;
    return 0;
}

static void default_destroy(void *state)
{
    free(state);
}

/* ── Store ──────────────────────────────────────────────────────────────── */

ReduxStore *redux_store_create(void *initial_state, size_t state_size,
                               ReduxReducer reducer)
{
    ReduxStore *store = calloc(1, sizeof(ReduxStore));
    if (!store) return NULL;

    store->state_size = state_size;
    if (initial_state && state_size > 0) {
        store->state = malloc(state_size);
        if (store->state) memcpy(store->state, initial_state, state_size);
    }

    store->reducer = reducer;
    if (!store->reducer.get_size) store->reducer.get_size = default_get_size;
    if (!store->reducer.destroy) store->reducer.destroy = default_destroy;
    store->middleware = NULL;
    store->devtools = NULL;
    store->listener_count = 0;
    return store;
}

void redux_store_destroy(ReduxStore *store)
{
    if (!store) return;

    if (store->state) store->reducer.destroy(store->state);

    ReduxMiddleware *mw = store->middleware;
    while (mw) {
        ReduxMiddleware *next = mw->next;
        if (mw->destroy) mw->destroy(mw->ctx);
        free(mw);
        mw = next;
    }

    if (store->devtools) redux_devtools_destroy(store->devtools);
    free(store);
}

void *redux_store_get_state(ReduxStore *store, size_t *out_size)
{
    if (!store) return NULL;
    if (out_size) *out_size = store->state_size;
    return store->state;
}

ReduxStore *redux_store_get_inner(ReduxStore *store)
{
    return store;
}

/* ── Dispatch with middleware chain ─────────────────────────────────────── */

typedef struct {
    ReduxStore     *store;
    ReduxMiddleware *current_mw;
} DispatchCtx;

static void *dispatch_next(void *ctx, const ReduxAction *action, size_t *sz)
{
    DispatchCtx *dctx = (DispatchCtx *)ctx;
    ReduxMiddleware *mw = dctx->current_mw;

    if (!mw) {
        void *ns = dctx->store->reducer.reduce(dctx->store->state, action, sz);
        return ns;
    }

    dctx->current_mw = mw->next;
    ReduxMiddleware *next_mw = dctx->current_mw;

    void *(*next_fn)(void *, const ReduxAction *, size_t *) = NULL;
    if (!next_mw) {
        next_fn = dispatch_next;
    }

    if (mw->apply) {
        return mw->apply(mw->ctx, action,
                         next_fn ? next_fn : (void *(*)(void*,const ReduxAction*,size_t*))dispatch_next,
                         sz);
    }
    return dispatch_next(ctx, action, sz);
}

void redux_store_dispatch(ReduxStore *store, const ReduxAction *action)
{
    if (!store || !action) return;

    DispatchCtx ctx = {store, store->middleware};
    size_t new_size = 0;

    void *new_state;
    if (store->middleware) {
        new_state = dispatch_next(&ctx, action, &new_size);
    } else {
        new_state = store->reducer.reduce(store->state, action, &new_size);
    }

    if (new_state && new_state != store->state) {
        if (store->state) store->reducer.destroy(store->state);
        store->state = new_state;
        store->state_size = new_size ? new_size : store->reducer.get_size(new_state);
    }

    if (store->devtools) {
        redux_devtools_record(store->devtools, store->state,
                              store->state_size, action->type);
    }

    for (int i = 0; i < store->listener_count; i++) {
        if (store->listeners[i].active && store->listeners[i].fn) {
            store->listeners[i].fn(store->state, store->state_size,
                                   store->listeners[i].userdata);
        }
    }
}

bool redux_store_add_middleware(ReduxStore *store, ReduxMiddleware *mw)
{
    if (!store || !mw) return false;
    mw->next = NULL;

    if (!store->middleware) {
        store->middleware = mw;
        return true;
    }

    ReduxMiddleware *last = store->middleware;
    while (last->next) last = last->next;
    last->next = mw;
    return true;
}

int redux_store_subscribe(ReduxStore *store, ReduxListener listener,
                          void *userdata)
{
    if (!store || !listener) return -1;
    for (int i = 0; i < 64; i++) {
        if (!store->listeners[i].active) {
            store->listeners[i].fn = listener;
            store->listeners[i].userdata = userdata;
            store->listeners[i].active = 1;
            if (i >= store->listener_count) store->listener_count = i + 1;
            return i;
        }
    }
    return -1;
}

void redux_store_unsubscribe(ReduxStore *store, int handle)
{
    if (!store || handle < 0 || handle >= 64) return;
    store->listeners[handle].active = 0;
    store->listeners[handle].fn = NULL;
}

/* ── combineReducers ────────────────────────────────────────────────────── */

typedef struct {
    char        **keys;
    ReduxReducer *reducers;
    int           count;
} CombinedState;

static void *combined_reduce(void *state, const ReduxAction *action,
                             size_t *out_size)
{
    CombinedState *cs = (CombinedState *)state;
    (void)action;
    (void)out_size;

    for (int i = 0; i < cs->count; i++) {
        if (cs->reducers[i].reduce) {
            void *slice = NULL;
            /* In practice: get slice state, reduce it, put back */
        }
    }
    return state;
}

static size_t combined_get_size(void *state)
{
    CombinedState *cs = (CombinedState *)state;
    return cs ? sizeof(CombinedState) : 0;
}

ReduxReducer redux_combine_reducers(const ReduxReducerEntry *entries,
                                    int count)
{
    ReduxReducer r = {combined_reduce, combined_get_size, default_destroy};
    (void)entries;
    (void)count;
    return r;
}

/* ── createSlice ────────────────────────────────────────────────────────── */

ReduxSlice redux_create_slice(const char *name,
                              void *initial_state, size_t init_size,
                              const ReduxCaseReducer *cases, int case_count,
                              ReduxReducer parent_reducer)
{
    ReduxSlice slice;
    slice.name = name;
    slice.reducer = parent_reducer;
    slice.action_count = case_count;
    slice.actions = NULL;
    (void)initial_state;
    (void)init_size;
    (void)cases;
    return slice;
}

/* ── Logger Middleware ──────────────────────────────────────────────────── */

typedef struct {
    FILE *out;
} LoggerCtx;

static void *logger_apply(void *ctx, const ReduxAction *action,
                          void *(*next)(void *, const ReduxAction *, size_t *),
                          size_t *out_size)
{
    LoggerCtx *lc = (LoggerCtx *)ctx;
    int64_t ts = (int64_t)time(NULL);
    fprintf(lc->out ? lc->out : stdout,
            "[REDUX %lld] dispatching: %s\n", (long long)ts, action->type);

    void *result = next(NULL, action, out_size);

    fprintf(lc->out ? lc->out : stdout,
            "[REDUX %lld] next state\n", (long long)ts);
    return result;
}

static void logger_destroy(void *ctx)
{
    free(ctx);
}

ReduxMiddleware *redux_middleware_logger(void)
{
    ReduxMiddleware *mw = calloc(1, sizeof(ReduxMiddleware));
    LoggerCtx *lc = calloc(1, sizeof(LoggerCtx));
    lc->out = stdout;
    mw->apply = logger_apply;
    mw->destroy = logger_destroy;
    mw->ctx = lc;
    return mw;
}

/* ── Thunk Middleware ───────────────────────────────────────────────────── */

typedef struct {
    ReduxThunkFn fn;
    void        *userdata;
} ThunkCtx;

static void *thunk_apply(void *ctx, const ReduxAction *action,
                         void *(*next)(void *, const ReduxAction *, size_t *),
                         size_t *out_size)
{
    (void)next;
    (void)out_size;
    ThunkCtx *tc = (ThunkCtx *)ctx;
    if (tc && tc->fn) {
        tc->fn(NULL, NULL, NULL, tc->userdata);
    }
    return NULL;
}

static void thunk_destroy(void *ctx)
{
    free(ctx);
}

ReduxMiddleware *redux_middleware_thunk(void)
{
    ReduxMiddleware *mw = calloc(1, sizeof(ReduxMiddleware));
    ThunkCtx *tc = calloc(1, sizeof(ThunkCtx));
    tc->fn = NULL;
    mw->apply = thunk_apply;
    mw->destroy = thunk_destroy;
    mw->ctx = tc;
    return mw;
}

/* ── Saga Sim Middleware ────────────────────────────────────────────────── */

typedef struct {
    ReduxSagaFn saga;
    void       *userdata;
} SagaCtx;

static void *saga_apply(void *ctx, const ReduxAction *action,
                        void *(*next)(void *, const ReduxAction *, size_t *),
                        size_t *out_size)
{
    SagaCtx *sc = (SagaCtx *)ctx;
    if (sc && sc->saga) {
        ReduxSagaCtx sctx = {NULL, false, sc->userdata};
        sc->saga(&sctx);
    }
    return next(NULL, action, out_size);
}

ReduxMiddleware *redux_middleware_saga(ReduxSagaFn saga, void *userdata)
{
    ReduxMiddleware *mw = calloc(1, sizeof(ReduxMiddleware));
    SagaCtx *sc = malloc(sizeof(SagaCtx));
    sc->saga = saga;
    sc->userdata = userdata;
    mw->apply = saga_apply;
    mw->destroy = free;
    mw->ctx = sc;
    return mw;
}

/* ── DevTools ───────────────────────────────────────────────────────────── */

struct ReduxDevTools {
    ReduxSnapshot *snapshots;
    int            count;
    int            capacity;
    int            current;
};

ReduxDevTools *redux_devtools_create(int max_history)
{
    ReduxDevTools *dt = calloc(1, sizeof(ReduxDevTools));
    dt->capacity = max_history > 0 ? max_history : 64;
    dt->snapshots = calloc(dt->capacity, sizeof(ReduxSnapshot));
    dt->count = 0;
    dt->current = -1;
    return dt;
}

void redux_devtools_destroy(ReduxDevTools *dt)
{
    if (!dt) return;
    for (int i = 0; i < dt->count; i++) {
        free(dt->snapshots[i].state);
    }
    free(dt->snapshots);
    free(dt);
}

void redux_devtools_record(ReduxDevTools *dt, void *state,
                           size_t size, const char *action)
{
    if (!dt) return;

    if (dt->count >= dt->capacity) {
        free(dt->snapshots[0].state);
        memmove(dt->snapshots, dt->snapshots + 1,
                (dt->capacity - 1) * sizeof(ReduxSnapshot));
        dt->count--;
        dt->current--;
    }

    int idx = dt->count;
    dt->snapshots[idx].state = malloc(size);
    if (dt->snapshots[idx].state) {
        memcpy(dt->snapshots[idx].state, state, size);
    }
    dt->snapshots[idx].size = size;
    dt->snapshots[idx].timestamp = (int64_t)time(NULL);
    dt->snapshots[idx].action_type = action;
    dt->count++;
    dt->current = idx;
}

int redux_devtools_history_count(ReduxDevTools *dt)
{
    return dt ? dt->count : 0;
}

ReduxSnapshot *redux_devtools_get_snapshot(ReduxDevTools *dt, int index)
{
    if (!dt || index < 0 || index >= dt->count) return NULL;
    return &dt->snapshots[index];
}

void *redux_devtools_time_travel(ReduxDevTools *dt, int index, size_t *out_size)
{
    if (!dt || index < 0 || index >= dt->count) return NULL;
    dt->current = index;
    if (out_size) *out_size = dt->snapshots[index].size;

    void *copy = malloc(dt->snapshots[index].size);
    if (copy) memcpy(copy, dt->snapshots[index].state, dt->snapshots[index].size);
    return copy;
}

bool redux_devtools_jump_to(ReduxStore *store, ReduxDevTools *dt, int index)
{
    if (!store || !dt || index < 0 || index >= dt->count) return false;
    void *restored = redux_devtools_time_travel(dt, index, NULL);
    if (!restored) return false;

    if (store->state) store->reducer.destroy(store->state);
    store->state = restored;
    store->state_size = dt->snapshots[index].size;
    dt->current = index;
    return true;
}

int redux_devtools_current_index(ReduxDevTools *dt)
{
    return dt ? dt->current : -1;
}
