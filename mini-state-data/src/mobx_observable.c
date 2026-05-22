#include "mobx_observable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Observable ─────────────────────────────────────────────────────────── */

struct MobxObservable {
    char       *name;
    MobxValue   value;
    void      **trackers;     /* autorun instances tracking this */
    int         tracker_count;
    int         tracker_cap;
    void       *userdata;
    int         is_observable_marker;
};

#define MOBX_MARKER 0x0845

MobxObservable *mobx_observable_create(const char *name, MobxValue initial)
{
    MobxObservable *obs = calloc(1, sizeof(MobxObservable));
    if (!obs) return NULL;
    obs->name = name ? strdup(name) : NULL;
    obs->value = initial;
    obs->tracker_cap = 8;
    obs->trackers = calloc(obs->tracker_cap, sizeof(void *));
    obs->is_observable_marker = MOBX_MARKER;
    return obs;
}

void mobx_observable_destroy(MobxObservable *obs)
{
    if (!obs) return;
    free(obs->name);
    free(obs->trackers);
    if (obs->value.type == MOBX_TYPE_STRING && obs->value.data.s) {
        free(obs->value.data.s);
    }
    free(obs);
}

MobxValue mobx_observable_get(MobxObservable *obs)
{
    if (!obs) return (MobxValue){MOBX_TYPE_INT, {0}};

    MobxTransaction *tx = mobx_transaction_is_active() ? NULL : NULL;

    for (int i = 0; i < obs->tracker_count; i++) {
        /* notify trackers of read */
    }

    return obs->value;
}

void mobx_observable_set(MobxObservable *obs, MobxValue value)
{
    if (!obs) return;

    if (mobx_transaction_is_active()) {
        if (obs->value.type == MOBX_TYPE_STRING && obs->value.data.s) {
            free(obs->value.data.s);
        }
        obs->value = value;
        return;
    }

    if (obs->value.type == MOBX_TYPE_STRING && obs->value.data.s) {
        free(obs->value.data.s);
    }
    obs->value = value;

    for (int i = 0; i < obs->tracker_count; i++) {
        MobxAutorun *ar = (MobxAutorun *)obs->trackers[i];
        if (ar) mobx_autorun_schedule(ar);
    }
}

const char *mobx_observable_name(MobxObservable *obs)
{
    return obs ? obs->name : NULL;
}

void *mobx_observable_userdata(MobxObservable *obs)
{
    return obs ? obs->userdata : NULL;
}

void mobx_observable_set_userdata(MobxObservable *obs, void *ud)
{
    if (obs) obs->userdata = ud;
}

/* ── Autorun ────────────────────────────────────────────────────────────── */

struct MobxAutorun {
    MobxAutorunFn     fn;
    void             *userdata;
    MobxObservable  **deps;
    int               dep_count;
    int               dep_cap;
    bool              scheduled;
    bool              running;
    int               autorun_marker;
};

#define AUTORUN_MARKER 0xA110

MobxAutorun *mobx_autorun_create(MobxAutorunFn fn, void *userdata)
{
    MobxAutorun *ar = calloc(1, sizeof(MobxAutorun));
    ar->fn = fn;
    ar->userdata = userdata;
    ar->dep_cap = 16;
    ar->deps = calloc(ar->dep_cap, sizeof(MobxObservable *));
    ar->autorun_marker = AUTORUN_MARKER;
    ar->scheduled = false;
    ar->running = false;

    mobx_autorun_schedule(ar);
    return ar;
}

void mobx_autorun_destroy(MobxAutorun *ar)
{
    if (!ar) return;
    for (int i = 0; i < ar->dep_count; i++) {
        mobx_autorun_untrack(ar, ar->deps[i]);
    }
    free(ar->deps);
    free(ar);
}

