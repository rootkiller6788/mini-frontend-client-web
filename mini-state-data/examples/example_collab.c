#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "immutable_store.h"
#include "mobx_observable.h"
#include "redux_store.h"
#include "query_cache.h"

/* ── Collaborative state with CRDT-light approach ──────────────────────── */

typedef struct {
    char   client_id[32];
    char  *buffer;
    int    length;
    int    capacity;
    int    cursor;
    int    version;
} Document;

typedef struct {
    char    type[16];    /* "insert", "delete" */
    int     position;
    char    text[256];
    int     length;
    int     version;
    char    client_id[32];
} Operation;

typedef struct {
    Document   *document;
    Operation  *history;
    int         history_count;
    int         history_cap;
    ImmutStore *store;
} CollabState;

/* ── Document helpers ───────────────────────────────────────────────────── */

static Document *doc_create(const char *client_id)
{
    Document *d = calloc(1, sizeof(Document));
    strncpy(d->client_id, client_id, 31);
    d->capacity = 1024;
    d->buffer = calloc(d->capacity, 1);
    d->length = 0;
    d->cursor = 0;
    d->version = 0;
    return d;
}

static void doc_insert(Document *doc, int pos, const char *text, int len)
{
    if (!doc || pos < 0 || pos > doc->length) return;
    if (doc->length + len >= doc->capacity) {
        doc->capacity = (doc->capacity + len) * 2;
        doc->buffer = realloc(doc->buffer, doc->capacity);
    }
    memmove(doc->buffer + pos + len, doc->buffer + pos, doc->length - pos);
    memcpy(doc->buffer + pos, text, len);
    doc->length += len;
    doc->version++;
}

static void doc_delete(Document *doc, int pos, int len)
{
    if (!doc || pos < 0 || pos + len > doc->length) return;
    memmove(doc->buffer + pos, doc->buffer + pos + len,
            doc->length - pos - len);
    doc->length -= len;
    doc->version++;
}

/* ── Operation history ──────────────────────────────────────────────────── */

static void add_operation(CollabState *cs, const char *type, int pos,
                          const char *text, int len)
{
    if (cs->history_count >= cs->history_cap) {
        cs->history_cap = cs->history_cap ? cs->history_cap * 2 : 16;
        cs->history = realloc(cs->history, cs->history_cap * sizeof(Operation));
    }
    Operation *op = &cs->history[cs->history_count++];
    strncpy(op->type, type, 15);
    op->position = pos;
    op->length = len;
    if (text && len > 0) {
        strncpy(op->text, text, len < 255 ? len : 255);
        op->text[len < 255 ? len : 255] = '\0';
    }
    op->version = cs->document->version;
    strncpy(op->client_id, cs->document->client_id, 31);
}

/* ── MobX: Observable cursor positions ──────────────────────────────────── */

static MobxObservable *g_cursor_obs = NULL;

static void update_cursor(MobxObservable *obs, int position)
{
    mobx_observable_set(obs, MOBX_INT(position));
}

/* ── Redux: Undo/Redo via action dispatch ────────────────────────────────── */

static void *collab_reducer(void *state, const ReduxAction *action,
                            size_t *out_size)
{
    CollabState *cs = (CollabState *)state;
    if (!cs) return NULL;

    if (strcmp(action->type, "COLLAB_UNDO") == 0) {
        if (cs->history_count > 0) {
            Operation *op = &cs->history[cs->history_count - 1];
            if (strcmp(op->type, "insert") == 0) {
                doc_delete(cs->document, op->position, op->length);
            } else if (strcmp(op->type, "delete") == 0) {
                doc_insert(cs->document, op->position, op->text, op->length);
            }
            cs->history_count--;
        }
    } else if (strcmp(action->type, "COLLAB_REDO") == 0) {
        /* redo would replay from separate redo stack */
    }

    *out_size = sizeof(CollabState);
    return cs;
}

/* ── Query cache: persist document ──────────────────────────────────────── */

static void *save_document_fn(const char *key, void *context, size_t *out_size)
{
    (void)key;
    CollabState *cs = (CollabState *)context;
    printf("  [save] Persisting document (v%d, %d chars)...\n",
           cs->document->version, cs->document->length);

    void *data = malloc(cs->document->length + 1);
    memcpy(data, cs->document->buffer, cs->document->length);
    ((char *)data)[cs->document->length] = '\0';
    *out_size = cs->document->length + 1;
    return data;
}

/* ── Immutable patches for OTs ──────────────────────────────────────────── */

