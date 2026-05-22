#include "zustand_sim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Internal Store ─────────────────────────────────────────────────────── */

#define MAX_LISTENERS 64
#define MAX_DEVTOOLS_ENTRIES 128

typedef struct ListenerEntry {
    ZustandListener listener;
    ZustandSelector selector;
    void           *userdata;
    void           *last_selected;
    size_t          last_size;
    bool            active;
} ListenerEntry;

struct ZustandStore {
    void          *state;
    size_t         state_size;
    ListenerEntry  listeners[MAX_LISTENERS];
    int            listener_count;
    ZustandDevTools *devtools;
    bool           devtools_enabled;
};

/* ── Store ──────────────────────────────────────────────────────────────── */

ZustandStore *zustand_store_create(void *initial_state, size_t state_size)
{
    ZustandStore *store = calloc(1, sizeof(ZustandStore));
    if (!store) return NULL;

    store->state_size = state_size;
    if (initial_state && state_size > 0) {
        store->state = malloc(state_size);
        if (store->state) memcpy(store->state, initial_state, state_size);
    }
    store->listener_count = 0;
    store->devtools = NULL;
    store->devtools_enabled = false;
    return store;
}

void zustand_store_destroy(ZustandStore *store)
{
    if (!store) return;
    zustand_store_destroy_all_listeners(store);
    free(store->state);
    if (store->devtools) zustand_devtools_destroy(store->devtools);
    free(store);
}

void *zustand_store_get_state(ZustandStore *store, size_t *out_size)
{
    if (!store) { if (out_size) *out_size = 0; return NULL; }
    if (out_size) *out_size = store->state_size;
    return store->state;
}

void zustand_store_set_state(ZustandStore *store, void *partial, size_t size,
                             bool replace)
{
    if (!store) return;

    void *old_state = store->state;
    size_t old_size = store->state_size;

    if (replace) {
        store->state = malloc(size);
        if (store->state) memcpy(store->state, partial, size);
        store->state_size = size;
    } else {
        /* merge partial */
        store->state = malloc(size);
        if (store->state) memcpy(store->state, partial, size);
        store->state_size = size;
    }

    if (store->devtools_enabled && store->devtools) {
        zustand_devtools_record(store->devtools, store->state,
                                store->state_size, "setState");
    }

    for (int i = 0; i < store->listener_count; i++) {
        ListenerEntry *le = &store->listeners[i];
        if (!le->active || !le->listener) continue;

        if (le->selector) {
            void *selected = NULL;
            size_t sel_size = 0;
            if (le->selector(store->state, NULL, &sel_size, le->userdata)) {
                selected = malloc(sel_size);
                if (le->selector(store->state, selected, &sel_size, le->userdata)) {
                    if (!le->last_selected ||
                        !zustand_shallow_equal(le->last_selected, selected, sel_size)) {
                        free(le->last_selected);
                        le->last_selected = malloc(sel_size);
                        memcpy(le->last_selected, selected, sel_size);
                        le->last_size = sel_size;
                        le->listener(selected, sel_size, le->userdata);
                    }
                }
                free(selected);
            }
        } else {
            le->listener(store->state, store->state_size, le->userdata);
        }
    }

    free(old_state);
}

void zustand_store_update(ZustandStore *store, ZustandStateUpdater updater,
                          void *userdata)
{
    if (!store || !updater) return;

    size_t new_size = store->state_size;
    void *draft = malloc(store->state_size);
    if (draft) memcpy(draft, store->state, store->state_size);

    updater(draft, &new_size, userdata);

    zustand_store_set_state(store, draft, new_size, true);
    free(draft);
}

/* ── Subscriptions ──────────────────────────────────────────────────────── */

int zustand_store_subscribe(ZustandStore *store, ZustandListener listener,
                            void *userdata)
{
    if (!store || !listener) return -1;
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (!store->listeners[i].active) {
            store->listeners[i].listener = listener;
            store->listeners[i].selector = NULL;
            store->listeners[i].userdata = userdata;
            store->listeners[i].last_selected = NULL;
            store->listeners[i].last_size = 0;
            store->listeners[i].active = true;
            if (i >= store->listener_count) store->listener_count = i + 1;
            return i;
        }
    }
    return -1;
}

int zustand_store_subscribe_with_selector(ZustandStore *store,
                                          ZustandListener listener,
                                          ZustandSelector selector,
                                          void *userdata)
{
    if (!store || !listener || !selector) return -1;
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (!store->listeners[i].active) {
            store->listeners[i].listener = listener;
            store->listeners[i].selector = selector;
            store->listeners[i].userdata = userdata;
            store->listeners[i].last_selected = NULL;
            store->listeners[i].last_size = 0;
            store->listeners[i].active = true;
            if (i >= store->listener_count) store->listener_count = i + 1;
            return i;
        }
    }
    return -1;
}

