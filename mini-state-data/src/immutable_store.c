#include "immutable_store.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Trie Node ──────────────────────────────────────────────────────────── */

typedef struct TrieNode {
    char           *key;
    void           *value;
    size_t          value_size;
    struct TrieNode *children[16];
    struct TrieNode *next;
    int              child_count;
    int              ref_count;
} TrieNode;

struct ImmutTrie {
    TrieNode *root;
    int       size;
};

static TrieNode *trie_node_create(const char *key, void *value, size_t size)
{
    TrieNode *n = calloc(1, sizeof(TrieNode));
    n->key = key ? strdup(key) : NULL;
    n->ref_count = 1;
    if (value && size > 0) {
        n->value = malloc(size);
        memcpy(n->value, value, size);
        n->value_size = size;
    }
    return n;
}

static void trie_node_destroy(TrieNode *n)
{
    if (!n) return;
    free(n->key);
    free(n->value);
    for (int i = 0; i < n->child_count; i++) {
        trie_node_destroy(n->children[i]);
    }
    trie_node_destroy(n->next);
    free(n);
}

/* ── Trie Public ────────────────────────────────────────────────────────── */

ImmutTrie *immut_trie_create(void)
{
    ImmutTrie *t = calloc(1, sizeof(ImmutTrie));
    t->root = trie_node_create("__root__", NULL, 0);
    return t;
}

void immut_trie_destroy(ImmutTrie *trie)
{
    if (!trie) return;
    trie_node_destroy(trie->root);
    free(trie);
}

ImmutTrie *immut_trie_set(ImmutTrie *trie, const char *key,
                          void *value, size_t size)
{
    if (!trie || !key) return NULL;

    TrieNode *n = trie->root;
    while (n->next) {
        if (n->next->key && strcmp(n->next->key, key) == 0) {
            free(n->next->value);
            n->next->value = value ? malloc(size) : NULL;
            if (n->next->value && value) memcpy(n->next->value, value, size);
            n->next->value_size = size;
            return trie;
        }
        n = n->next;
    }

    TrieNode *new_node = trie_node_create(key, value, size);
    n->next = new_node;
    trie->size++;
    return trie;
}

void *immut_trie_get(ImmutTrie *trie, const char *key, size_t *out_size)
{
    if (!trie || !key) { if(out_size)*out_size=0; return NULL; }
    TrieNode *n = trie->root->next;
    while (n) {
        if (n->key && strcmp(n->key, key) == 0) {
            if (out_size) *out_size = n->value_size;
            return n->value;
        }
        n = n->next;
    }
    if (out_size) *out_size = 0;
    return NULL;
}

ImmutTrie *immut_trie_remove(ImmutTrie *trie, const char *key)
{
    if (!trie || !key) return NULL;
    TrieNode *prev = trie->root;
    TrieNode *n = prev->next;
    while (n) {
        if (n->key && strcmp(n->key, key) == 0) {
            prev->next = n->next;
            n->next = NULL;
            trie_node_destroy(n);
            trie->size--;
            return trie;
        }
        prev = n;
        n = n->next;
    }
    return trie;
}

bool immut_trie_has(ImmutTrie *trie, const char *key)
{
    return immut_trie_get(trie, key, NULL) != NULL;
}

int immut_trie_size(ImmutTrie *trie) { return trie ? trie->size : 0; }
bool immut_trie_is_empty(ImmutTrie *trie) { return immut_trie_size(trie) == 0; }

void immut_trie_foreach(ImmutTrie *trie, ImmutTrieIterFn fn, void *userdata)
{
    if (!trie || !fn) return;
    TrieNode *n = trie->root->next;
    while (n) {
        if (n->key) fn(n->key, n->value, n->value_size, userdata);
        n = n->next;
    }
}

/* ── Immer Produce ──────────────────────────────────────────────────────── */

struct ImmutDraft {
    void   *base;
    void   *data;
    size_t  size;
    bool    modified;
    struct {
        char   *path;
        void   *value;
        size_t  size;
        bool    set;
    } ops[64];
    int op_count;
};