static void demo_patches(void)
{
    printf("  Creating patches...\n");
    char state_a[] = "hello world";
    char state_b[] = "hello wonderful world";

    ImmutPatch *patch = immut_diff(state_a, strlen(state_a),
                                   state_b, strlen(state_b));
    printf("  Patch entries: %d\n", immut_patch_entry_count(patch));

    ImmutPatch *inverted = immut_patch_invert(patch);
    printf("  Inverted entries: %d\n", immut_patch_entry_count(inverted));

    immut_diff_free(patch);
    immut_diff_free(inverted);
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== mini-state-data: Collaborative State Demo ===\n\n");

    CollabState cs = {0};
    cs.document = doc_create("client-alpha");
    cs.history_cap = 16;
    cs.history = calloc(cs.history_cap, sizeof(Operation));
    cs.store = immut_store_create();

    /* Initial state snapshot */
    immut_store_set_state(cs.store, cs.document->buffer, cs.document->length);
    printf("Version: %d\n", immut_store_version(cs.store));

    /* Client A makes edits */
    printf("\n--- Client A editing ---\n");
    doc_insert(cs.document, 0, "Hello ", 6);
    add_operation(&cs, "insert", 0, "Hello ", 6);
    printf("  Text: %s\n", cs.document->buffer);

    doc_insert(cs.document, cs.document->length, "world!", 6);
    add_operation(&cs, "insert", cs.document->length - 6, "world!", 6);
    printf("  Text: %s\n", cs.document->buffer);

    doc_insert(cs.document, 6, "beautiful ", 10);
    add_operation(&cs, "insert", 6, "beautiful ", 10);
    printf("  Text: %s\n", cs.document->buffer);

    /* Version tracking with immutable store */
    immut_store_set_state(cs.store, cs.document->buffer, cs.document->length);
    printf("  Version: %d, state size: %d\n",
           immut_store_version(cs.store), cs.document->length);

    /* MobX: cursor tracking */
    printf("\n--- Cursor Tracking (MobX) ---\n");
    g_cursor_obs = mobx_observable_create("cursor", MOBX_INT(0));
    update_cursor(g_cursor_obs, 10);
    printf("  Cursor position: %lld\n",
           (long long)mobx_observable_get(g_cursor_obs).data.i);

    /* Redux: Undo stack */
    printf("\n--- Undo via Redux ---\n");
    ReduxReducer reducer = {collab_reducer, NULL, NULL};
    ReduxStore *redux = redux_store_create(&cs, sizeof(CollabState), reducer);

    printf("  Before undo: '%s'\n", cs.document->buffer);
    ReduxAction undo_action = REDUX_ACTION("COLLAB_UNDO");
    redux_store_dispatch(redux, &undo_action);
    printf("  After undo:  '%s'\n", cs.document->buffer);

    undo_action = REDUX_ACTION("COLLAB_UNDO");
    redux_store_dispatch(redux, &undo_action);
    printf("  After undo 2: '%s'\n", cs.document->buffer);

    /* OT-style patches */
    printf("\n--- Operational Transformation (Patch) ---\n");
    demo_patches();

    /* Persist via query cache */
    printf("\n--- Persistence via Query Cache ---\n");
    QueryCache *cache = query_cache_create();
    QueryOptions save_opts = {0};
    save_opts.query_key = "document:main";
    save_opts.cache_time_ms = 3600000;

    query_cache_mutate(cache, "save-doc", save_document_fn,
                       &cs, sizeof(CollabState), NULL);

    /* Normalized store for user presence */
    printf("\n--- Presence (Normalized Store) ---\n");
    NormalizedSchema schemas[] = {{"user", "id", NULL, 0}};
    ImmutNormalized *presence = immut_normalized_create(schemas, 1);

    typedef struct { char id[32]; char name[64]; int cursor_pos; } UserPresence;
    UserPresence alice = {"alice", "Alice", 12};
    UserPresence bob = {"bob", "Bob", 25};

    immut_normalized_upsert(presence, "user", "alice", &alice, sizeof(UserPresence));
    immut_normalized_upsert(presence, "user", "bob", &bob, sizeof(UserPresence));

    size_t psz;
    UserPresence *found = immut_normalized_get(presence, "user", "bob", &psz);
    if (found) printf("  Bob's cursor: %d\n", found->cursor_pos);

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    immut_normalized_destroy(presence);
    query_cache_destroy(cache);
    redux_store_destroy(redux);
    mobx_observable_destroy(g_cursor_obs);
    immut_store_destroy(cs.store);
    free(cs.history);
    free(cs.document->buffer);
    free(cs.document);

    printf("Done.\n");
    return 0;
}