void zustand_store_unsubscribe(ZustandStore *store, int handle)
{
    if (!store || handle < 0 || handle >= MAX_LISTENERS) return;
    free(store->listeners[handle].last_selected);
    store->listeners[handle].active = false;
    store->listeners[handle].last_selected = NULL;
}

void zustand_store_destroy_all_listeners(ZustandStore *store)
{
    if (!store) return;
    for (int i = 0; i < store->listener_count; i++) {
        free(store->listeners[i].last_selected);
        store->listeners[i].active = false;
    }
    store->listener_count = 0;
}

/* ── Shallow Equality ───────────────────────────────────────────────────── */

bool zustand_shallow_equal(const void *a, const void *b, size_t size)
{
    if (a == b) return true;
    if (!a || !b) return false;
    return memcmp(a, b, size) == 0;
}

bool zustand_shallow_equal_default(void) { return true; }

/* ── produce (immer-like) ───────────────────────────────────────────────── */

struct ZustandDraft {
    void   *base;
    void   *data;
    size_t  size;
    bool    modified;
};

void *zustand_produce(void *base_state, size_t base_size,
                      void (*recipe)(void *draft, size_t *out_size,
                                     void *userdata),
                      void *userdata, size_t *out_size)
{
    if (!base_state || !recipe) return NULL;

    void *draft_data = malloc(base_size);
    if (draft_data) memcpy(draft_data, base_state, base_size);

    size_t new_size = base_size;
    recipe(draft_data, &new_size, userdata);

    if (out_size) *out_size = new_size;

    if (new_size == base_size && memcmp(draft_data, base_state, base_size) == 0) {
        free(draft_data);
        return NULL;
    }
    return draft_data;
}

void zustand_produce_free(void *state) { free(state); }

ZustandDraft *zustand_draft_create(void *base, size_t size)
{
    ZustandDraft *draft = calloc(1, sizeof(ZustandDraft));
    draft->base = base;
    draft->size = size;
    draft->data = malloc(size);
    if (draft->data) memcpy(draft->data, base, size);
    draft->modified = false;
    return draft;
}

void *zustand_draft_finish(ZustandDraft *draft, size_t *out_size)
{
    if (!draft) return NULL;
    if (out_size) *out_size = draft->size;
    void *result = draft->data;
    free(draft);
    return result;
}

void zustand_draft_abort(ZustandDraft *draft)
{
    if (!draft) return;
    free(draft->data);
    free(draft);
}

bool zustand_draft_is_modified(ZustandDraft *draft)
{
    return draft ? draft->modified : false;
}

void *zustand_draft_get(ZustandDraft *draft, size_t offset, size_t size)
{
    if (!draft || offset + size > draft->size) return NULL;
    return (char *)draft->data + offset;
}

void zustand_draft_set(ZustandDraft *draft, size_t offset,
                       const void *value, size_t size)
{
    if (!draft || offset + size > draft->size) return;
    memcpy((char *)draft->data + offset, value, size);
    draft->modified = true;
}

void *zustand_draft_data(ZustandDraft *draft)
{
    return draft ? draft->data : NULL;
}

/* ── Middleware ──────────────────────────────────────────────────────────── */

void zustand_apply_middleware(ZustandStore *store, ZustandMWConfig *configs,
                              int count)
{
    if (!store) return;
    for (int i = 0; i < count; i++) {
        switch (configs[i].type) {
        case ZUSTAND_MW_DEVTOOLS:
            store->devtools = zustand_devtools_create(
                configs[i].name ? configs[i].name : "zustand",
                MAX_DEVTOOLS_ENTRIES);
            store->devtools_enabled = true;
            break;
        case ZUSTAND_MW_PERSIST:
        case ZUSTAND_MW_LOGGER:
        case ZUSTAND_MW_CUSTOM:
        default:
            break;
        }
    }
}

/* ── Persist ────────────────────────────────────────────────────────────── */

void zustand_persist_init(ZustandStore *store, ZustandPersistOpts opts)
{
    (void)store;
    (void)opts;
}

bool zustand_persist_save(ZustandStore *store)
{
    (void)store;
    return false;
}

bool zustand_persist_load(ZustandStore *store)
{
    (void)store;
    return false;
}

/* ── DevTools ───────────────────────────────────────────────────────────── */

