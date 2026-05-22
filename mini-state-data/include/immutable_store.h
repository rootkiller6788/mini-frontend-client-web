#ifndef IMMUTABLE_STORE_H
#define IMMUTABLE_STORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core Types ─────────────────────────────────────────────────────────── */

typedef struct ImmutTrie      ImmutTrie;
typedef struct ImmutMap       ImmutMap;
typedef struct ImmutList      ImmutList;
typedef struct ImmutStore     ImmutStore;
typedef struct ImmutPatch     ImmutPatch;
typedef struct ImmutNormalized ImmutNormalized;

/* ── Trie (Structural Sharing) ──────────────────────────────────────────── */

ImmutTrie  *immut_trie_create(void);
void        immut_trie_destroy(ImmutTrie *trie);

ImmutTrie  *immut_trie_set(ImmutTrie *trie, const char *key,
                            void *value, size_t size);
void       *immut_trie_get(ImmutTrie *trie, const char *key, size_t *out_size);
ImmutTrie  *immut_trie_remove(ImmutTrie *trie, const char *key);
bool        immut_trie_has(ImmutTrie *trie, const char *key);
int         immut_trie_size(ImmutTrie *trie);
bool        immut_trie_is_empty(ImmutTrie *trie);

typedef void (*ImmutTrieIterFn)(const char *key, void *value, size_t size,
                                void *userdata);
void        immut_trie_foreach(ImmutTrie *trie, ImmutTrieIterFn fn,
                               void *userdata);

/* ── Immer Produce ──────────────────────────────────────────────────────── */

typedef void (*ImmutRecipeFn)(void *draft, size_t *out_size, void *userdata);

void *immut_produce(void *base_state, size_t base_size,
                    ImmutRecipeFn recipe, void *userdata,
                    size_t *out_size);
void *immut_produce_nested(void *base_state, size_t base_size,
                           const char *path,
                           ImmutRecipeFn recipe, void *userdata,
                           size_t *out_size);

typedef struct ImmutDraft ImmutDraft;

ImmutDraft *immut_draft_begin(void *base, size_t size);
void        immut_draft_set(ImmutDraft *draft, const char *path,
                            void *value, size_t size);
void       *immut_draft_get(ImmutDraft *draft, const char *path,
                            size_t *out_size);
void        immut_draft_remove(ImmutDraft *draft, const char *path);
void       *immut_draft_commit(ImmutDraft *draft, size_t *out_size);
void        immut_draft_abort(ImmutDraft *draft);
bool        immut_draft_is_modified(ImmutDraft *draft);

/* ── Immutable Map ──────────────────────────────────────────────────────── */

ImmutMap *immut_map_create(void);
void      immut_map_destroy(ImmutMap *map);

ImmutMap *immut_map_set(ImmutMap *map, const char *key, void *value,
                         size_t size);
void     *immut_map_get(ImmutMap *map, const char *key, size_t *out_size);
ImmutMap *immut_map_remove(ImmutMap *map, const char *key);
bool      immut_map_has(ImmutMap *map, const char *key);
int       immut_map_size(ImmutMap *map);

ImmutMap *immut_map_merge(ImmutMap *a, ImmutMap *b);
ImmutMap *immut_map_set_in(ImmutMap *map, const char **keys, int key_count,
                            void *value, size_t size);
void     *immut_map_get_in(ImmutMap *map, const char **keys, int key_count,
                           size_t *out_size);

typedef void (*ImmutMapIterFn)(const char *key, void *value, size_t size,
                               void *userdata);
void      immut_map_foreach(ImmutMap *map, ImmutMapIterFn fn, void *userdata);

/* ── Immutable List ─────────────────────────────────────────────────────── */

ImmutList *immut_list_create(void);
void       immut_list_destroy(ImmutList *list);

ImmutList *immut_list_push(ImmutList *list, void *value, size_t size);
ImmutList *immut_list_pop(ImmutList *list, void **out_value, size_t *out_size);
ImmutList *immut_list_set(ImmutList *list, int index, void *value, size_t size);
void      *immut_list_get(ImmutList *list, int index, size_t *out_size);
ImmutList *immut_list_insert(ImmutList *list, int index, void *value,
                              size_t size);
ImmutList *immut_list_remove(ImmutList *list, int index);
ImmutList *immut_list_concat(ImmutList *a, ImmutList *b);
ImmutList *immut_list_slice(ImmutList *list, int start, int end);
int        immut_list_size(ImmutList *list);

typedef void (*ImmutListIterFn)(int index, void *value, size_t size,
                                void *userdata);
void       immut_list_foreach(ImmutList *list, ImmutListIterFn fn,
                              void *userdata);