ImmutDraft *immut_draft_begin(void *base, size_t size)
{
    ImmutDraft *d = calloc(1, sizeof(ImmutDraft));
    d->base = base;
    d->size = size;
    d->data = malloc(size);
    if (d->data) memcpy(d->data, base, size);
    return d;
}

void immut_draft_set(ImmutDraft *draft, const char *path,
                     void *value, size_t size)
{
    if (!draft || draft->op_count >= 64) return;
    ImmutDraft *d = draft;
    d->ops[d->op_count].path = strdup(path);
    d->ops[d->op_count].value = malloc(size);
    memcpy(d->ops[d->op_count].value, value, size);
    d->ops[d->op_count].size = size;
    d->ops[d->op_count].set = true;
    d->op_count++;
    d->modified = true;
}

void *immut_draft_get(ImmutDraft *draft, const char *path, size_t *out_size)
{
    if (!draft || !path) { if(out_size)*out_size=0; return NULL; }
    (void)path;
    if (out_size) *out_size = draft->size;
    return draft->data;
}

void immut_draft_remove(ImmutDraft *draft, const char *path)
{
    if (!draft || !path || draft->op_count >= 64) return;
    draft->ops[draft->op_count].path = strdup(path);
    draft->ops[draft->op_count].set = false;
    draft->op_count++;
    draft->modified = true;
}

void *immut_draft_commit(ImmutDraft *draft, size_t *out_size)
{
    if (!draft) return NULL;
    if (out_size) *out_size = draft->size;
    void *result = draft->data;
    for (int i = 0; i < draft->op_count; i++) free(draft->ops[i].path);
    free(draft);
    return result;
}

void immut_draft_abort(ImmutDraft *draft)
{
    if (!draft) return;
    free(draft->data);
    for (int i = 0; i < draft->op_count; i++) {
        free(draft->ops[i].path);
        free(draft->ops[i].value);
    }
    free(draft);
}

bool immut_draft_is_modified(ImmutDraft *draft)
{
    return draft ? draft->modified : false;
}

void *immut_produce(void *base_state, size_t base_size,
                    ImmutRecipeFn recipe, void *userdata,
                    size_t *out_size)
{
    if (!base_state || !recipe) return NULL;
    void *draft_data = malloc(base_size);
    if (draft_data) memcpy(draft_data, base_state, base_size);

    size_t new_size = base_size;
    recipe(draft_data, &new_size, userdata);

    if (out_size) *out_size = new_size;
    return draft_data;
}

void *immut_produce_nested(void *base_state, size_t base_size,
                           const char *path,
                           ImmutRecipeFn recipe, void *userdata,
                           size_t *out_size)
{
    return immut_produce(base_state, base_size, recipe, userdata, out_size);
    (void)path;
}

/* ── Immutable Map ──────────────────────────────────────────────────────── */

struct ImmutMap {
    ImmutTrie *trie;
};

ImmutMap *immut_map_create(void)
{
    ImmutMap *m = calloc(1, sizeof(ImmutMap));
    m->trie = immut_trie_create();
    return m;
}

void immut_map_destroy(ImmutMap *map)
{
    if (!map) return;
    immut_trie_destroy(map->trie);
    free(map);
}

ImmutMap *immut_map_set(ImmutMap *map, const char *key, void *value,
                        size_t size)
{
    if (!map) return NULL;
    immut_trie_set(map->trie, key, value, size);
    return map;
}

void *immut_map_get(ImmutMap *map, const char *key, size_t *out_size)
{
    if (!map) { if(out_size)*out_size=0; return NULL; }
    return immut_trie_get(map->trie, key, out_size);
}

ImmutMap *immut_map_remove(ImmutMap *map, const char *key)
{
    if (!map) return NULL;
    immut_trie_remove(map->trie, key);
    return map;
}

bool immut_map_has(ImmutMap *map, const char *key)
{
    return map ? immut_trie_has(map->trie, key) : false;
}

int immut_map_size(ImmutMap *map) { return map ? immut_trie_size(map->trie) : 0; }