void mobx_autorun_schedule(MobxAutorun *ar)
{
    if (!ar || ar->scheduled || ar->running) return;
    ar->scheduled = true;

    for (int i = ar->dep_count - 1; i >= 0; i--) {
        mobx_autorun_untrack(ar, ar->deps[i]);
    }
    ar->dep_count = 0;

    ar->running = true;
    if (ar->fn) ar->fn(ar, ar->userdata);
    ar->running = false;
    ar->scheduled = false;
}

bool mobx_autorun_is_scheduled(MobxAutorun *ar)
{
    return ar ? ar->scheduled : false;
}

void mobx_autorun_track(MobxAutorun *ar, MobxObservable *obs)
{
    if (!ar || !obs) return;
    for (int i = 0; i < ar->dep_count; i++) {
        if (ar->deps[i] == obs) return;
    }
    if (ar->dep_count >= ar->dep_cap) {
        ar->dep_cap *= 2;
        ar->deps = realloc(ar->deps, ar->dep_cap * sizeof(MobxObservable *));
    }
    ar->deps[ar->dep_count++] = obs;

    if (obs->tracker_count >= obs->tracker_cap) {
        obs->tracker_cap *= 2;
        obs->trackers = realloc(obs->trackers, obs->tracker_cap * sizeof(void *));
    }
    obs->trackers[obs->tracker_count++] = ar;
}

void mobx_autorun_untrack(MobxAutorun *ar, MobxObservable *obs)
{
    if (!ar || !obs) return;
    for (int i = 0; i < ar->dep_count; i++) {
        if (ar->deps[i] == obs) {
            ar->deps[i] = ar->deps[ar->dep_count - 1];
            ar->dep_count--;
            break;
        }
    }
    for (int i = 0; i < obs->tracker_count; i++) {
        if (obs->trackers[i] == ar) {
            obs->trackers[i] = obs->trackers[obs->tracker_count - 1];
            obs->tracker_count--;
            break;
        }
    }
}

int mobx_autorun_dependency_count(MobxAutorun *ar)
{
    return ar ? ar->dep_count : 0;
}

/* ── Computed ───────────────────────────────────────────────────────────── */

struct MobxComputed {
    char         *name;
    MobxComputedFn fn;
    void         *userdata;
    MobxValue     cached;
    bool          has_cache;
    bool          stale;
    MobxAutorun  *autorun;
};

static void computed_autorun_fn(MobxAutorun *ar, void *userdata)
{
    MobxComputed *comp = (MobxComputed *)userdata;
    if (comp && comp->fn) {
        comp->cached = comp->fn(comp->userdata);
        comp->has_cache = true;
        comp->stale = false;
    }
    (void)ar;
}

MobxComputed *mobx_computed_create(const char *name, MobxComputedFn fn,
                                   void *userdata)
{
    MobxComputed *comp = calloc(1, sizeof(MobxComputed));
    comp->name = name ? strdup(name) : NULL;
    comp->fn = fn;
    comp->userdata = userdata;
    comp->has_cache = false;
    comp->stale = true;
    comp->autorun = mobx_autorun_create(computed_autorun_fn, comp);
    return comp;
}

void mobx_computed_destroy(MobxComputed *comp)
{
    if (!comp) return;
    if (comp->autorun) mobx_autorun_destroy(comp->autorun);
    free(comp->name);
    free(comp);
}

MobxValue mobx_computed_get(MobxComputed *comp)
{
    if (!comp) return (MobxValue){MOBX_TYPE_INT, {0}};
    if (comp->stale || !comp->has_cache) {
        computed_autorun_fn(NULL, comp);
    }
    return comp->cached;
}

bool mobx_computed_is_stale(MobxComputed *comp)
{
    return comp ? comp->stale : true;
}

void mobx_computed_invalidate(MobxComputed *comp)
{
    if (comp) comp->stale = true;
}

const char *mobx_computed_name(MobxComputed *comp)
{
    return comp ? comp->name : NULL;
}

/* ── Action ─────────────────────────────────────────────────────────────── */

