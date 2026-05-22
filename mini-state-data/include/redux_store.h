#ifndef REDUX_STORE_H
#define REDUX_STORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core Types ─────────────────────────────────────────────────────────── */

typedef struct ReduxAction {
    const char *type;
    void       *payload;
    size_t      payload_size;
} ReduxAction;

typedef struct ReduxReducer {
    void *(*reduce)(void *state, const ReduxAction *action, size_t *out_size);
    size_t (*get_size)(void *state);
    void   (*destroy)(void *state);
} ReduxReducer;

typedef struct ReduxMiddleware {
    void *(*apply)(void *store_ctx, const ReduxAction *action,
                   void *(*next)(void *ctx, const ReduxAction *a, size_t *sz),
                   size_t *out_size);
    void (*destroy)(void *ctx);
    void *ctx;
    struct ReduxMiddleware *next;
} ReduxMiddleware;

typedef struct ReduxStore  ReduxStore;
typedef struct ReduxDevTools ReduxDevTools;

/* ── Action Creators ───────────────────────────────────────────────────── */

ReduxAction redux_action_make(const char *type, void *payload, size_t size);
void        redux_action_free(ReduxAction *action);

#define REDUX_ACTION(TYPE) ((ReduxAction){.type = (TYPE), .payload = NULL, .payload_size = 0})
#define REDUX_ACTION_PAYLOAD(TYPE, PTR, SZ) \
    ((ReduxAction){.type = (TYPE), .payload = (PTR), .payload_size = (SZ)})

/* ── Store ──────────────────────────────────────────────────────────────── */

ReduxStore *redux_store_create(void *initial_state, size_t state_size,
                               ReduxReducer reducer);
void        redux_store_destroy(ReduxStore *store);

void       *redux_store_get_state(ReduxStore *store, size_t *out_size);
void        redux_store_dispatch(ReduxStore *store, const ReduxAction *action);

bool        redux_store_add_middleware(ReduxStore *store, ReduxMiddleware *mw);

typedef void (*ReduxListener)(void *new_state, size_t size, void *userdata);
int         redux_store_subscribe(ReduxStore *store, ReduxListener listener,
                                  void *userdata);
void        redux_store_unsubscribe(ReduxStore *store, int handle);

ReduxStore *redux_store_get_inner(ReduxStore *store);

/* ── combineReducers ───────────────────────────────────────────────────── */

typedef struct ReduxReducerEntry {
    const char        *key;
    ReduxReducer       reducer;
} ReduxReducerEntry;

ReduxReducer redux_combine_reducers(const ReduxReducerEntry *entries,
                                    int count);

/* ── createSlice ────────────────────────────────────────────────────────── */

typedef struct ReduxSlice {
    const char    *name;
    ReduxReducer   reducer;
    ReduxAction   *(*actions)(int *count);   /* returns array of action templates */
    int            action_count;
} ReduxSlice;

typedef struct ReduxCaseReducer {
    const char *action_type;
    void      *(*reduce)(void *state, const ReduxAction *action, size_t *out_size);
} ReduxCaseReducer;

ReduxSlice redux_create_slice(const char *name,
                              void *initial_state, size_t init_size,
                              const ReduxCaseReducer *cases, int case_count,
                              ReduxReducer parent_reducer);

/* ── Middleware: Logger ─────────────────────────────────────────────────── */

ReduxMiddleware *redux_middleware_logger(void);

/* ── Middleware: Thunk ──────────────────────────────────────────────────── */

typedef void (*ReduxThunkFn)(void *dispatch_fn, void *get_state_fn,
                             void *get_state_outer, void *userdata);
ReduxMiddleware *redux_middleware_thunk(void);

/* ── Middleware: Saga Sim ───────────────────────────────────────────────── */

typedef struct ReduxSagaCtx {
    void  *store;
    bool   cancelled;
    void  *userdata;
} ReduxSagaCtx;

typedef void (*ReduxSagaFn)(ReduxSagaCtx *ctx);
ReduxMiddleware *redux_middleware_saga(ReduxSagaFn saga, void *userdata);

/* ── DevTools (Time-Travel) ─────────────────────────────────────────────── */

typedef struct ReduxSnapshot {
    void   *state;
    size_t  size;
    int64_t timestamp;
    const char *action_type;
} ReduxSnapshot;

ReduxDevTools *redux_devtools_create(int max_history);
void           redux_devtools_destroy(ReduxDevTools *dt);
void           redux_devtools_record(ReduxDevTools *dt, void *state,
                                     size_t size, const char *action);
int            redux_devtools_history_count(ReduxDevTools *dt);
ReduxSnapshot *redux_devtools_get_snapshot(ReduxDevTools *dt, int index);
void          *redux_devtools_time_travel(ReduxDevTools *dt, int index,
                                          size_t *out_size);
bool           redux_devtools_jump_to(ReduxStore *store, ReduxDevTools *dt,
                                      int index);
int            redux_devtools_current_index(ReduxDevTools *dt);

#ifdef __cplusplus
}
#endif

#endif /* REDUX_STORE_H */
