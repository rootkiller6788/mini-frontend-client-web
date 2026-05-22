#ifndef ZUSTAND_SIM_H
#define ZUSTAND_SIM_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core Types ─────────────────────────────────────────────────────────── */

typedef struct ZustandStore ZustandStore;

typedef void *(*ZustandStateCreator)(void *set_fn, void *get_fn, void *api);
typedef void  (*ZustandListener)(void *selected, size_t size, void *userdata);
typedef bool  (*ZustandSelector)(const void *state, void *out_selected,
                                 size_t *out_size, void *userdata);
typedef bool  (*ZustandShallowEq)(const void *a, const void *b, size_t size);
typedef void  (*ZustandMiddleware)(ZustandStore *store, void *config);

/* ── Store Creation ─────────────────────────────────────────────────────── */

ZustandStore *zustand_store_create(void *initial_state, size_t state_size);
void          zustand_store_destroy(ZustandStore *store);

void         *zustand_store_get_state(ZustandStore *store, size_t *out_size);
void          zustand_store_set_state(ZustandStore *store,
                                      void *partial_or_updater, size_t size,
                                      bool replace);

typedef void (*ZustandStateUpdater)(void *draft, size_t *out_size,
                                    void *userdata);
void          zustand_store_update(ZustandStore *store,
                                   ZustandStateUpdater updater, void *userdata);

/* ── Subscriptions ──────────────────────────────────────────────────────── */

int  zustand_store_subscribe(ZustandStore *store, ZustandListener listener,
                             void *userdata);
int  zustand_store_subscribe_with_selector(ZustandStore *store,
                                           ZustandListener listener,
                                           ZustandSelector selector,
                                           void *userdata);
void zustand_store_unsubscribe(ZustandStore *store, int handle);
void zustand_store_destroy_all_listeners(ZustandStore *store);

/* ── Shallow Equality ───────────────────────────────────────────────────── */

bool zustand_shallow_equal(const void *a, const void *b, size_t size);
bool zustand_shallow_equal_default(void);

/* ── Immer-like Immutable Update (produce) ──────────────────────────────── */

void *zustand_produce(void *base_state, size_t base_size,
                      void (*recipe)(void *draft, size_t *out_size,
                                     void *userdata),
                      void *userdata, size_t *out_size);
void  zustand_produce_free(void *state);

typedef struct ZustandDraft ZustandDraft;

ZustandDraft *zustand_draft_create(void *base, size_t size);
void         *zustand_draft_finish(ZustandDraft *draft, size_t *out_size);
void          zustand_draft_abort(ZustandDraft *draft);
bool          zustand_draft_is_modified(ZustandDraft *draft);

void *zustand_draft_get(ZustandDraft *draft, size_t offset, size_t size);
void  zustand_draft_set(ZustandDraft *draft, size_t offset,
                        const void *value, size_t size);
void *zustand_draft_data(ZustandDraft *draft);

/* ── Middleware ──────────────────────────────────────────────────────────── */

typedef enum {
    ZUSTAND_MW_PERSIST,
    ZUSTAND_MW_DEVTOOLS,
    ZUSTAND_MW_LOGGER,
    ZUSTAND_MW_CUSTOM
} ZustandMiddlewareType;

typedef struct ZustandMWConfig {
    ZustandMiddlewareType type;
    const char *name;
    void       *opts;
} ZustandMWConfig;

void zustand_apply_middleware(ZustandStore *store, ZustandMWConfig *configs,
                              int count);

/* ── Persist Middleware ─────────────────────────────────────────────────── */

typedef struct ZustandPersistOpts {
    const char *key;
    bool (*serialize)(void *state, size_t size, void **out, size_t *out_sz);
    bool (*deserialize)(void *in, size_t in_sz, void **out, size_t *out_sz);
} ZustandPersistOpts;

void zustand_persist_init(ZustandStore *store, ZustandPersistOpts opts);
bool zustand_persist_save(ZustandStore *store);
bool zustand_persist_load(ZustandStore *store);

/* ── DevTools Middleware ────────────────────────────────────────────────── */

typedef struct ZustandDevTools ZustandDevTools;

ZustandDevTools *zustand_devtools_create(const char *name, int max_entries);
void             zustand_devtools_destroy(ZustandDevTools *dt);
void             zustand_devtools_record(ZustandDevTools *dt, void *state,
                                         size_t size, const char *action);
void            *zustand_devtools_jump(ZustandDevTools *dt, int index,
                                       size_t *out_size);
int              zustand_devtools_count(ZustandDevTools *dt);

/* ── Slice Pattern ──────────────────────────────────────────────────────── */

typedef struct ZustandSlice {
    const char *name;
    void       *state;
    size_t      state_size;
    void      (*actions)(void *state, void *set_fn, void *get_fn);
} ZustandSlice;

ZustandStore *zustand_create_with_slices(ZustandSlice *slices, int count);

/* ── Async Actions ──────────────────────────────────────────────────────── */

typedef struct ZustandAsync ZustandAsync;

typedef void (*ZustandAsyncFn)(ZustandAsync *async, void *userdata);

ZustandAsync *zustand_async_create(ZustandStore *store);
void          zustand_async_destroy(ZustandAsync *async);

void  zustand_async_dispatch(ZustandAsync *async, ZustandAsyncFn fn,
                             void *userdata);
bool  zustand_async_is_pending(ZustandAsync *async, const char *id);
void  zustand_async_abort(ZustandAsync *async, const char *id);

void  zustand_async_set_state(ZustandAsync *async, void *data, size_t size);
void *zustand_async_get_state(ZustandAsync *async, size_t *size);

/* ── Utility ────────────────────────────────────────────────────────────── */

void zustand_store_destroy_state(void *state);

#ifdef __cplusplus
}
#endif

#endif /* ZUSTAND_SIM_H */