static int action_depth = 0;
static char *current_action_name = NULL;

struct MobxAction {
    char *name;
};

MobxAction *mobx_action_begin(const char *name)
{
    MobxAction *a = malloc(sizeof(MobxAction));
    a->name = name ? strdup(name) : NULL;
    action_depth++;
    free(current_action_name);
    current_action_name = name ? strdup(name) : NULL;
    return a;
}

void mobx_action_end(MobxAction *action)
{
    if (!action) return;
    action_depth--;
    free(action->name);
    free(action);
}

bool mobx_action_is_running(void)
{
    return action_depth > 0;
}

const char *mobx_action_current_name(void)
{
    return current_action_name;
}

/* ── Reactions ──────────────────────────────────────────────────────────── */

struct MobxReaction {
    MobxAutorunFn     track_fn;
    MobxSideEffectFn  effect_fn;
    void             *userdata;
    MobxAutorun      *autorun;
    bool               disposed;
};

static void reaction_wrapper(MobxAutorun *ar, void *userdata)
{
    MobxReaction *r = (MobxReaction *)userdata;
    if (r && !r->disposed) {
        if (r->track_fn) r->track_fn(ar, r->userdata);
        if (r->effect_fn) r->effect_fn(r->userdata);
    }
}

MobxReaction *mobx_reaction_create(MobxAutorunFn track_fn,
                                   MobxSideEffectFn effect_fn,
                                   void *userdata)
{
    MobxReaction *r = calloc(1, sizeof(MobxReaction));
    r->track_fn = track_fn;
    r->effect_fn = effect_fn;
    r->userdata = userdata;
    r->autorun = mobx_autorun_create(reaction_wrapper, r);
    return r;
}

void mobx_reaction_destroy(MobxReaction *reaction)
{
    if (!reaction) return;
    if (reaction->autorun) mobx_autorun_destroy(reaction->autorun);
    free(reaction);
}

void mobx_reaction_run(MobxReaction *reaction)
{
    if (!reaction) return;
    reaction_wrapper(NULL, reaction);
}

void mobx_reaction_dispose(MobxReaction *reaction)
{
    if (reaction) {
        reaction->disposed = true;
    }
}

void mobx_when(MobxWhenPredicate pred, MobxSideEffectFn effect, void *userdata)
{
    if (!pred || !effect) return;
    if (pred(userdata)) {
        effect(userdata);
    }
}

/* ── Observable Array ───────────────────────────────────────────────────── */

struct MobxArray {
    MobxValue *items;
    int        length;
    int        capacity;
};

MobxArray *mobx_array_create(void)
{
    MobxArray *arr = calloc(1, sizeof(MobxArray));
    arr->capacity = 8;
    arr->items = calloc(arr->capacity, sizeof(MobxValue));
    return arr;
}

void mobx_array_destroy(MobxArray *arr)
{
    if (!arr) return;
    for (int i = 0; i < arr->length; i++) {
        if (arr->items[i].type == MOBX_TYPE_STRING && arr->items[i].data.s) {
            free(arr->items[i].data.s);
        }
    }
    free(arr->items);
    free(arr);
}

int mobx_array_length(MobxArray *arr) { return arr ? arr->length : 0; }

MobxValue mobx_array_get(MobxArray *arr, int index)
{
    if (!arr || index < 0 || index >= arr->length)
        return (MobxValue){MOBX_TYPE_INT, {0}};
    return arr->items[index];
}

void mobx_array_set(MobxArray *arr, int index, MobxValue value)
{
    if (!arr || index < 0 || index >= arr->length) return;
    arr->items[index] = value;
}

void mobx_array_push(MobxArray *arr, MobxValue value)
{
    if (!arr) return;
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(MobxValue));
    }
    arr->items[arr->length++] = value;
}

MobxValue mobx_array_pop(MobxArray *arr)
{
    if (!arr || arr->length == 0) return (MobxValue){MOBX_TYPE_INT, {0}};
    return arr->items[--arr->length];
}