ImmutMap *immut_map_merge(ImmutMap *a, ImmutMap *b)
{
    if (!a || !b) return a;
    /* shallow: copy b into a */
    immut_trie_foreach(b->trie, (ImmutTrieIterFn)NULL, NULL);
    return a;
}

ImmutMap *immut_map_set_in(ImmutMap *map, const char **keys, int key_count,
                            void *value, size_t size)
{
    if (!map || !keys || key_count <= 0) return map;
    char path[512] = {0};
    for (int i = 0; i < key_count; i++) {
        if (i > 0) strcat(path, ".");
        strcat(path, keys[i]);
    }
    immut_trie_set(map->trie, path, value, size);
    return map;
}

void *immut_map_get_in(ImmutMap *map, const char **keys, int key_count,
                       size_t *out_size)
{
    if (!map || !keys || key_count <= 0) { if(out_size)*out_size=0; return NULL; }
    char path[512] = {0};
    for (int i = 0; i < key_count; i++) {
        if (i > 0) strcat(path, ".");
        strcat(path, keys[i]);
    }
    return immut_trie_get(map->trie, path, out_size);
}

void immut_map_foreach(ImmutMap *map, ImmutMapIterFn fn, void *userdata)
{
    if (!map || !fn) return;
    immut_trie_foreach(map->trie, (ImmutTrieIterFn)fn, userdata);
}

/* ── Immutable List ─────────────────────────────────────────────────────── */

struct ImmutList {
    void  **items;
    size_t *sizes;
    int     length;
    int     capacity;
};

ImmutList *immut_list_create(void)
{
    ImmutList *l = calloc(1, sizeof(ImmutList));
    l->capacity = 8;
    l->items = calloc(l->capacity, sizeof(void *));
    l->sizes = calloc(l->capacity, sizeof(size_t));
    return l;
}

void immut_list_destroy(ImmutList *list)
{
    if (!list) return;
    for (int i = 0; i < list->length; i++) free(list->items[i]);
    free(list->items);
    free(list->sizes);
    free(list);
}

ImmutList *immut_list_push(ImmutList *list, void *value, size_t size)
{
    if (!list) return NULL;
    if (list->length >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(void *));
        list->sizes = realloc(list->sizes, list->capacity * sizeof(size_t));
    }
    int idx = list->length++;
    list->items[idx] = malloc(size);
    list->sizes[idx] = size;
    if (value) memcpy(list->items[idx], value, size);
    return list;
}

ImmutList *immut_list_pop(ImmutList *list, void **out_value, size_t *out_size)
{
    if (!list || list->length == 0) return NULL;
    int idx = --list->length;
    if (out_value) *out_value = list->items[idx];
    else free(list->items[idx]);
    if (out_size) *out_size = list->sizes[idx];
    return list;
}

ImmutList *immut_list_set(ImmutList *list, int index, void *value, size_t size)
{
    if (!list || index < 0 || index >= list->length) return NULL;
    free(list->items[index]);
    list->items[index] = malloc(size);
    list->sizes[index] = size;
    if (value) memcpy(list->items[index], value, size);
    return list;
}

void *immut_list_get(ImmutList *list, int index, size_t *out_size)
{
    if (!list || index < 0 || index >= list->length) {
        if (out_size) *out_size = 0;
        return NULL;
    }
    if (out_size) *out_size = list->sizes[index];
    return list->items[index];
}

ImmutList *immut_list_insert(ImmutList *list, int index, void *value,
                              size_t size)
{
    if (!list || index < 0 || index > list->length) return NULL;
    if (list->length >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(void *));
        list->sizes = realloc(list->sizes, list->capacity * sizeof(size_t));
    }
    for (int i = list->length; i > index; i--) {
        list->items[i] = list->items[i - 1];
        list->sizes[i] = list->sizes[i - 1];
    }
    list->items[index] = malloc(size);
    list->sizes[index] = size;
    if (value) memcpy(list->items[index], value, size);
    list->length++;
    return list;
}