ImmutList *immut_list_map(ImmutList *list,
                          void *(*fn)(void *val, size_t size, size_t *out_sz,
                                      void *ud),
                          void *userdata);
ImmutList *immut_list_filter(ImmutList *list,
                             bool (*pred)(void *val, size_t size, void *ud),
                             void *userdata);

/* ── Change Detection ───────────────────────────────────────────────────── */

bool immut_ref_equal(const void *a, const void *b);
bool immut_quick_equal(const void *a, size_t sa, const void *b, size_t sb);

/* ── Patch Generation ───────────────────────────────────────────────────── */

typedef enum {
    PATCH_ADD,
    PATCH_REMOVE,
    PATCH_REPLACE,
    PATCH_MOVE
} PatchOp;

typedef struct ImmutPatchEntry {
    const char *path;
    PatchOp     op;
    void       *value;
    size_t      value_size;
    int         from_index;
    int         to_index;
} ImmutPatchEntry;

ImmutPatch *immut_diff(void *state_a, size_t size_a,
                       void *state_b, size_t size_b);
void        immut_diff_free(ImmutPatch *patch);
int         immut_patch_entry_count(ImmutPatch *patch);
ImmutPatchEntry immut_patch_get(ImmutPatch *patch, int index);
void       *immut_patch_apply(void *state, size_t size,
                              ImmutPatch *patch, size_t *out_size);
ImmutPatch *immut_patch_invert(ImmutPatch *patch);

/* ── Memoization ────────────────────────────────────────────────────────── */

typedef uint32_t (*ImmutHashFn)(const void *data, size_t size);
typedef bool     (*ImmutEqualFn)(const void *a, size_t sa,
                                 const void *b, size_t sb);

typedef struct ImmutMemo ImmutMemo;

ImmutMemo *immut_memo_create(int capacity);
void       immut_memo_destroy(ImmutMemo *memo);

bool immut_memo_get(ImmutMemo *memo, const void *key, size_t key_size,
                    void **out_value, size_t *out_size);
void immut_memo_put(ImmutMemo *memo, const void *key, size_t key_size,
                    void *value, size_t value_size);
void immut_memo_clear(ImmutMemo *memo);

bool immut_shallow_eq(const void *a, size_t sa, const void *b, size_t sb);
bool immut_deep_eq(const void *a, size_t sa, const void *b, size_t sb);
uint32_t immut_hash_jenkins(const void *data, size_t size);

/* ── Normalized Store ───────────────────────────────────────────────────── */

typedef struct NormalizedSchema {
    const char *entity_name;
    const char *id_field;        /* field name for entity ID, NULL for auto */
    const char **fields;
    int          field_count;
} NormalizedSchema;

ImmutNormalized *immut_normalized_create(const NormalizedSchema *schemas,
                                         int schema_count);
void             immut_normalized_destroy(ImmutNormalized *store);

typedef struct {
    const char *entity;
    const char *id;
    void       *data;
    size_t      data_size;
} NormalizedEntity;

void immut_normalized_upsert(ImmutNormalized *store, const char *entity,
                             const char *id, void *data, size_t size);
bool immut_normalized_remove(ImmutNormalized *store, const char *entity,
                             const char *id);
void *immut_normalized_get(ImmutNormalized *store, const char *entity,
                           const char *id, size_t *out_size);
int   immut_normalized_count(ImmutNormalized *store, const char *entity);
void  immut_normalized_clear(ImmutNormalized *store, const char *entity);

typedef struct NormalizedIds {
    const char **ids;
    int          count;
} NormalizedIds;

NormalizedIds immut_normalized_all_ids(ImmutNormalized *store,
                                       const char *entity);
void immut_normalized_free_ids(NormalizedIds ids);

typedef void (*NormalizedIterFn)(const char *id, void *data, size_t size,
                                 void *userdata);
void immut_normalized_foreach(ImmutNormalized *store, const char *entity,
                              NormalizedIterFn fn, void *userdata);

/* ── Immutable Store Root ───────────────────────────────────────────────── */

ImmutStore *immut_store_create(void);
void        immut_store_destroy(ImmutStore *store);

void        immut_store_set_state(ImmutStore *store, void *state, size_t size);
void       *immut_store_get_state(ImmutStore *store, size_t *out_size);

typedef void (*ImmutStoreListener)(void *new_state, size_t size,
                                   void *userdata);
int  immut_store_subscribe(ImmutStore *store, ImmutStoreListener fn,
                           void *userdata);
void immut_store_unsubscribe(ImmutStore *store, int handle);

ImmutPatch *immut_store_diff(ImmutStore *store, int from_version,
                             int to_version);
int         immut_store_version(ImmutStore *store);

#ifdef __cplusplus
}
#endif

#endif /* IMMUTABLE_STORE_H */