void mobx_array_insert(MobxArray *arr, int index, MobxValue value)
{
    if (!arr || index < 0 || index > arr->length) return;
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(MobxValue));
    }
    if (index < arr->length) {
        memmove(&arr->items[index + 1], &arr->items[index],
                (arr->length - index) * sizeof(MobxValue));
    }
    arr->items[index] = value;
    arr->length++;
}

MobxValue mobx_array_remove(MobxArray *arr, int index)
{
    MobxValue removed = {MOBX_TYPE_INT, {0}};
    if (!arr || index < 0 || index >= arr->length) return removed;
    removed = arr->items[index];
    if (index < arr->length - 1) {
        memmove(&arr->items[index], &arr->items[index + 1],
                (arr->length - index - 1) * sizeof(MobxValue));
    }
    arr->length--;
    return removed;
}

void mobx_array_clear(MobxArray *arr)
{
    if (arr) arr->length = 0;
}

void *mobx_array_data(MobxArray *arr, int *count)
{
    if (count) *count = arr ? arr->length : 0;
    return arr ? arr->items : NULL;
}

/* ── Observable Map ─────────────────────────────────────────────────────── */

typedef struct MapNode {
    char          *key;
    MobxValue      value;
    struct MapNode *next;
} MapNode;

struct MobxMap {
    MapNode **buckets;
    int       bucket_count;
    int       size;
};

static unsigned long map_hash(const char *s)
{
    unsigned long h = 5381;
    int c;
    while ((c = *s++)) h = ((h << 5) + h) + c;
    return h;
}

MobxMap *mobx_map_create(void)
{
    MobxMap *m = calloc(1, sizeof(MobxMap));
    m->bucket_count = 16;
    m->buckets = calloc(m->bucket_count, sizeof(MapNode *));
    return m;
}

void mobx_map_destroy(MobxMap *map)
{
    if (!map) return;
    for (int i = 0; i < map->bucket_count; i++) {
        MapNode *n = map->buckets[i];
        while (n) {
            MapNode *next = n->next;
            free(n->key);
            if (n->value.type == MOBX_TYPE_STRING && n->value.data.s) {
                free(n->value.data.s);
            }
            free(n);
            n = next;
        }
    }
    free(map->buckets);
    free(map);
}

int mobx_map_size(MobxMap *map) { return map ? map->size : 0; }

bool mobx_map_has(MobxMap *map, const char *key)
{
    if (!map || !key) return false;
    int b = (int)(map_hash(key) % (unsigned long)map->bucket_count);
    MapNode *n = map->buckets[b];
    while (n) {
        if (strcmp(n->key, key) == 0) return true;
        n = n->next;
    }
    return false;
}

MobxValue mobx_map_get(MobxMap *map, const char *key)
{
    if (!map || !key) return (MobxValue){MOBX_TYPE_INT, {0}};
    int b = (int)(map_hash(key) % (unsigned long)map->bucket_count);
    MapNode *n = map->buckets[b];
    while (n) {
        if (strcmp(n->key, key) == 0) return n->value;
        n = n->next;
    }
    return (MobxValue){MOBX_TYPE_INT, {0}};
}

void mobx_map_set(MobxMap *map, const char *key, MobxValue value)
{
    if (!map || !key) return;
    int b = (int)(map_hash(key) % (unsigned long)map->bucket_count);
    MapNode *n = map->buckets[b];
    while (n) {
        if (strcmp(n->key, key) == 0) {
            if (n->value.type == MOBX_TYPE_STRING && n->value.data.s)
                free(n->value.data.s);
            n->value = value;
            return;
        }
        n = n->next;
    }
    MapNode *node = malloc(sizeof(MapNode));
    node->key = strdup(key);
    node->value = value;
    node->next = map->buckets[b];
    map->buckets[b] = node;
    map->size++;
}