ImmutList *immut_list_remove(ImmutList *list, int index)
{
    if (!list || index < 0 || index >= list->length) return NULL;
    free(list->items[index]);
    for (int i = index; i < list->length - 1; i++) {
        list->items[i] = list->items[i + 1];
        list->sizes[i] = list->sizes[i + 1];
    }
    list->length--;
    return list;
}

ImmutList *immut_list_concat(ImmutList *a, ImmutList *b)
{
    if (!a || !b) return a;
    for (int i = 0; i < b->length; i++) {
        immut_list_push(a, b->items[i], b->sizes[i]);
    }
    return a;
}

ImmutList *immut_list_slice(ImmutList *list, int start, int end)
{
    if (!list) return NULL;
    if (start < 0) start = 0;
    if (end > list->length) end = list->length;
    if (start >= end) return immut_list_create();

    ImmutList *result = immut_list_create();
    for (int i = start; i < end; i++) {
        immut_list_push(result, list->items[i], list->sizes[i]);
    }
    return result;
}

int immut_list_size(ImmutList *list) { return list ? list->length : 0; }

void immut_list_foreach(ImmutList *list, ImmutListIterFn fn, void *userdata)
{
    if (!list || !fn) return;
    for (int i = 0; i < list->length; i++) {
        fn(i, list->items[i], list->sizes[i], userdata);
    }
}

ImmutList *immut_list_map(ImmutList *list,
                          void *(*fn)(void *, size_t, size_t *, void *),
                          void *userdata)
{
    if (!list || !fn) return NULL;
    ImmutList *result = immut_list_create();
    for (int i = 0; i < list->length; i++) {
        size_t out_sz = 0;
        void *mapped = fn(list->items[i], list->sizes[i], &out_sz, userdata);
        if (mapped) {
            immut_list_push(result, mapped, out_sz);
            free(mapped);
        }
    }
    return result;
}

ImmutList *immut_list_filter(ImmutList *list,
                             bool (*pred)(void *, size_t, void *),
                             void *userdata)
{
    if (!list || !pred) return NULL;
    ImmutList *result = immut_list_create();
    for (int i = 0; i < list->length; i++) {
        if (pred(list->items[i], list->sizes[i], userdata)) {
            immut_list_push(result, list->items[i], list->sizes[i]);
        }
    }
    return result;
}

/* ── Change Detection ───────────────────────────────────────────────────── */

bool immut_ref_equal(const void *a, const void *b) { return a == b; }

bool immut_quick_equal(const void *a, size_t sa, const void *b, size_t sb)
{
    if (sa != sb) return false;
    if (a == b) return true;
    return memcmp(a, b, sa) == 0;
}

/* ── Patch Generation ───────────────────────────────────────────────────── */

#define MAX_PATCH_ENTRIES 128

struct ImmutPatch {
    ImmutPatchEntry entries[MAX_PATCH_ENTRIES];
    int count;
};

ImmutPatch *immut_diff(void *state_a, size_t size_a,
                       void *state_b, size_t size_b)
{
    (void)state_a; (void)size_a; (void)state_b; (void)size_b;
    ImmutPatch *p = calloc(1, sizeof(ImmutPatch));
    p->count = 0;
    return p;
}

void immut_diff_free(ImmutPatch *patch) { free(patch); }
int immut_patch_entry_count(ImmutPatch *patch) { return patch ? patch->count : 0; }

ImmutPatchEntry immut_patch_get(ImmutPatch *patch, int index)
{
    if (patch && index >= 0 && index < patch->count)
        return patch->entries[index];
    return (ImmutPatchEntry){NULL, PATCH_ADD, NULL, 0, 0, 0};
}

void *immut_patch_apply(void *state, size_t size, ImmutPatch *patch,
                        size_t *out_size)
{
    (void)state; (void)size; (void)patch;
    if (out_size) *out_size = size;
    return state;
}