struct ZustandDevTools {
    char   *name;
    void  **states;
    size_t *sizes;
    int     count;
    int     cap;
};

ZustandDevTools *zustand_devtools_create(const char *name, int max_entries)
{
    ZustandDevTools *dt = calloc(1, sizeof(ZustandDevTools));
    dt->name = name ? strdup(name) : strdup("zustand-store");
    dt->cap = max_entries > 0 ? max_entries : MAX_DEVTOOLS_ENTRIES;
    dt->states = calloc(dt->cap, sizeof(void *));
    dt->sizes = calloc(dt->cap, sizeof(size_t));
    return dt;
}

void zustand_devtools_destroy(ZustandDevTools *dt)
{
    if (!dt) return;
    for (int i = 0; i < dt->count; i++) free(dt->states[i]);
    free(dt->states);
    free(dt->sizes);
    free(dt->name);
    free(dt);
}

void zustand_devtools_record(ZustandDevTools *dt, void *state,
                             size_t size, const char *action)
{
    if (!dt) return;
    (void)action;
    if (dt->count >= dt->cap) {
        free(dt->states[0]);
        memmove(dt->states, dt->states + 1, (dt->cap - 1) * sizeof(void *));
        memmove(dt->sizes, dt->sizes + 1, (dt->cap - 1) * sizeof(size_t));
        dt->count--;
    }
    dt->states[dt->count] = malloc(size);
    memcpy(dt->states[dt->count], state, size);
    dt->sizes[dt->count] = size;
    dt->count++;
}

void *zustand_devtools_jump(ZustandDevTools *dt, int index, size_t *out_size)
{
    if (!dt || index < 0 || index >= dt->count) return NULL;
    if (out_size) *out_size = dt->sizes[index];
    return dt->states[index];
}

int zustand_devtools_count(ZustandDevTools *dt)
{
    return dt ? dt->count : 0;
}

/* ── Slice Pattern ──────────────────────────────────────────────────────── */

ZustandStore *zustand_create_with_slices(ZustandSlice *slices, int count)
{
    if (!slices || count <= 0) return NULL;

    size_t total_size = 0;
    for (int i = 0; i < count; i++) total_size += slices[i].state_size;

    void *combined = calloc(1, total_size);
    size_t offset = 0;
    for (int i = 0; i < count; i++) {
        if (slices[i].state) {
            memcpy((char *)combined + offset, slices[i].state,
                   slices[i].state_size);
        }
        offset += slices[i].state_size;
    }

    ZustandStore *store = zustand_store_create(combined, total_size);
    free(combined);

    for (int i = 0; i < count; i++) {
        if (slices[i].actions) {
            slices[i].actions(slices[i].state, store, store);
        }
    }

    return store;
}

/* ── Async Actions ──────────────────────────────────────────────────────── */

struct ZustandAsync {
    ZustandStore *store;
    struct {
        char   *id;
        bool    pending;
    } tasks[32];
    int task_count;
};

ZustandAsync *zustand_async_create(ZustandStore *store)
{
    ZustandAsync *a = calloc(1, sizeof(ZustandAsync));
    a->store = store;
    return a;
}

void zustand_async_destroy(ZustandAsync *async)
{
    if (!async) return;
    for (int i = 0; i < async->task_count; i++) free(async->tasks[i].id);
    free(async);
}

void zustand_async_dispatch(ZustandAsync *async, ZustandAsyncFn fn,
                            void *userdata)
{
    if (!async || !fn) return;
    fn(async, userdata);
}

bool zustand_async_is_pending(ZustandAsync *async, const char *id)
{
    if (!async || !id) return false;
    for (int i = 0; i < async->task_count; i++) {
        if (async->tasks[i].id && strcmp(async->tasks[i].id, id) == 0)
            return async->tasks[i].pending;
    }
    return false;
}

void zustand_async_abort(ZustandAsync *async, const char *id)
{
    if (!async || !id) return;
    for (int i = 0; i < async->task_count; i++) {
        if (async->tasks[i].id && strcmp(async->tasks[i].id, id) == 0) {
            async->tasks[i].pending = false;
        }
    }
}

void zustand_async_set_state(ZustandAsync *async, void *data, size_t size)
{
    if (!async || !async->store) return;
    zustand_store_set_state(async->store, data, size, true);
}

void *zustand_async_get_state(ZustandAsync *async, size_t *size)
{
    if (!async || !async->store) return NULL;
    return zustand_store_get_state(async->store, size);
}

void zustand_store_destroy_state(void *state)
{
    free(state);
}