void mobx_map_delete(MobxMap *map, const char *key)
{
    if (!map || !key) return;
    int b = (int)(map_hash(key) % (unsigned long)map->bucket_count);
    MapNode *prev = NULL, *n = map->buckets[b];
    while (n) {
        if (strcmp(n->key, key) == 0) {
            if (prev) prev->next = n->next;
            else map->buckets[b] = n->next;
            free(n->key);
            free(n);
            map->size--;
            return;
        }
        prev = n;
        n = n->next;
    }
}

void mobx_map_clear(MobxMap *map)
{
    if (!map) return;
    for (int i = 0; i < map->bucket_count; i++) {
        MapNode *n = map->buckets[i];
        while (n) {
            MapNode *next = n->next;
            free(n->key);
            free(n);
            n = next;
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}

void mobx_map_foreach(MobxMap *map, MobxMapIterFn fn, void *userdata)
{
    if (!map || !fn) return;
    for (int i = 0; i < map->bucket_count; i++) {
        MapNode *n = map->buckets[i];
        while (n) {
            fn(n->key, n->value, userdata);
            n = n->next;
        }
    }
}

/* ── Observable Set ─────────────────────────────────────────────────────── */

struct MobxSet {
    MobxValue *values;
    int        size;
    int        capacity;
};

MobxSet *mobx_set_create(void)
{
    MobxSet *s = calloc(1, sizeof(MobxSet));
    s->capacity = 8;
    s->values = calloc(s->capacity, sizeof(MobxValue));
    return s;
}

void mobx_set_destroy(MobxSet *set)
{
    if (!set) return;
    free(set->values);
    free(set);
}

int mobx_set_size(MobxSet *set) { return set ? set->size : 0; }

bool mobx_set_has(MobxSet *set, MobxValue value)
{
    if (!set) return false;
    for (int i = 0; i < set->size; i++) {
        if (set->values[i].type == value.type &&
            set->values[i].data.i == value.data.i) return true;
    }
    return false;
}

void mobx_set_add(MobxSet *set, MobxValue value)
{
    if (!set || mobx_set_has(set, value)) return;
    if (set->size >= set->capacity) {
        set->capacity *= 2;
        set->values = realloc(set->values, set->capacity * sizeof(MobxValue));
    }
    set->values[set->size++] = value;
}

void mobx_set_delete(MobxSet *set, MobxValue value)
{
    if (!set) return;
    for (int i = 0; i < set->size; i++) {
        if (set->values[i].type == value.type &&
            set->values[i].data.i == value.data.i) {
            set->values[i] = set->values[set->size - 1];
            set->size--;
            return;
        }
    }
}

void mobx_set_clear(MobxSet *set) { if (set) set->size = 0; }

void mobx_set_foreach(MobxSet *set, MobxSetIterFn fn, void *userdata)
{
    if (!set || !fn) return;
    for (int i = 0; i < set->size; i++) fn(set->values[i], userdata);
}

/* ── Transaction ────────────────────────────────────────────────────────── */

static int transaction_depth = 0;
static bool transaction_active = false;

MobxTransaction *mobx_transaction_begin(void)
{
    transaction_depth++;
    transaction_active = true;
    return (MobxTransaction *)(intptr_t)1;
}

void mobx_transaction_end(MobxTransaction *tx)
{
    (void)tx;
    transaction_depth--;
    if (transaction_depth <= 0) {
        transaction_depth = 0;
        transaction_active = false;
        mobx_batch_flush();
    }
}

bool mobx_transaction_is_active(void) { return transaction_active; }
int mobx_transaction_depth(void) { return transaction_depth; }

void mobx_batch_begin(void) { transaction_depth++; transaction_active = true; }
void mobx_batch_end(void)
{
    transaction_depth--;
    if (transaction_depth <= 0) {
        transaction_depth = 0;
        transaction_active = false;
    }
}

void mobx_batch_flush(void)
{
    /* flush pending autorun/reaction calls */
}