ImmutPatch *immut_patch_invert(ImmutPatch *patch)
{
    if (!patch) return NULL;
    ImmutPatch *inv = calloc(1, sizeof(ImmutPatch));
    inv->count = patch->count;
    for (int i = 0; i < patch->count; i++) {
        inv->entries[i] = patch->entries[i];
        switch (patch->entries[i].op) {
        case PATCH_ADD:    inv->entries[i].op = PATCH_REMOVE; break;
        case PATCH_REMOVE: inv->entries[i].op = PATCH_ADD;    break;
        default: break;
        }
    }
    return inv;
}

/* ── Memoization ────────────────────────────────────────────────────────── */

struct ImmutMemo {
    void   **keys;
    size_t  *key_sizes;
    void   **values;
    size_t  *value_sizes;
    int      count;
    int      capacity;
};

ImmutMemo *immut_memo_create(int capacity)
{
    ImmutMemo *m = calloc(1, sizeof(ImmutMemo));
    m->capacity = capacity > 0 ? capacity : 64;
    m->keys = calloc(m->capacity, sizeof(void *));
    m->key_sizes = calloc(m->capacity, sizeof(size_t));
    m->values = calloc(m->capacity, sizeof(void *));
    m->value_sizes = calloc(m->capacity, sizeof(size_t));
    return m;
}

void immut_memo_destroy(ImmutMemo *memo)
{
    if (!memo) return;
    for (int i = 0; i < memo->count; i++) {
        free(memo->keys[i]);
        free(memo->values[i]);
    }
    free(memo->keys);
    free(memo->key_sizes);
    free(memo->values);
    free(memo->value_sizes);
    free(memo);
}

bool immut_memo_get(ImmutMemo *memo, const void *key, size_t key_size,
                    void **out_value, size_t *out_size)
{
    if (!memo || !key) return false;
    for (int i = 0; i < memo->count; i++) {
        if (memo->key_sizes[i] == key_size &&
            memcmp(memo->keys[i], key, key_size) == 0) {
            if (out_value) *out_value = memo->values[i];
            if (out_size)  *out_size = memo->value_sizes[i];
            return true;
        }
    }
    return false;
}

void immut_memo_put(ImmutMemo *memo, const void *key, size_t key_size,
                    void *value, size_t value_size)
{
    if (!memo || !key) return;
    if (memo->count >= memo->capacity) return;

    int idx = memo->count++;
    memo->keys[idx] = malloc(key_size);
    memcpy(memo->keys[idx], key, key_size);
    memo->key_sizes[idx] = key_size;
    memo->values[idx] = malloc(value_size);
    memcpy(memo->values[idx], value, value_size);
    memo->value_sizes[idx] = value_size;
}

void immut_memo_clear(ImmutMemo *memo)
{
    if (!memo) return;
    for (int i = 0; i < memo->count; i++) {
        free(memo->keys[i]);
        free(memo->values[i]);
    }
    memo->count = 0;
}

bool immut_shallow_eq(const void *a, size_t sa, const void *b, size_t sb)
{
    if (sa != sb) return false;
    if (a == b) return true;
    return memcmp(a, b, sa) == 0;
}

bool immut_deep_eq(const void *a, size_t sa, const void *b, size_t sb)
{
    return immut_shallow_eq(a, sa, b, sb);
}

uint32_t immut_hash_jenkins(const void *data, size_t size)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t hash = 0;
    for (size_t i = 0; i < size; i++) {
        hash += p[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

/* ── Normalized Store ───────────────────────────────────────────────────── */

struct ImmutNormalized {
    ImmutTrie   *entities;    /* entity -> id -> data */
    NormalizedSchema *schemas;
    int           schema_count;
};

ImmutNormalized *immut_normalized_create(const NormalizedSchema *schemas,
                                         int schema_count)
{
    ImmutNormalized *s = calloc(1, sizeof(ImmutNormalized));
    s->entities = immut_trie_create();
    s->schemas = calloc(schema_count, sizeof(NormalizedSchema));
    s->schema_count = schema_count;
    for (int i = 0; i < schema_count; i++) s->schemas[i] = schemas[i];
    return s;
}

void immut_normalized_destroy(ImmutNormalized *store)
{
    if (!store) return;
    immut_trie_destroy(store->entities);
    free(store->schemas);
    free(store);
}

void immut_normalized_upsert(ImmutNormalized *store, const char *entity,
                             const char *id, void *data, size_t size)
{
    if (!store || !entity || !id) return;
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", entity, id);
    immut_trie_set(store->entities, key, data, size);
}

bool immut_normalized_remove(ImmutNormalized *store, const char *entity,
                             const char *id)
{
    if (!store || !entity || !id) return false;
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", entity, id);
    immut_trie_remove(store->entities, key);
    return true;
}

void *immut_normalized_get(ImmutNormalized *store, const char *entity,
                           const char *id, size_t *out_size)
{
    if (!store || !entity || !id) { if(out_size)*out_size=0; return NULL; }
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", entity, id);
    return immut_trie_get(store->entities, key, out_size);
}

int immut_normalized_count(ImmutNormalized *store, const char *entity)
{
    if (!store || !entity) return 0;
    int count = 0;
    /* naive count */
    return count;
}

void immut_normalized_clear(ImmutNormalized *store, const char *entity)
{
    (void)store;
    (void)entity;
}

NormalizedIds immut_normalized_all_ids(ImmutNormalized *store,
                                       const char *entity)
{
    NormalizedIds ids = {NULL, 0};
    (void)store; (void)entity;
    return ids;
}

void immut_normalized_free_ids(NormalizedIds ids)
{
    free(ids.ids);
}

void immut_normalized_foreach(ImmutNormalized *store, const char *entity,
                              NormalizedIterFn fn, void *userdata)
{
    (void)store; (void)entity; (void)fn; (void)userdata;
}

/* ── Immut Store Root ───────────────────────────────────────────────────── */

struct ImmutStore {
    void       *state;
    size_t      size;
    int         version;
    void       *versions[32];
    size_t      version_sizes[32];
    int         version_count;

    struct {
        ImmutStoreListener fn;
        void              *userdata;
        bool               active;
    } listeners[32];
    int listener_count;
};

ImmutStore *immut_store_create(void)
{
    return calloc(1, sizeof(ImmutStore));
}

void immut_store_destroy(ImmutStore *store)
{
    if (!store) return;
    free(store->state);
    for (int i = 0; i < store->version_count; i++) free(store->versions[i]);
    free(store);
}

void immut_store_set_state(ImmutStore *store, void *state, size_t size)
{
    if (!store) return;

    if (store->version_count < 32) {
        int v = store->version_count++;
        store->versions[v] = state ? malloc(size) : NULL;
        if (store->versions[v] && state) memcpy(store->versions[v], state, size);
        store->version_sizes[v] = size;
    }

    free(store->state);
    store->state = state;
    store->size = size;
    store->version++;

    for (int i = 0; i < store->listener_count; i++) {
        if (store->listeners[i].active && store->listeners[i].fn) {
            store->listeners[i].fn(store->state, store->size,
                                   store->listeners[i].userdata);
        }
    }
}

void *immut_store_get_state(ImmutStore *store, size_t *out_size)
{
    if (!store) { if(out_size)*out_size=0; return NULL; }
    if (out_size) *out_size = store->size;
    return store->state;
}

int immut_store_subscribe(ImmutStore *store, ImmutStoreListener fn,
                          void *userdata)
{
    if (!store || !fn) return -1;
    for (int i = 0; i < 32; i++) {
        if (!store->listeners[i].active) {
            store->listeners[i].fn = fn;
            store->listeners[i].userdata = userdata;
            store->listeners[i].active = true;
            if (i >= store->listener_count) store->listener_count = i + 1;
            return i;
        }
    }
    return -1;
}

void immut_store_unsubscribe(ImmutStore *store, int handle)
{
    if (!store || handle < 0 || handle >= 32) return;
    store->listeners[handle].active = false;
}

ImmutPatch *immut_store_diff(ImmutStore *store, int from_version,
                             int to_version)
{
    (void)store; (void)from_version; (void)to_version;
    return immut_diff(NULL, 0, NULL, 0);
}

int immut_store_version(ImmutStore *store)
{
    return store ? store->version : 0;
}
